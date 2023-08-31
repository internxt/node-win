#pragma once

#include <CallbacksBase.h>
#include <CallbacksContext.h>

class CallbackHandler
{
    public:
        static void RegisterThreadSafeCallbacks(napi_env env, InputSyncCallbacks input);
};

void RegisterThreadSafeCalback(napi_env env, InputSyncCallbacks input);
void RegisterThreadSafeNotifyDeleteCallback(napi_env env, InputSyncCallbacks input);
void RegisterThreadSafeNotifyRenameCallback(napi_env env, InputSyncCallbacks input);