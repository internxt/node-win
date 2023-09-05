#include <Callbacks.h>
#include <string>

napi_threadsafe_function g_notify_rename_threadsafe_callback = nullptr;


struct NotifyRenameArgs {
    std::wstring targetPathArg;
    std::wstring fileIdentityArg;
};

void notify_rename_call(napi_env env, napi_value js_callback, void* context, void* data) {
    NotifyRenameArgs* args = static_cast<NotifyRenameArgs*>(data);

    // Convierte los wstrings a u16strings
    std::u16string u16_targetPath(args->targetPathArg.begin(), args->targetPathArg.end());
    std::u16string u16_fileIdentity(args->fileIdentityArg.begin(), args->fileIdentityArg.end());

    // Convierte los u16strings a napi_value (probablemente cadenas de JS)
    napi_value js_targetPathArg, js_fileIdentityArg;
    
    napi_create_string_utf16(env, u16_targetPath.c_str(), u16_targetPath.size(), &js_targetPathArg);
    napi_create_string_utf16(env, u16_fileIdentity.c_str(), u16_fileIdentity.size(), &js_fileIdentityArg);

    napi_value args_to_js_callback[2];
    args_to_js_callback[0] = js_targetPathArg;
    args_to_js_callback[1] = js_fileIdentityArg;

    napi_value undefined, result;
    napi_get_undefined(env, &undefined);
    napi_call_function(env, undefined, js_callback, 2, args_to_js_callback, &result);

    delete args;
}

void setup_global_tsfn_rename(napi_threadsafe_function tsfn) {
    g_notify_rename_threadsafe_callback = tsfn;
}

void register_threadsafe_notify_rename_callback(const std::string& resource_name, napi_env env, InputSyncCallbacks input) {
    std::u16string converted_resource_name = std::u16string(resource_name.begin(), resource_name.end());

    napi_value resource_name_value;
    napi_create_string_utf16(env, converted_resource_name.c_str(), NAPI_AUTO_LENGTH, &resource_name_value);

    napi_threadsafe_function notify_rename_threadsafe_callback;
    napi_value notify_rename_value;
    napi_status status_ref = napi_get_reference_value(env, input.notify_rename_callback_ref, &notify_rename_value);

    napi_valuetype valuetype;
    napi_status type_status = napi_typeof(env, notify_rename_value, &valuetype);

    if (type_status != napi_ok || valuetype != napi_function) {
        fprintf(stderr, "notify_rename_value is not a function.\n");
        abort();
    }

    napi_status status = napi_create_threadsafe_function(
        env,
        notify_rename_value,
        NULL,
        resource_name_value,
        0,
        1,
        NULL,
        NULL,
        NULL,
        notify_rename_call,
        &notify_rename_threadsafe_callback
    );

    if (status != napi_ok) {
        const napi_extended_error_info* errorInfo = NULL;
        napi_get_last_error_info(env, &errorInfo);
        fprintf(stderr, "Failed to create threadsafe function: %s\n", errorInfo->error_message);
        fprintf(stderr, "N-API Status Code: %d\n", errorInfo->error_code);
        fprintf(stderr, "Engine-specific error code: %u\n", errorInfo->engine_error_code);
        abort();
    }

    setup_global_tsfn_rename(notify_rename_threadsafe_callback);
}

void CALLBACK notify_rename_callback_wrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {
    LPCVOID fileIdentity = callbackInfo->FileIdentity;
    DWORD fileIdentityLength = callbackInfo->FileIdentityLength;

    const wchar_t* wchar_ptr = static_cast<const wchar_t*>(fileIdentity);
    std::wstring fileIdentityStr(wchar_ptr, fileIdentityLength / sizeof(wchar_t));

    PCWSTR targetPathArg = callbackParameters->Rename.TargetPath;

    NotifyRenameArgs* args = new NotifyRenameArgs();
    args->targetPathArg = std::wstring(targetPathArg);
    args->fileIdentityArg = fileIdentityStr;

    napi_status status = napi_call_threadsafe_function(g_notify_rename_threadsafe_callback, args, napi_tsfn_blocking);
    
    if (status != napi_ok) {
        wprintf(L"Callback called unsuccessfully.\n");
    };

    CF_OPERATION_PARAMETERS opParams = {0};
    opParams.AckDelete.CompletionStatus = STATUS_SUCCESS;
    opParams.ParamSize = sizeof(CF_OPERATION_PARAMETERS);

    CF_OPERATION_INFO opInfo = {0};
    opInfo.StructSize = sizeof(CF_OPERATION_INFO);
    opInfo.Type = CF_OPERATION_TYPE_ACK_RENAME;
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