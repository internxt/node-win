void CALLBACK DeleteDataNotificationCallback (
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);

void CALLBACK FetchDataCallback (
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);

void CALLBACK FetchPlaceholdersCallback (
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);

typedef void (CALLBACK *CF_CALLBACK_FUNCTION)(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
);

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