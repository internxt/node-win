#pragma once

#include <CallbacksContext.h>

class CallbackHandler
{
    public:
        static void RegisterThreadSafeCallbacks(CallbackContext* context);
};

// thread safe register callbacks
void RegisterThreadSafeNotifyDeleteCallback(const std::string& resource_name, CallbackContext* context);

// secure calls
void NotifyDeleteCall(napi_env env, napi_value js_callback, void* context, void* data);

// callback wrappers
void CALLBACK NotifyDeleteCallbackWrapper(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters);