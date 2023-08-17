#include <node_api.h>
#include "Wrappers.h"

napi_value init(napi_env env, napi_value exports) {
  // Watcher
  napi_property_descriptor watchAndWaitDesc = {
    "watchAndWait",             // nombre de la función en JS
    nullptr,
    WatchAndWaitWrapper,       // nombre de tu función en C++
    nullptr,
    nullptr,
    nullptr,
    napi_default,
    nullptr
  };

  napi_status defineWatchAndWaitStatus = napi_define_properties(env, exports, 1, &watchAndWaitDesc);
  if (defineWatchAndWaitStatus != napi_ok) {
      napi_throw_error(env, nullptr, "Failed to define WatchAndWait function");
      return nullptr;
  }
  
  //CreatePlaceholderFileWrapper
  napi_property_descriptor desc = {
    "createPlaceholderFile",
    nullptr,
    CreatePlaceholderFile,
    nullptr,
    nullptr,
    nullptr,
    napi_default,
    nullptr
  };

  napi_status defineStatus = napi_define_properties(env, exports, 1, &desc);
  if (defineStatus != napi_ok) {
    napi_throw_error(env, nullptr, "Failed to define function");
    return nullptr;
  }

  //UnregisterSyncRootWrapper
  napi_property_descriptor unregisterDesc = {
        "unregisterSyncRoot",
        nullptr,
        UnregisterSyncRootWrapper,
        nullptr,
        nullptr,
        nullptr,
        napi_default,
        nullptr
    };

  napi_status defineUnregisterStatus = napi_define_properties(env, exports, 1, &unregisterDesc);
  if (defineUnregisterStatus != napi_ok) {
      napi_throw_error(env, nullptr, "Failed to define UnregisterSyncRoot function");
      return nullptr;
  }

  //RegisterSyncRootWrapper
  napi_property_descriptor registerSyncRootDesc = {
    "registerSyncRoot",
    nullptr,
    RegisterSyncRootWrapper,
    nullptr,
    nullptr,
    nullptr,
    napi_default,
    nullptr
  };

  napi_status defineRegisterSyncRootStatus = napi_define_properties(env, exports, 1, &registerSyncRootDesc);
  if (defineRegisterSyncRootStatus != napi_ok) {
      napi_throw_error(env, nullptr, "Failed to define RegisterSyncRoot function");
      return nullptr;
  }
  
  //ConnectSyncRootWrapper
  napi_property_descriptor connectSyncRootDesc = {
    "connectSyncRoot",
    nullptr,
    ConnectSyncRootWrapper,
    nullptr,
    nullptr,
    nullptr,
    napi_default,
    nullptr
  };

  napi_status defineConnectSyncRootStatus = napi_define_properties(env, exports, 1, &connectSyncRootDesc);
  if (defineConnectSyncRootStatus != napi_ok) {
      napi_throw_error(env, nullptr, "Failed to define ConnectSyncRoot function");
      return nullptr;
  }


  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)