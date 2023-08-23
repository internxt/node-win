#pragma once

#include <node_api.h>

void CALLBACK ValidateDataCallbackWrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);

void CALLBACK CancelFetchDataCallbackWrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);

void CALLBACK FetchPlaceholdersCallbackWrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);

void CALLBACK CancelFetchPlaceholdersCallbackWrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);

void CALLBACK NotifyFileOpenCompletionCallbackWrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);

void CALLBACK NotifyFileCloseCompletionCallbackWrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);

void CALLBACK NotifyDehydrateCallbackWrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);

void CALLBACK NotifyDehydrateCompletionCallbackWrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);

void CALLBACK NotifyDeleteCallbackWrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);

void CALLBACK NotifyDeleteCompletionCallbackWrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);

void CALLBACK NotifyRenameCallbackWrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);

void CALLBACK NotifyRenameCompletionCallbackWrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);

void CALLBACK NoneCallbackWrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);

typedef void (CALLBACK *CF_CALLBACK_FUNCTION)(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);

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

struct CallbackContext {
    napi_env env;
    InputSyncCallbacks callbacks;
    napi_async_context async_context;
    napi_ref async_resource_ref;
    napi_async_work work; 
};


struct SCallbackContext {
    napi_env env;
    napi_ref callbackRef;
};

struct SyncCallbacks {
    CF_CALLBACK_FUNCTION fetchDataCallback;                          // CF_CALLBACK_TYPE_FETCH_DATA
    CF_CALLBACK_FUNCTION validateDataCallback;                       // CF_CALLBACK_TYPE_VALIDATE_DATA
    CF_CALLBACK_FUNCTION cancelFetchDataCallback;                    // CF_CALLBACK_TYPE_CANCEL_FETCH_DATA
    CF_CALLBACK_FUNCTION fetchPlaceholdersCallback;                  // CF_CALLBACK_TYPE_FETCH_PLACEHOLDERS
    CF_CALLBACK_FUNCTION cancelFetchPlaceholdersCallback;            // CF_CALLBACK_TYPE_CANCEL_FETCH_PLACEHOLDERS
    CF_CALLBACK_FUNCTION notifyFileOpenCompletionCallback;           // CF_CALLBACK_TYPE_NOTIFY_FILE_OPEN_COMPLETION
    CF_CALLBACK_FUNCTION notifyFileCloseCompletionCallback;          // CF_CALLBACK_TYPE_NOTIFY_FILE_CLOSE_COMPLETION
    CF_CALLBACK_FUNCTION notifyDehydrateCallback;                    // CF_CALLBACK_TYPE_NOTIFY_DEHYDRATE
    CF_CALLBACK_FUNCTION notifyDehydrateCompletionCallback;          // CF_CALLBACK_TYPE_NOTIFY_DEHYDRATE_COMPLETION
    CF_CALLBACK_FUNCTION notifyDeleteCallback;                       // CF_CALLBACK_TYPE_NOTIFY_DELETE
    CF_CALLBACK_FUNCTION notifyDeleteCompletionCallback;             // CF_CALLBACK_TYPE_NOTIFY_DELETE_COMPLETION
    CF_CALLBACK_FUNCTION notifyRenameCallback;                       // CF_CALLBACK_TYPE_NOTIFY_RENAME
    CF_CALLBACK_FUNCTION notifyRenameCompletionCallback;             // CF_CALLBACK_TYPE_NOTIFY_RENAME_COMPLETION
    CF_CALLBACK_FUNCTION noneCallback;                               // CF_CALLBACK_TYPE_NONE
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

void CALLBACK DeleteDataNotificationCallback (
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);

void DeleteDataNotificationCallbackThreadSafe(
    napi_env env, 
    napi_value js_cb, 
    void* context, 
    void* data
);

// void CALLBACK FetchDataCallback (
//     _In_ CONST CF_CALLBACK_INFO* callbackInfo,
//     _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
// );

// void CALLBACK FetchPlaceholdersCallback (
//     _In_ CONST CF_CALLBACK_INFO* callbackInfo,
//     _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
// );