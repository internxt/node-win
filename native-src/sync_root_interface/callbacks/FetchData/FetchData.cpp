#include "stdafx.h"
#include <Callbacks.h>
#include <string>
#include <condition_variable>
#include <mutex>
#include <FileCopierWithProgress.h>
#include <fstream>
#include <vector>
#include <utility>
#include <cfapi.h>
#include <windows.h>
#include <iostream>
#include <chrono>
#include "Utilities.h"
#include <locale>
#include <codecvt>
#include <filesystem>
#include <Logger.h>
#include <TransferContext.h>
#include "napi_extract_args.h"

napi_threadsafe_function g_fetch_data_threadsafe_callback = nullptr;

#define CHUNK_SIZE (32 * 1024 * 1024)

napi_value create_response(napi_env env, bool finished)
{
    napi_value result;
    napi_get_boolean(env, finished, &result);
    return result;
}

size_t file_incremental_reading(napi_env env, TransferContext &ctx, bool final_step, float &progress)
{
    std::ifstream file(ctx.tmpPath, std::ios::in | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open tmp file");
    }

    file.clear();
    file.seekg(0, std::ios::end);
    size_t newSize = static_cast<size_t>(file.tellg());

    size_t datasizeAvailableUnread = newSize - ctx.lastReadOffset;
    size_t growth = newSize - ctx.lastSize;

    try
    {
        if (datasizeAvailableUnread > 0)
        {
            std::vector<char> buffer(CHUNK_SIZE);
            file.seekg(ctx.lastReadOffset);
            file.read(buffer.data(), CHUNK_SIZE);

            LARGE_INTEGER startingOffset, chunkBufferSize;
            startingOffset.QuadPart = ctx.lastReadOffset;
            chunkBufferSize.QuadPart = min(datasizeAvailableUnread, CHUNK_SIZE);

            FileCopierWithProgress::TransferData(
                ctx.connectionKey,
                ctx.transferKey,
                buffer.data(),
                startingOffset,
                chunkBufferSize,
                STATUS_SUCCESS);

            ctx.lastReadOffset += chunkBufferSize.QuadPart;

            UINT64 totalSize = static_cast<UINT64>(ctx.fileSize.QuadPart);
            progress = static_cast<float>(ctx.lastReadOffset) / static_cast<float>(totalSize);
            Utilities::ApplyTransferStateToFile(ctx.path, ctx.callbackInfo, totalSize, ctx.lastReadOffset);
        }
    }
    catch (...)
    {
        Logger::getInstance().log("Excepción en file_incremental_reading.", LogLevel::ERROR);
        ctx.loadFinished = true;
        FileCopierWithProgress::TransferData(
            ctx.connectionKey,
            ctx.transferKey,
            nullptr,
            ctx.requiredOffset,
            ctx.requiredLength,
            STATUS_UNSUCCESSFUL);
    }

    file.close();
    ctx.lastSize = newSize;
    return ctx.lastReadOffset;
}

napi_value response_callback_fn_fetch_data(napi_env env, napi_callback_info info)
{
    wprintf(L"Function response_callback_fn_fetch_data called\n");

    auto [response, tmpPath] = napi_extract_args<bool, std::wstring>(env, info);

    TransferContext *ctx = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, nullptr, reinterpret_cast<void **>(&ctx));

    if (!response)
    {
        wprintf(L"Canceling hydration\n");

        ctx->loadFinished = true;
        ctx->lastReadOffset = 0;

        std::lock_guard<std::mutex> lock(ctx->mtx);
        ctx->ready = true;
        ctx->cv.notify_one();

        return create_response(env, true);
    }

    wprintf(L"Download tmp path: %s\n", tmpPath.c_str());

    ctx->tmpPath = tmpPath;

    float progress = 0.0f;
    ctx->lastReadOffset = file_incremental_reading(env, *ctx, false, progress);

    if (ctx->lastReadOffset == (size_t)ctx->fileSize.QuadPart)
    {
        ctx->lastReadOffset = 0;
        ctx->loadFinished = true;

        Utilities::ApplyTransferStateToFile(ctx->path, ctx->callbackInfo, ctx->fileSize.QuadPart, ctx->fileSize.QuadPart);

        CfSetPinState(handleForPath(ctx->path.c_str()).get(), CF_PIN_STATE_PINNED, CF_SET_PIN_FLAG_NONE, nullptr);
    }

    {
        std::lock_guard<std::mutex> lock(ctx->mtx);
        if (ctx->loadFinished)
        {
            ctx->ready = true;
            ctx->cv.notify_one();
        }
    }

    wprintf(L"Fetch data finished: %d, progress: %.2f\n", ctx->loadFinished, progress);

    return create_response(env, ctx->loadFinished);
}

