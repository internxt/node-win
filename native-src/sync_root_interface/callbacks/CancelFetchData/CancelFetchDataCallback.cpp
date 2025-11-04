#include <stdafx.h>
#include <Callbacks.h>
#include <Logger.h>
#include <cfapi.h>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <filesystem>

napi_threadsafe_function g_cancel_fetch_data_threadsafe_callback = nullptr;

struct CallbackContext
{
    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false;
};

struct CancelFetchDataArgs
{
    std::wstring fileIdentityArg;
    CallbackContext *context;

    CancelFetchDataArgs(const std::wstring &fileId, CallbackContext *ctx)
        : fileIdentityArg(fileId), context(ctx) {}
};

void notify_cancel_fetch_data_call(napi_env env, napi_value js_callback, void *context, void *data)
{
    CancelFetchDataArgs *args = static_cast<CancelFetchDataArgs *>(data);
    std::u16string u16_fileIdentity(args->fileIdentityArg.begin(), args->fileIdentityArg.end());

    napi_value js_string;
    napi_create_string_utf16(env, u16_fileIdentity.c_str(), u16_fileIdentity.size(), &js_string);

    napi_value args_to_js_callback_cancel_fetch[1] = {js_string};

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_value result;

    Logger::getInstance().log("Executed to call JS function in cancelFetchCallback.", LogLevel::ERROR);
    napi_status status = napi_call_function(env, undefined, js_callback, 1, args_to_js_callback_cancel_fetch, &result);
    if (status != napi_ok)
    {
        fprintf(stderr, "Failed to call JS function.\n");
        Logger::getInstance().log("Failed to call JS function in cancelFetchCallback.", LogLevel::ERROR);
    }

    {
        std::lock_guard<std::mutex> lock(args->context->mtx);
        args->context->ready = true;
    }

    args->context->cv.notify_one();
    delete args;
}

void register_threadsafe_cancel_fetch_data_callback(const std::string &resource_name, napi_env env, InputSyncCallbacks input)
{
    std::u16string converted_resource_name(resource_name.begin(), resource_name.end());

    napi_value resource_name_value;
    napi_create_string_utf16(env, converted_resource_name.c_str(), NAPI_AUTO_LENGTH, &resource_name_value);

    napi_threadsafe_function tsfn_cancel_fetch_data;
    napi_value cancel_fetch_data_value;
    napi_get_reference_value(env, input.cancel_fetch_data_callback_ref, &cancel_fetch_data_value);

    napi_status status = napi_create_threadsafe_function(
        env,
        cancel_fetch_data_value,
        NULL,
        resource_name_value,
        0,
        1,
        NULL,
        NULL,
        NULL,
        notify_cancel_fetch_data_call,
        &tsfn_cancel_fetch_data);

    if (status != napi_ok)
    {
        napi_throw_error(env, nullptr, "Failed to create cancel fetch data threadsafe function");
        return;
    }

    g_cancel_fetch_data_threadsafe_callback = tsfn_cancel_fetch_data;
}

void CALLBACK cancel_fetch_data_callback_wrapper(
    _In_ CONST CF_CALLBACK_INFO *callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS *callbackParameters)
{
    printf("cancel_fetch_data_callback_wrapper called\n");

    LPCVOID fileIdentity = callbackInfo->FileIdentity;
    DWORD fileIdentityLength = callbackInfo->FileIdentityLength;

    const wchar_t *wchar_ptr = static_cast<const wchar_t *>(fileIdentity);
    std::wstring fileIdentityStr(wchar_ptr, fileIdentityLength / sizeof(wchar_t));

    if (g_cancel_fetch_data_threadsafe_callback == nullptr)
    {
        wprintf(L"Callback fetch_data_callback_wrapper called but g_fetch_data_threadsafe_callback is null\n");
        return;
    }

    CallbackContext context;
    CancelFetchDataArgs *args = new CancelFetchDataArgs(fileIdentityStr, &context);

    napi_call_threadsafe_function(g_cancel_fetch_data_threadsafe_callback, args, napi_tsfn_blocking);

    {
        std::unique_lock<std::mutex> lock(context.mtx);
        auto timeout = std::chrono::seconds(30);

        if (context.cv.wait_for(lock, timeout, [&context]
                                { return context.ready; }))
        {
            wprintf(L"Cancel fetch completed\n");
        }
        else
        {
            wprintf(L"Cancel fetch timed out\n");
        }
    }
}
