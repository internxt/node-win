#include <Callbacks.h>
#include <string>
#include <condition_variable>
#include <mutex>

napi_threadsafe_function g_notify_close_completion_threadsafe_callback = nullptr;

void setup_global_tsfn_close(napi_threadsafe_function tsfn)
{
    g_notify_close_completion_threadsafe_callback = tsfn;
}

struct NotifyDeleteArgs
{
    std::wstring targetPathArg;
    std::wstring fileIdentityArg;
};

void notify_close_completion_call(napi_env env, napi_value js_callback, void *context, void *data)
{
    std::wstring *receivedData = static_cast<std::wstring *>(data);
    napi_value js_string;
    napi_create_string_utf16(env, reinterpret_cast<const char16_t *>(receivedData->c_str()), receivedData->size(), &js_string);

    napi_value args_to_js_callback[1] = {js_string};

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_value result;

    napi_status status = napi_call_function(env, undefined, js_callback, 2, args_to_js_callback, &result);
    if (status != napi_ok)
    {
        fprintf(stderr, "Failed to call JS function.\n");
        return;
    }

    delete receivedData;
}

void register_threadsafe_notify_close_completion_callback(const std::string &resource_name, napi_env env, InputSyncCallbacks input)
{
    std::u16string converted_resource_name = std::u16string(resource_name.begin(), resource_name.end());

    napi_value resource_name_value;
    napi_create_string_utf16(env, converted_resource_name.c_str(), NAPI_AUTO_LENGTH, &resource_name_value);

    napi_threadsafe_function notify_close_completion_threadsafe_callback;
    napi_value notify_close_completion_callback;
    napi_status status_ref = napi_get_reference_value(env, input.notify_file_open_completion_callback_ref, &notify_close_completion_callback);

    if (notify_close_completion_callback == nullptr)
    {
        fprintf(stderr, "notify_close_completion_callback is null\n");
        return;
    }

    napi_status status = napi_create_threadsafe_function(
        env,
        notify_close_completion_callback,
        NULL,
        resource_name_value,
        0,
        1,
        NULL,
        NULL,
        NULL,
        notify_close_completion_call,
        &notify_close_completion_threadsafe_callback);

    if (status != napi_ok)
    {
        const napi_extended_error_info *errorInfo = NULL;
        napi_get_last_error_info(env, &errorInfo);
        fprintf(stderr, "Failed to create threadsafe function: %s\n", errorInfo->error_message);
        fprintf(stderr, "N-API Status Code: %d\n", errorInfo->error_code);
        fprintf(stderr, "Engine-specific error code: %u\n", errorInfo->engine_error_code);
        abort();
    }

    setup_global_tsfn_close(notify_close_completion_threadsafe_callback);
}

void CALLBACK notify_close_completion_callback_wrapper(
    _In_ CONST CF_CALLBACK_INFO *callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS *callbackParameters)
{
    wprintf(L"Callback notify_close_completion_callback_wrapper called\n");
    LPCVOID fileIdentity = callbackInfo->FileIdentity;
    DWORD fileIdentityLength = callbackInfo->FileIdentityLength;

    const wchar_t *wchar_ptr = static_cast<const wchar_t *>(fileIdentity);
    std::wstring fileIdentityStr(wchar_ptr, fileIdentityLength / sizeof(wchar_t));

    std::wstring *dataToSend = new std::wstring(fileIdentityStr);

    napi_status status = napi_call_threadsafe_function(g_notify_close_completion_threadsafe_callback, dataToSend, napi_tsfn_blocking);

    if (status != napi_ok)
    {
        wprintf(L"Callback called unsuccessfully.\n");
    }

}