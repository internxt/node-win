#pragma once

#include <CallbacksContext.h>

void register_threadsafe_callbacks(napi_env env, InputSyncCallbacks input);

// Notify Delete Callback
void register_threadsafe_notify_delete_callback(const std::string &resource_name, napi_env env, InputSyncCallbacks input);
void CALLBACK notify_delete_callback_wrapper(_In_ CONST CF_CALLBACK_INFO *callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS *callbackParameters);

// Rename Callback
void CALLBACK notify_rename_completion_callback_wrapper(_In_ CONST CF_CALLBACK_INFO *callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS *callbackParameters);
