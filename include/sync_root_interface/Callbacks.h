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

typedef void (*NAPI_CALLBACK_FUNCTION)(napi_env);

struct InputSyncCallbacks {
    NAPI_CALLBACK_FUNCTION fetchDataCallback;
    NAPI_CALLBACK_FUNCTION validateDataCallback;
    NAPI_CALLBACK_FUNCTION cancelFetchDataCallback;
    NAPI_CALLBACK_FUNCTION fetchPlaceholdersCallback;
    NAPI_CALLBACK_FUNCTION cancelFetchPlaceholdersCallback;
    NAPI_CALLBACK_FUNCTION notifyFileOpenCompletionCallback;
    NAPI_CALLBACK_FUNCTION notifyFileCloseCompletionCallback;
    NAPI_CALLBACK_FUNCTION notifyDehydrateCallback;
    NAPI_CALLBACK_FUNCTION notifyDehydrateCompletionCallback;
    NAPI_CALLBACK_FUNCTION notifyDeleteCallback;
    NAPI_CALLBACK_FUNCTION notifyDeleteCompletionCallback;
    NAPI_CALLBACK_FUNCTION notifyRenameCallback;
    NAPI_CALLBACK_FUNCTION notifyRenameCompletionCallback;
    NAPI_CALLBACK_FUNCTION noneCallback;
};

struct CallbackContext {
    napi_env env;
    InputSyncCallbacks callbacks;
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

// void CALLBACK DeleteDataNotificationCallback (
//     _In_ CONST CF_CALLBACK_INFO* callbackInfo,
//     _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
// );

// void CALLBACK FetchDataCallback (
//     _In_ CONST CF_CALLBACK_INFO* callbackInfo,
//     _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
// );

// void CALLBACK FetchPlaceholdersCallback (
//     _In_ CONST CF_CALLBACK_INFO* callbackInfo,
//     _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
// );