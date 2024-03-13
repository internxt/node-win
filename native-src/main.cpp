#include <node_api.h>
#include "Wrappers.h"

napi_value init(napi_env env, napi_value exports)
{
  // Watcher
  napi_property_descriptor watchAndWaitDesc = {
      "watchAndWait", // nombre de la función en JS
      nullptr,
      WatchAndWaitWrapper, // nombre de tu función en C++
      nullptr,
      nullptr,
      nullptr,
      napi_default,
      nullptr};

  napi_status defineWatchAndWaitStatus = napi_define_properties(env, exports, 1, &watchAndWaitDesc);
  if (defineWatchAndWaitStatus != napi_ok)
  {
    napi_throw_error(env, nullptr, "Failed to define WatchAndWait function");
    return nullptr;
  }

  // CreatePlaceholderFileWrapper
  napi_property_descriptor desc = {
      "createPlaceholderFile",
      nullptr,
      CreatePlaceholderFile,
      nullptr,
      nullptr,
      nullptr,
      napi_default,
      nullptr};

  napi_status defineStatus = napi_define_properties(env, exports, 1, &desc);
  if (defineStatus != napi_ok)
  {
    napi_throw_error(env, nullptr, "Failed to define function");
    return nullptr;
  }

  // UnregisterSyncRootWrapper
  napi_property_descriptor unregisterDesc = {
      "unregisterSyncRoot",
      nullptr,
      UnregisterSyncRootWrapper,
      nullptr,
      nullptr,
      nullptr,
      napi_default,
      nullptr};

  napi_status defineUnregisterStatus = napi_define_properties(env, exports, 1, &unregisterDesc);
  if (defineUnregisterStatus != napi_ok)
  {
    napi_throw_error(env, nullptr, "Failed to define UnregisterSyncRoot function");
    return nullptr;
  }

  // RegisterSyncRootWrapper
  napi_property_descriptor registerSyncRootDesc = {
      "registerSyncRoot",
      nullptr,
      RegisterSyncRootWrapper,
      nullptr,
      nullptr,
      nullptr,
      napi_default,
      nullptr};

  napi_status defineRegisterSyncRootStatus = napi_define_properties(env, exports, 1, &registerSyncRootDesc);
  if (defineRegisterSyncRootStatus != napi_ok)
  {
    napi_throw_error(env, nullptr, "Failed to define RegisterSyncRoot function");
    return nullptr;
  }

  // ConnectSyncRootWrapper
  napi_property_descriptor connectSyncRootDesc = {
      "connectSyncRoot",
      nullptr,
      ConnectSyncRootWrapper,
      nullptr,
      nullptr,
      nullptr,
      napi_default,
      nullptr};

  napi_status defineConnectSyncRootStatus = napi_define_properties(env, exports, 1, &connectSyncRootDesc);
  if (defineConnectSyncRootStatus != napi_ok)
  {
    napi_throw_error(env, nullptr, "Failed to define ConnectSyncRoot function");
    return nullptr;
  }

  napi_property_descriptor createEntryDesc = {
      "createEntry",
      nullptr,
      CreateEntryWrapper,
      nullptr,
      nullptr,
      nullptr,
      napi_default,
      nullptr};

  napi_status defineCreateEntryStatus = napi_define_properties(env, exports, 1, &createEntryDesc);
  if (defineCreateEntryStatus != napi_ok)
  {
    napi_throw_error(env, nullptr, "Failed to define createEntry function");
    return nullptr;
  }

  // disconection
  napi_property_descriptor disconnectDesc = {
      "disconnectSyncRoot",
      nullptr,
      DisconnectSyncRootWrapper,
      nullptr,
      nullptr,
      nullptr,
      napi_default,
      nullptr};

  napi_status defineDisconnectStatus = napi_define_properties(env, exports, 1, &disconnectDesc);
  if (defineDisconnectStatus != napi_ok)
  {
    napi_throw_error(env, nullptr, "Failed to define DisconnectSyncRoot function");
    return nullptr;
  }

  napi_property_descriptor getItemsIdsDesc{
      "getItemsIds",
      nullptr,
      GetItemsSyncRootWrapper,
      nullptr,
      nullptr,
      nullptr,
      napi_default,
      nullptr};

  napi_status defineGetItemsStatus = napi_define_properties(env, exports, 1, &getItemsIdsDesc);
  if (defineGetItemsStatus != napi_ok)
  {
    napi_throw_error(env, nullptr, "Failed to define GetItemsSyncRoot function");
    return nullptr;
  }

  napi_property_descriptor addLoggerPathDesc = {
      "addLoggerPath",
      nullptr,
      addLoggerPathWrapper,
      nullptr,
      nullptr,
      nullptr,
      napi_default,
      nullptr};

  napi_status defineAddLoggerPathStatus = napi_define_properties(env, exports, 1, &addLoggerPathDesc);
  if (defineAddLoggerPathStatus != napi_ok)
  {
    napi_throw_error(env, nullptr, "Failed to define addLoggerPath function");
    return nullptr;
  }

  napi_property_descriptor updateSyncStatusDesc = {
      "updateSyncStatus",
      nullptr,
      UpdateSyncStatusWrapper,
      nullptr,
      nullptr,
      nullptr,
      napi_default,
      nullptr};

  napi_status updateSyncStatusStatus = napi_define_properties(env, exports, 1, &updateSyncStatusDesc);
  if (updateSyncStatusStatus != napi_ok)
  {
    napi_throw_error(env, nullptr, "Failed to define updateSyncStatus function");
    return nullptr;
  }

  napi_property_descriptor getPlaceholderStateDesc = {
      "getPlaceholderState",
      nullptr,
      GetPlaceholderStateWrapper,
      nullptr,
      nullptr,
      nullptr,
      napi_default,
      nullptr};

  napi_status defineGetPlaceholderStateStatus = napi_define_properties(env, exports, 1, &getPlaceholderStateDesc);
  if (defineGetPlaceholderStateStatus != napi_ok)
  {
    napi_throw_error(env, nullptr, "Failed to define getPlaceholderState function");
    return nullptr;
  }

  napi_property_descriptor getPlaceholderWithStatePendingDesc = {
      "getPlaceholderWithStatePending",
      nullptr,
      GetPlaceholderWithStatePendingWrapper,
      nullptr,
      nullptr,
      nullptr,
      napi_default,
      nullptr};

  napi_status defineGetPlaceholderWithStatePendingStatus = napi_define_properties(env, exports, 1, &getPlaceholderWithStatePendingDesc);
  if (defineGetPlaceholderWithStatePendingStatus != napi_ok)
  {
    napi_throw_error(env, nullptr, "Failed to define getPlaceholderWithStatePending function");
    return nullptr;
  }

  napi_property_descriptor convertToPlaceholderDesc = {
      "convertToPlaceholder",
      nullptr,
      ConvertToPlaceholderWrapper,
      nullptr,
      nullptr,
      nullptr,
      napi_default,
      nullptr};

  napi_status convertToPlaceholderStatus = napi_define_properties(env, exports, 1, &convertToPlaceholderDesc);
  if (convertToPlaceholderStatus != napi_ok)
  {
    napi_throw_error(env, nullptr, "Failed to define convertToPlaceholder function");
    return nullptr;
  }

  napi_property_descriptor closeMutexDesc = {
      "closeMutex",
      nullptr,
      CloseMutexWrapper,
      nullptr,
      nullptr,
      nullptr,
      napi_default,
      nullptr};

  napi_status closeMutexStatus = napi_define_properties(env, exports, 1, &closeMutexDesc);
  if (closeMutexStatus != napi_ok)
  {
    napi_throw_error(env, nullptr, "Failed to define closeMutex function");
    return nullptr;
  }

  return exports;
}
NAPI_MODULE(NODE_GYP_MODULE_NAME, init)