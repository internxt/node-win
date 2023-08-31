#include <Callbacks.h>

void RegisterThreadSafeNotifyDeleteCallback(napi_env env, InputSyncCallbacks input) {
    napi_threadsafe_function threadsafe_function;
    napi_value callback;
    napi_status status_ref = napi_get_reference_value(env, input.notify_rename_callback_ref, &callback);

    napi_valuetype valuetype;
    napi_status type_status = napi_typeof(env, callback, &valuetype);

    if (type_status != napi_ok || valuetype != napi_function) {
        fprintf(stderr, "Thecallback is not a function.\n");
        abort();
    }

    napi_value resource_name;
    napi_create_string_utf8(env, "asyncWork", NAPI_AUTO_LENGTH, &resource_name);
    napi_status status = napi_create_threadsafe_function(
        env,
        callback,
        NULL,
        resource_name,
        0,
        1,
        NULL,
        NULL,
        NULL,
        NotifyDeleteCall,
        &threadsafe_function
    );

    if (status != napi_ok) {
        const napi_extended_error_info* errorInfo = NULL;
        napi_get_last_error_info(env, &errorInfo);
        fprintf(stderr, "Failed to create threadsafe function: %s\n", errorInfo->error_message);
        fprintf(stderr, "N-API Status Code: %d\n", errorInfo->error_code);
        fprintf(stderr, "Engine-specific error code: %u\n", errorInfo->engine_error_code);
        abort();
    }
}

struct NotifyDeleteArgs {
    std::wstring targetPathArg;
    std::wstring fileIdentityArg;
};

void NotifyDeleteCall(napi_env env, napi_value js_callback, void* context, void* data) {
  NotifyDeleteArgs* args = static_cast<NotifyDeleteArgs*>(data);

  std::u16string u16_targetPath(args->targetPathArg.begin(), args->targetPathArg.end());
  std::u16string u16_fileIdentity(args->fileIdentityArg.begin(), args->fileIdentityArg.end());

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