void notify_fetch_data_call(napi_env env, napi_value js_callback, void *context, void *data)
{
    TransferContext *ctx = static_cast<TransferContext *>(data);

    const wchar_t *wchar_ptr = static_cast<const wchar_t *>(ctx->callbackInfo.FileIdentity);
    DWORD fileIdentityLength = ctx->callbackInfo.FileIdentityLength / sizeof(wchar_t);

    napi_value js_fileIdentityArg;
    napi_create_string_utf16(env, (char16_t *)wchar_ptr, fileIdentityLength, &js_fileIdentityArg);

    napi_value js_response_callback_fn;
    napi_create_function(env, "callback", NAPI_AUTO_LENGTH, response_callback_fn_fetch_data, ctx, &js_response_callback_fn);

    napi_value args_to_js_callback[2] = {js_fileIdentityArg, js_response_callback_fn};

    {
        std::unique_lock<std::mutex> lock(ctx->mtx);
        ctx->ready = false;
    }

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_call_function(env, undefined, js_callback, 2, args_to_js_callback, nullptr);
}

void CALLBACK fetch_data_callback_wrapper(_In_ CONST CF_CALLBACK_INFO *callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS *callbackParameters)
{
    auto ctx = GetOrCreateTransferContext(callbackInfo->ConnectionKey, callbackInfo->TransferKey);

    ctx->fileSize = callbackInfo->FileSize;
    ctx->requiredLength = callbackParameters->FetchData.RequiredLength;
    ctx->requiredOffset = callbackParameters->FetchData.RequiredFileOffset;
    ctx->callbackInfo = *callbackInfo;
    ctx->path = std::wstring(callbackInfo->VolumeDosName) + callbackInfo->NormalizedPath;

    wprintf(L"Download path: %s\n", ctx->path.c_str());

    napi_call_threadsafe_function(g_fetch_data_threadsafe_callback, ctx.get(), napi_tsfn_blocking);

    {
        std::unique_lock<std::mutex> lock(ctx->mtx);
        while (!ctx->ready)
        {
            ctx->cv.wait(lock);
        }
    }

    RemoveTransferContext(ctx->transferKey);
}

void register_threadsafe_fetch_data_callback(const std::string &resource_name, napi_env env, InputSyncCallbacks input)
{
    std::u16string converted_resource_name(resource_name.begin(), resource_name.end());

    napi_value resource_name_value;
    napi_create_string_utf16(env, converted_resource_name.c_str(), NAPI_AUTO_LENGTH, &resource_name_value);

    napi_value fetch_data_value;
    napi_get_reference_value(env, input.fetch_data_callback_ref, &fetch_data_value);

    napi_threadsafe_function tsfn_fetch_data;
    napi_create_threadsafe_function(
        env,
        fetch_data_value,
        nullptr,
        resource_name_value,
        0,
        1,
        nullptr,
        nullptr,
        nullptr,
        notify_fetch_data_call,
        &tsfn_fetch_data);

    g_fetch_data_threadsafe_callback = tsfn_fetch_data;
}
