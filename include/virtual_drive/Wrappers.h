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
napi_value addLoggerPathWrapper(napi_env env, napi_callback_info args);
napi_value UpdateSyncStatusWrapper(napi_env env, napi_callback_info args);