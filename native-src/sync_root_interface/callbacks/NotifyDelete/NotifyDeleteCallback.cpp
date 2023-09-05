#include <Callbacks.h>
#include <string>

napi_threadsafe_function g_notify_delete_threadsafe_callback = nullptr;
struct NotifyDeleteArgs {
    std::wstring targetPathArg;
    std::wstring fileIdentityArg;
};

void notify_delete_call(napi_env env, napi_value js_callback, void* context, void* data) {
    std::wstring* receivedData = static_cast<std::wstring*>(data);
    napi_value js_string;
    napi_create_string_utf16(env, reinterpret_cast<const char16_t*>(receivedData->c_str()), receivedData->size(), &js_string);

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_value result;
    napi_call_function(env, undefined, js_callback, 1, &js_string, &result);

    delete receivedData;
}

void setup_global_tsfn_delete(napi_threadsafe_function tsfn) {
    g_notify_delete_threadsafe_callback = tsfn;
}

void register_threadsafe_notify_delete_callback(const std::string& resource_name, napi_env env, InputSyncCallbacks input) {

    std::u16string converted_resource_name = std::u16string(resource_name.begin(), resource_name.end());

    napi_value resource_name_value;
    napi_create_string_utf16(env, converted_resource_name.c_str(), NAPI_AUTO_LENGTH, &resource_name_value);

    napi_threadsafe_function notify_delete_threadsafe_callback;
    napi_value notify_delete_callback;
    napi_status status_ref = napi_get_reference_value(env, input.notify_delete_callback_ref, &notify_delete_callback);

    if (notify_delete_callback == nullptr) {
        fprintf(stderr, "notify_delete_callback is null\n");
        return;
    }

    napi_status status = napi_create_threadsafe_function(
        env,
        notify_delete_callback,
        NULL,
        resource_name_value,
        0,
        1,
        NULL,
        NULL,
        NULL,
        notify_delete_call,
        &notify_delete_threadsafe_callback
    );

    if (status != napi_ok) {
        const napi_extended_error_info* errorInfo = NULL;
        napi_get_last_error_info(env, &errorInfo);
        fprintf(stderr, "Failed to create threadsafe function: %s\n", errorInfo->error_message);
        fprintf(stderr, "N-API Status Code: %d\n", errorInfo->error_code);
        fprintf(stderr, "Engine-specific error code: %u\n", errorInfo->engine_error_code);
        abort();
    }

    setup_global_tsfn_delete(notify_delete_threadsafe_callback);
}

void CALLBACK notify_delete_callback_wrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {
    LPCVOID fileIdentity = callbackInfo->FileIdentity;
    DWORD fileIdentityLength = callbackInfo->FileIdentityLength;

    const wchar_t* wchar_ptr = static_cast<const wchar_t*>(fileIdentity);
    std::wstring fileIdentityStr(wchar_ptr, fileIdentityLength / sizeof(wchar_t));

    std::wstring* dataToSend = new std::wstring(fileIdentityStr);

    napi_status status = napi_call_threadsafe_function(g_notify_delete_threadsafe_callback, dataToSend, napi_tsfn_blocking);
    
    if (status != napi_ok) {
        wprintf(L"Callback called unsuccessfully.\n");
    }

    CF_OPERATION_PARAMETERS opParams = {0};
    opParams.AckDelete.CompletionStatus = STATUS_SUCCESS;
    opParams.ParamSize = sizeof(CF_OPERATION_PARAMETERS);

    CF_OPERATION_INFO opInfo = {0};
    opInfo.StructSize = sizeof(CF_OPERATION_INFO);
    opInfo.Type = CF_OPERATION_TYPE_ACK_DELETE;
    opInfo.ConnectionKey = callbackInfo->ConnectionKey;
    opInfo.TransferKey = callbackInfo->TransferKey;

    HRESULT hr = CfExecute(
        &opInfo,
        &opParams
    );

    if (FAILED(hr))
    {
        wprintf(L"Error in CfExecute().\n");
        wprintf(L"Error in CfExecute(), HRESULT: %lx\n", hr);
    }
}