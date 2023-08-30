#pragma once

#include <node_api.h>
#include <iostream>

struct InputSyncCallbacks {
    napi_ref fetchDataCallbackRef;
    napi_ref validateDataCallbackRef;
    napi_ref cancelFetchDataCallbackRef;
    napi_ref fetchPlaceholdersCallbackRef;
    napi_ref cancelFetchPlaceholdersCallbackRef;
    napi_ref notifyFileOpenCompletionCallbackRef;
    napi_ref notifyFileCloseCompletionCallbackRef;
    napi_ref notifyDehydrateCallbackRef;
    napi_ref notifyDehydrateCompletionCallbackRef;
    napi_ref notifyDeleteCallbackRef;
    napi_ref notifyDeleteCompletionCallbackRef;
    napi_ref notifyRenameCallbackRef;
    napi_ref notifyRenameCompletionCallbackRef;
    napi_ref noneCallbackRef;
};

typedef void (CALLBACK *CF_CALLBACK_FUNCTION)(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);

struct SyncCallbacks {
    CF_CALLBACK_FUNCTION fetchDataCallback;
    CF_CALLBACK_FUNCTION validateDataCallback;
    CF_CALLBACK_FUNCTION cancelFetchDataCallback;
    CF_CALLBACK_FUNCTION fetchPlaceholdersCallback;
    CF_CALLBACK_FUNCTION cancelFetchPlaceholdersCallback;
    CF_CALLBACK_FUNCTION notifyFileOpenCompletionCallback;
    CF_CALLBACK_FUNCTION notifyFileCloseCompletionCallback;
    CF_CALLBACK_FUNCTION notifyDehydrateCallback;
    CF_CALLBACK_FUNCTION notifyDehydrateCompletionCallback;
    CF_CALLBACK_FUNCTION notifyDeleteCallback;
    CF_CALLBACK_FUNCTION notifyDeleteCompletionCallback;
    CF_CALLBACK_FUNCTION notifyRenameCallback;
    CF_CALLBACK_FUNCTION notifyRenameCompletionCallback;
    CF_CALLBACK_FUNCTION noneCallback;
};

struct CallbacksWorks {
    napi_async_work fetchDataCallbackWork;
    napi_async_work validateDataCallbackWork;
    napi_async_work cancelFetchDataCallbackWork;
    napi_async_work fetchPlaceholdersCallbackWork;
    napi_async_work cancelFetchPlaceholdersCallbackWork;
    napi_async_work notifyFileOpenCompletionCallbackWork;
    napi_async_work notifyFileCloseCompletionCallbackWork;
    napi_async_work notifyDehydrateCallbackWork;
    napi_async_work notifyDehydrateCompletionCallbackWork;
    napi_async_work notifyDeleteCallbackWork;
    napi_async_work notifyDeleteCompletionCallbackWork;
    napi_async_work notifyRenameCallbackWork;
    napi_async_work notifyRenameCompletionCallbackWork;
    napi_async_work noneCallbackWork;
};

struct CallbackArgs {
    //NotifyDeleteCompletionArgs notifyDeleteCompletionArgs;
    //NotifyRenameArgs notifyRenameArgs;
};

struct CallbackContext {
    napi_env env;
    CallbackArgs callbackArgs;
    InputSyncCallbacks callbacks;
    napi_async_context async_context;
    napi_ref async_resource_ref;
    CallbacksWorks callbacksWorks; 
};

class GlobalContextContainer {
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
        if (currentContext) {
            if (currentContext->env && currentContext->async_resource_ref) {
                napi_delete_reference(currentContext->env, currentContext->async_resource_ref);
            }
            if (currentContext->env && currentContext->async_context) {
                napi_async_destroy(currentContext->env, currentContext->async_context);
            }

            delete currentContext;
            currentContext = nullptr;
        }
    }
};