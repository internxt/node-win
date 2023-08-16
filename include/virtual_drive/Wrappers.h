#include <node_api.h>

napi_value CreatePlaceholderFile(napi_env env, napi_callback_info args);
napi_value UnregisterSyncRootWrapper(napi_env env, napi_callback_info args);
napi_value RegisterSyncRootWrapper(napi_env env, napi_callback_info args);
napi_value ConnectSyncRootWrapper(napi_env env, napi_callback_info args);
napi_value WatchAndWaitWrapper(napi_env env, napi_callback_info args);

typedef void (*NAPI_CALLBACK_FUNCTION)(napi_env, napi_value);

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