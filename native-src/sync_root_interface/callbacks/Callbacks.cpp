#include <Callbacks.h>
#include <string>

void register_threadsafe_callbacks(napi_env env, InputSyncCallbacks input)
{
    register_threadsafe_notify_delete_callback("NotifyDeleteThreadSafe", env, input);
    register_threadsafe_notify_rename_callback("NotifyRenameThreadSafe", env, input);
    register_threadsafe_fetch_data_callback("FetchDataThreadSafe", env, input);
    register_threadsafe_cancel_fetch_data_callback("CancelFetchDataThreadSafe", env, input);
}