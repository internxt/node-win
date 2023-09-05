#include <Callbacks.h>
#include <string>

void register_threadsafe_callbacks(napi_env env, InputSyncCallbacks input) {
    register_threadsafe_notify_delete_callback("NotifyDeleteThreadSafe", env, input);
}