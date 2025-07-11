#pragma once

#include <CallbacksContext.h>

void register_threadsafe_callbacks(napi_env env, InputSyncCallbacks input);

// Fetch Data Callback
void register_threadsafe_fetch_data_callback(const std::string &resource_name, napi_env env, InputSyncCallbacks input);
void CALLBACK fetch_data_callback_wrapper(_In_ CONST CF_CALLBACK_INFO *callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS *callbackParameters);

// Fetch Data Cancel Callback
void register_threadsafe_cancel_fetch_data_callback(const std::string &resource_name, napi_env env, InputSyncCallbacks input);
void CALLBACK cancel_fetch_data_callback_wrapper(_In_ CONST CF_CALLBACK_INFO *callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS *callbackParameters);
