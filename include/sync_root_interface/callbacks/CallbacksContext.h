#pragma once

#include <Callbacks.h>


struct CallbackContext {
    napi_env env;
    InputSyncCallbacks callbacks;
    ThreadSafeCallbacksVersion threadsafe_callbacks_version;
};

class GlobalCallbackContextContainer {
    private:
        static CallbackContext* currentContext;

    public:
        static void SetContext(CallbackContext* context) {
            currentContext = context;
        }

        static CallbackContext* GetContext() {
            return currentContext;
        }

        static void ClearContext() {
            if (currentContext != nullptr) {
                delete currentContext;
                currentContext = nullptr;
            }
        }
    
};