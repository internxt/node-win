#pragma once

#include <node_api.h>
#include <iostream>
#include <CallbacksContext.h>

struct InputSyncCallbacks {
    napi_ref fetch_data_callback_ref;
    napi_ref validate_data_callback_ref;
    napi_ref cancel_fetch_data_callback_ref;
    napi_ref fetch_placeholders_callback_ref;
    napi_ref cancel_fetch_placeholders_callback_ref;
    napi_ref notify_file_open_completion_callback_ref;
    napi_ref notify_file_close_completion_callback_ref;
    napi_ref notify_dehydrate_callback_ref;
    napi_ref notify_dehydrate_completion_callback_ref;
    napi_ref notify_delete_callback_ref;
    napi_ref notify_delete_completion_callback_ref;
    napi_ref notify_rename_callback_ref;
    napi_ref notify_rename_completion_callback_ref;
    napi_ref none_callback_ref;
};

struct ThreadSafeCallbacksVersion {
    napi_threadsafe_function fetch_data_threadsafe_callback;
    napi_threadsafe_function validate_data_threadsafe_callback;
    napi_threadsafe_function cancel_fetch_data_threadsafe_callback;
    napi_threadsafe_function fetch_placeholders_threadsafe_callback;
    napi_threadsafe_function cancel_fetch_placeholders_threadsafe_callback;
    napi_threadsafe_function notify_file_open_completion_threadsafe_callback;
    napi_threadsafe_function notify_file_close_completion_threadsafe_callback;
    napi_threadsafe_function notify_dehydrate_threadsafe_callback;
    napi_threadsafe_function notify_dehydrate_completion_threadsafe_callback;
    napi_threadsafe_function notify_delete_threadsafe_callback;
    napi_threadsafe_function notify_delete_completion_threadsafe_callback;
    napi_threadsafe_function notify_rename_threadsafe_callback;
    napi_threadsafe_function notify_rename_completion_threadsafe_callback;
    napi_threadsafe_function none_threadsafe_callback;
};