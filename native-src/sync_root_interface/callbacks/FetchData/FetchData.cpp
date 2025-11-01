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

#define FIELD_SIZE(type, field) (sizeof(((type *)0)->field))

#define CF_SIZE_OF_OP_PARAM(field)                  \
    (FIELD_OFFSET(CF_OPERATION_PARAMETERS, field) + \
     FIELD_SIZE(CF_OPERATION_PARAMETERS, field))

#define CHUNK_SIZE (4096 * 1024)

#define CHUNKDELAYMS 250

napi_value create_response(napi_env env, bool finished, float progress)
{
    napi_value result_object;
    napi_create_object(env, &result_object);

    napi_value finished_value;
    napi_get_boolean(env, finished, &finished_value);
    napi_set_named_property(env, result_object, "finished", finished_value);

    napi_value progress_value;
    napi_create_double(env, progress, &progress_value);
    napi_set_named_property(env, result_object, "progress", progress_value);

    return result_object;
}

static size_t file_incremental_reading(napi_env env,
                                       TransferContext &ctx,
                                       bool final_step,
                                       float &progress)
{
    std::ifstream file;
    file.open(ctx.fullServerFilePath, std::ios::in | std::ios::binary);

    if (!file.is_open())
    {
        Logger::getInstance().log("Error al abrir el archivo en file_incremental_reading.", LogLevel::ERROR);
        return ctx.lastReadOffset;
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

            HRESULT hr = FileCopierWithProgress::TransferData(
                ctx.connectionKey,
                ctx.transferKey,
                buffer.data(),
                startingOffset,
                chunkBufferSize,
                STATUS_SUCCESS);

            ctx.lastReadOffset += chunkBufferSize.QuadPart;

            if (FAILED(hr))
            {
                wprintf(L"Error en TransferData(). HRESULT: %lx\n", hr);
                ctx.loadFinished = true;
                FileCopierWithProgress::TransferData(
                    ctx.connectionKey,
                    ctx.transferKey,
                    NULL,
                    ctx.requiredOffset,
                    ctx.requiredLength,
                    STATUS_UNSUCCESSFUL);
            }
            else
            {
                UINT64 totalSize = static_cast<UINT64>(ctx.fileSize.QuadPart);
                progress = static_cast<float>(ctx.lastReadOffset) / static_cast<float>(totalSize);
                Utilities::ApplyTransferStateToFile(ctx.fullClientPath.c_str(),
                                                    ctx.callbackInfo,
                                                    totalSize,
                                                    ctx.lastReadOffset);
                ::Sleep(CHUNKDELAYMS);
            }
        }
    }
    catch (...)
    {
        Logger::getInstance().log("Excepción en file_incremental_reading.", LogLevel::ERROR);
        FileCopierWithProgress::TransferData(
            ctx.connectionKey,
            ctx.transferKey,
            NULL,
            ctx.requiredOffset,
            ctx.requiredLength,
            STATUS_UNSUCCESSFUL);
    }

    file.close();
    ctx.lastSize = newSize;
    return ctx.lastReadOffset;
}

static napi_value response_callback_fn_fetch_data(napi_env env, napi_callback_info info)
{
    Logger::getInstance().log("response_callback_fn_fetch_data called", LogLevel::DEBUG);

    auto [response, response_wstr] = napi_extract_args<bool, std::wstring>(env, info);

    TransferContext *ctxPtr = nullptr;
    napi_value thisArg = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &thisArg, reinterpret_cast<void **>(&ctxPtr));

    if (!response)
    {
        Logger::getInstance().log("JS responded with false; we cancel hydration.", LogLevel::DEBUG);

        ctxPtr->loadFinished = true;
        ctxPtr->lastReadOffset = 0;

        std::lock_guard<std::mutex> lock(ctxPtr->mtx);
        ctxPtr->ready = true;
        ctxPtr->cv.notify_one();

        return create_response(env, true, 0);
    }

    Logger::getInstance().log(
        "JS responded with server file path = " + Logger::fromWStringToString(response_wstr),
        LogLevel::DEBUG);

    ctxPtr->fullServerFilePath = response_wstr;

    float progress = 0.0f;
    ctxPtr->lastReadOffset = file_incremental_reading(env, *ctxPtr, false, progress);

    if (ctxPtr->lastReadOffset == (size_t)ctxPtr->fileSize.QuadPart)
    {
        Logger::getInstance().log("File fully read.", LogLevel::DEBUG);
        ctxPtr->lastReadOffset = 0;
        ctxPtr->loadFinished = true;

        Utilities::ApplyTransferStateToFile(
            ctxPtr->fullClientPath.c_str(),
            ctxPtr->callbackInfo,
            ctxPtr->fileSize.QuadPart,
            ctxPtr->fileSize.QuadPart);

        ::Sleep(CHUNKDELAYMS);

        CfSetPinState(handleForPath(ctxPtr->fullClientPath.c_str()).get(), CF_PIN_STATE_PINNED, CF_SET_PIN_FLAG_NONE, nullptr);
    }

    {
        std::lock_guard<std::mutex> lock(ctxPtr->mtx);
        if (ctxPtr->loadFinished)
        {
            ctxPtr->ready = true;
            ctxPtr->cv.notify_one();
        }
    }

    Logger::getInstance().log(
        "fetch data => finished: " + std::to_string(ctxPtr->loadFinished) + ", progress: " + std::to_string(progress),
        LogLevel::DEBUG);

    return create_response(env, ctxPtr->loadFinished, progress);
}

