#include <node_api.h>
#include "Wrappers.h"

napi_value init(napi_env env, napi_value exports)
{
  napi_property_descriptor properties[] = {
      {"createFilePlaceholder", nullptr, CreateFilePlaceholderWrapper, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"unregisterSyncRoot", nullptr, UnregisterSyncRootWrapper, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"registerSyncRoot", nullptr, RegisterSyncRootWrapper, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"getRegisteredSyncRoots", nullptr, GetRegisteredSyncRootsWrapper, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"connectSyncRoot", nullptr, ConnectSyncRootWrapper, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"createFolderPlaceholder", nullptr, CreateFolderPlaceholderWrapper, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"disconnectSyncRoot", nullptr, DisconnectSyncRootWrapper, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"getFileIdentity", nullptr, GetFileIdentityWrapper, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"addLoggerPath", nullptr, addLoggerPathWrapper, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"updateSyncStatus", nullptr, UpdateSyncStatusWrapper, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"getPlaceholderState", nullptr, GetPlaceholderStateWrapper, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"convertToPlaceholder", nullptr, ConvertToPlaceholderWrapper, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"hydrateFile", nullptr, HydrateFileWrapper, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"dehydrateFile", nullptr, DehydrateFileWrapper, nullptr, nullptr, nullptr, napi_default, nullptr}};

  if (napi_define_properties(env, exports, 14, properties) != napi_ok)
  {
    napi_throw_error(env, nullptr, "Failed to define properties");
    return nullptr;
  }

  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)