#include <Callbacks.h>
#include <string>

void RegisterThreadSafeCallback(const std::string& resource_name, CallbackContext* context) {
    std::u16string converted_resource_name = std::u16string(resource_name.begin(), resource_name.end());

    napi_env env = context->env;

    napi_value resource_name_value;
    napi_create_string_utf16(env, converted_resource_name.c_str(), NAPI_AUTO_LENGTH, &resource_name_value);

    napi_value notify_delete_callback;
    napi_status status_ref = napi_get_reference_value(env, context->callbacks.notify_delete_callback_ref, &notify_delete_callback);

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
        NotifyDeleteCall,
        &context->threadsafe_callbacks.notify_delete_callback
    );
}