static void notify_fetch_data_call(napi_env env, napi_value js_callback, void *context, void *data)
{
    Logger::getInstance().log("notify_fetch_data_call called context isolated", LogLevel::DEBUG);
    napi_status status;
    TransferContext *ctx = static_cast<TransferContext *>(data);
    Logger::getInstance().log("notify_fetch_data_call: ctx->fullClientPath = " + Logger::fromWStringToString(ctx->fullClientPath), LogLevel::DEBUG);

    std::wstring fileIdentityWstr;
    {
        const wchar_t *wchar_ptr = static_cast<const wchar_t *>(ctx->callbackInfo.FileIdentity);
        DWORD fileIdentityLength = ctx->callbackInfo.FileIdentityLength / sizeof(wchar_t);
        fileIdentityWstr.assign(wchar_ptr, fileIdentityLength);
    }
    napi_value js_fileIdentityArg;
    {
        std::u16string u16_fileIdentity(fileIdentityWstr.begin(), fileIdentityWstr.end());
        napi_create_string_utf16(env,
                                 u16_fileIdentity.c_str(),
                                 u16_fileIdentity.size(),
                                 &js_fileIdentityArg);
    }

    napi_value js_response_callback_fn;
    napi_create_function(env,
                         "responseCallback",
                         NAPI_AUTO_LENGTH,
                         response_callback_fn_fetch_data,
                         ctx,
                         &js_response_callback_fn);

    napi_value args_to_js_callback[2] = {js_fileIdentityArg, js_response_callback_fn};

    Logger::getInstance().log("notify_fetch_data_call: calling JS function", LogLevel::DEBUG);
    napi_value undefined, result;
    status = napi_get_undefined(env, &undefined);
    if (status != napi_ok)
    {
        Logger::getInstance().log("Failed to get undefined in notify_fetch_data_call.", LogLevel::ERROR);
        return;
    }

    Logger::getInstance().log("notify_fetch_data_call: setting ctx->ready to false", LogLevel::DEBUG);
    {
        Logger::getInstance().log("notify_fetch_data_call: locking ctx->mtx", LogLevel::DEBUG);
        std::unique_lock<std::mutex> lock(ctx->mtx);
        ctx->ready = false;
    }

    status = napi_call_function(env,
                                undefined,
                                js_callback,
                                2,
                                args_to_js_callback,
                                &result);
    if (status != napi_ok)
    {
        Logger::getInstance().log("Failed to call JS function in notify_fetch_data_call.", LogLevel::ERROR);
        return;
    }

    Logger::getInstance().log("Hydration concluded or user signaled to finish in notify_fetch_data_call.", LogLevel::INFO);

    ctx->lastReadOffset = 0;
    ctx->loadFinished = false;
    ctx->ready = false;

    // RemoveTransferContext(ctx->transferKey);
}

void register_threadsafe_fetch_data_callback(const std::string &resource_name, napi_env env, InputSyncCallbacks input)
{
    std::u16string converted_resource_name(resource_name.begin(), resource_name.end());

    napi_value resource_name_value;
    napi_create_string_utf16(env, converted_resource_name.c_str(), NAPI_AUTO_LENGTH, &resource_name_value);

    napi_threadsafe_function tsfn_fetch_data;
    napi_value fetch_data_value;
    napi_get_reference_value(env, input.fetch_data_callback_ref, &fetch_data_value);

    napi_status status = napi_create_threadsafe_function(
        env,
        fetch_data_value,
        NULL,
        resource_name_value,
        0,
        1,
        NULL,
        NULL,
        NULL,
        notify_fetch_data_call,
        &tsfn_fetch_data);

    if (status != napi_ok)
    {
        napi_throw_error(env, nullptr, "Failed to create fetch data threadsafe function");
        return;
    }

    Logger::getInstance().log("Threadsafe function (fetch_data) created successfully.", LogLevel::DEBUG);

    g_fetch_data_threadsafe_callback = tsfn_fetch_data;
}

void CALLBACK fetch_data_callback_wrapper(
    _In_ CONST CF_CALLBACK_INFO *callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS *callbackParameters)
{
    Logger::getInstance().log("fetch_data_callback_wrapper called", LogLevel::DEBUG);

    auto ctx = GetOrCreateTransferContext(callbackInfo->ConnectionKey, callbackInfo->TransferKey);

    ctx->fileSize = callbackInfo->FileSize;
    ctx->requiredLength = callbackParameters->FetchData.RequiredLength;
    ctx->requiredOffset = callbackParameters->FetchData.RequiredFileOffset;
    ctx->callbackInfo = *callbackInfo;

    std::wstring fullClientPath(callbackInfo->VolumeDosName); // e.g., "C:"
    fullClientPath.append(callbackInfo->NormalizedPath);      // e.g., "\Users\file.txt"
    ctx->fullClientPath = fullClientPath;                     // Result: "C:\Users\file.txt"

    Logger::getInstance().log("Full download path: " + Logger::fromWStringToString(fullClientPath),
                              LogLevel::INFO);

    if (g_fetch_data_threadsafe_callback == nullptr)
    {
        Logger::getInstance().log("fetch_data_callback_wrapper: g_fetch_data_threadsafe_callback is null",
                                  LogLevel::ERROR);
        return;
    }

    napi_call_threadsafe_function(g_fetch_data_threadsafe_callback, ctx.get(), napi_tsfn_blocking);

    Logger::getInstance().log("fetch_data_callback_wrapper after napi_call_threadsafe_function", LogLevel::DEBUG);

    {
        std::unique_lock<std::mutex> lock(ctx->mtx);
        while (!ctx->ready)
        {
            ctx->cv.wait(lock);
        }
    }

    Logger::getInstance().log("Hydration finish in fetch_data_callback_wrapper", LogLevel::INFO);

    RemoveTransferContext(ctx->transferKey);
}
