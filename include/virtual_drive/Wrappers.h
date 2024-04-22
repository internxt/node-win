#pragma once

#include <node_api.h>

napi_value CreatePlaceholderFile(napi_env env, napi_callback_info args);
napi_value UnregisterSyncRootWrapper(napi_env env, napi_callback_info args);
napi_value RegisterSyncRootWrapper(napi_env env, napi_callback_info args);
napi_value ConnectSyncRootWrapper(napi_env env, napi_callback_info args);
napi_value WatchAndWaitWrapper(napi_env env, napi_callback_info args);
napi_value CreateEntryWrapper(napi_env env, napi_callback_info args);
napi_value DisconnectSyncRootWrapper(napi_env env, napi_callback_info args);
napi_value GetItemsSyncRootWrapper(napi_env env, napi_callback_info args);
napi_value GetFileIdentityWrapper(napi_env env, napi_callback_info args);
napi_value addLoggerPathWrapper(napi_env env, napi_callback_info args);
napi_value UpdateSyncStatusWrapper(napi_env env, napi_callback_info args);
napi_value GetPlaceholderStateWrapper(napi_env env, napi_callback_info args);
napi_value GetPlaceholderWithStatePendingWrapper(napi_env env, napi_callback_info args);
napi_value ConvertToPlaceholderWrapper(napi_env env, napi_callback_info args);
napi_value CloseMutexWrapper(napi_env env, napi_callback_info args);
napi_value DeleteFileSyncRootWrapper(napi_env env, napi_callback_info args);
napi_value UpdateFileIdentityWrapper(napi_env env, napi_callback_info args);
