#pragma once

#include <node_api.h>

napi_value CreateFilePlaceholderWrapper(napi_env env, napi_callback_info args);
napi_value CreateFolderPlaceholderWrapper(napi_env env, napi_callback_info args);
napi_value UnregisterSyncRootWrapper(napi_env env, napi_callback_info args);
napi_value RegisterSyncRootWrapper(napi_env env, napi_callback_info args);
napi_value GetRegisteredSyncRootsWrapper(napi_env env, napi_callback_info args);
napi_value ConnectSyncRootWrapper(napi_env env, napi_callback_info args);
napi_value DisconnectSyncRootWrapper(napi_env env, napi_callback_info args);
napi_value addLoggerPathWrapper(napi_env env, napi_callback_info args);
napi_value UpdateSyncStatusWrapper(napi_env env, napi_callback_info args);
napi_value GetPlaceholderStateWrapper(napi_env env, napi_callback_info args);
napi_value ConvertToPlaceholderWrapper(napi_env env, napi_callback_info args);
napi_value DehydrateFileWrapper(napi_env env, napi_callback_info args);
napi_value HydrateFileWrapper(napi_env env, napi_callback_info args);
