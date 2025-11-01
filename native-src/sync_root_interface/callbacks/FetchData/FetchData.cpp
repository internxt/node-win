#include "stdafx.h"
#include <Callbacks.h>
#include <string>
#include <condition_variable>
#include <mutex>
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
#include "napi_safe_wrap.h"

napi_threadsafe_function g_fetch_data_threadsafe_callback = nullptr;

#define FIELD_SIZE(type, field) (sizeof(((type *)0)->field))
#define CF_SIZE_OF_OP_PARAM(field) (FIELD_OFFSET(CF_OPERATION_PARAMETERS, field) + FIELD_SIZE(CF_OPERATION_PARAMETERS, field))

HRESULT transfer_data(
    _In_ CF_CONNECTION_KEY connectionKey,
    _In_ LARGE_INTEGER transferKey,
    _In_reads_bytes_opt_(length.QuadPart) LPCVOID transferData,
    _In_ LARGE_INTEGER startingOffset,
    _In_ LARGE_INTEGER length,
    _In_ NTSTATUS completionStatus)
{
    CF_OPERATION_INFO opInfo = {0};
    opInfo.StructSize = sizeof(opInfo);
    opInfo.Type = CF_OPERATION_TYPE_TRANSFER_DATA;
    opInfo.ConnectionKey = connectionKey;
    opInfo.TransferKey = transferKey;

    CF_OPERATION_PARAMETERS opParams = {0};
    opParams.ParamSize = CF_SIZE_OF_OP_PARAM(TransferData);
    opParams.TransferData.CompletionStatus = completionStatus;
    opParams.TransferData.Buffer = transferData;
    opParams.TransferData.Offset = startingOffset;
    opParams.TransferData.Length = length;

    return CfExecute(&opInfo, &opParams);
}

napi_value response_callback_fn_fetch_data(napi_env env, napi_callback_info info)
{
    size_t argc = 3;
    napi_value args[3];
    TransferContext *ctx = nullptr;
    napi_get_cb_info(env, info, &argc, args, nullptr, reinterpret_cast<void **>(&ctx));

    bool finished;
    napi_get_value_bool(env, args[0], &finished);

    if (finished)
    {
        wprintf(L"Hydration finished\n");

        CfSetPinState(handleForPath(ctx->path.c_str()).get(), CF_PIN_STATE_PINNED, CF_SET_PIN_FLAG_NONE, nullptr);

        std::lock_guard<std::mutex> lock(ctx->mtx);
        ctx->ready = true;
        ctx->cv.notify_one();

        return nullptr;
    }

    void *data;
    size_t length;
    napi_get_buffer_info(env, args[1], &data, &length);

    int64_t offset;
    napi_get_value_int64(env, args[2], &offset);

    LARGE_INTEGER startingOffset, chunkSize;
    startingOffset.QuadPart = offset;
    chunkSize.QuadPart = length;

    HRESULT hr = transfer_data(ctx->connectionKey, ctx->transferKey, data, startingOffset, chunkSize, STATUS_SUCCESS);

    if (FAILED(hr))
    {
        transfer_data(ctx->connectionKey, ctx->transferKey, nullptr, ctx->requiredOffset, ctx->requiredLength, STATUS_UNSUCCESSFUL);
        winrt::throw_hresult(hr);
    }

    size_t completed = offset + length;

    Utilities::ApplyTransferStateToFile(ctx->path, ctx->callbackInfo, ctx->fileSize.QuadPart, completed);

    wprintf(L"Bytes completed: %d bytes\n", completed);

    return nullptr;
}

napi_value response_callback_fn_fetch_data_wrapper(napi_env env, napi_callback_info info)
{
    return NAPI_SAFE_WRAP(env, info, response_callback_fn_fetch_data);
}

void notify_fetch_data_call(napi_env env, napi_value js_callback, void *context, void *data)
{
    TransferContext *ctx = static_cast<TransferContext *>(data);

    napi_value js_path;
    napi_create_string_utf16(env, (char16_t *)ctx->path.c_str(), ctx->path.length(), &js_path);

    napi_value js_callback;
    napi_create_function(env, "callback", NAPI_AUTO_LENGTH, response_callback_fn_fetch_data_wrapper, ctx, &js_callback);

    napi_value args_to_js_callback[2] = {js_path, js_callback};

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
