#pragma once

#include <CallbacksContext.h>
#include "DirectoryWatcher.h"
#include "Semaphore.h"

void register_threadsafe_callbacks(napi_env env, InputSyncCallbacks input);

// Notify Delete Callback
void register_threadsafe_notify_delete_callback(const std::string &resource_name, napi_env env, InputSyncCallbacks input);
void CALLBACK notify_delete_callback_wrapper(_In_ CONST CF_CALLBACK_INFO *callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS *callbackParameters);

// Rename Callback
void register_threadsafe_notify_rename_callback(const std::string &resource_name, napi_env env, InputSyncCallbacks input);
void CALLBACK notify_rename_callback_wrapper(_In_ CONST CF_CALLBACK_INFO *callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS *callbackParameters);

// Fetch Placeholders Callback
void register_threadsafe_fetch_placeholders_callback(const std::string &resource_name, napi_env env, InputSyncCallbacks input);
void CALLBACK fetch_placeholders_callback_wrapper(_In_ CONST CF_CALLBACK_INFO *callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS *callbackParameters);

// Fetch Data Callback
void register_threadsafe_fetch_data_callback(const std::string &resource_name, napi_env env, InputSyncCallbacks input);
void CALLBACK fetch_data_callback_wrapper(_In_ CONST CF_CALLBACK_INFO *callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS *callbackParameters);

// Fetch Data Cancel Callback
void register_threadsafe_cancel_fetch_data_callback(const std::string &resource_name, napi_env env, InputSyncCallbacks input);
void CALLBACK cancel_fetch_data_callback_wrapper(_In_ CONST CF_CALLBACK_INFO *callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS *callbackParameters);

// Notify File Added Callback. This is a fake callback
void notify_file_added_call(napi_env env, napi_value js_callback, void *context, void *data);
void register_threadsafe_notify_file_added_callback(FileChange &change, const std::string &resource_name, napi_env env, InputSyncCallbacksThreadsafe input);

// Notify message from node-win
void notify_message_call(napi_env env, napi_value js_callback, void *context, void *data);
void register_threadsafe_message_callback(FileChange &change, const std::string &resource_name, napi_env env, InputSyncCallbacksThreadsafe input);