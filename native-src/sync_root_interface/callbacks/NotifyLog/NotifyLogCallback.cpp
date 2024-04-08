#include "Callbacks.h"

inline std::string to_publish_message;
inline std::string to_publish_level;

inline std::mutex mtx_log;
inline std::condition_variable cv_log;
inline bool ready_log = false;


struct LogArgs {
    std::string message;
    std::string level;
};

void notify_log_call(napi_env env, napi_value js_callback, void *context, void *data) {
    napi_status status;
    napi_value message_napi, level_napi, undefined, result;

    LogArgs *logArgs = static_cast<LogArgs*>(data);

    std::string message = logArgs->message;
    std::string level = logArgs->level;

    status = napi_create_string_utf8(env, message.c_str(), message.length(), &message_napi);
    if (status != napi_ok) {
        fprintf(stderr, "Failed to create string.\n");
        delete logArgs;
        return;
    }
    
    status = napi_create_string_utf8(env, level.c_str(), level.length(), &level_napi);
    if (status != napi_ok) {
        fprintf(stderr, "Failed to create string.\n");
        delete logArgs;
        return;
    }

    napi_value args_to_js_callback[2] = { message_napi, level_napi};

    status = napi_get_undefined(env, &undefined);
    if (status != napi_ok) {
        fprintf(stderr, "Failed to get undefined value.\n");
        delete logArgs;
        return;
    }

    status = napi_call_function(env, nullptr, js_callback, 2, args_to_js_callback, &result);
    if (status != napi_ok) {
        fprintf(stderr, "Failed to call function.\n");
    }

    {
        std::lock_guard<std::mutex> lock(mtx_log);
        ready_log = true;
    }


    delete logArgs;

}
void register_threadsafe_notify_log_callback(const std::string &message, const std::string &level, napi_env env, napi_threadsafe_function threadsafe_function) {

    LogArgs *args = new LogArgs(); 
    args->message = message;
    args->level = level;

    napi_status status = napi_call_threadsafe_function(threadsafe_function, args, napi_tsfn_blocking);

    {
        std::unique_lock<std::mutex> lock(mtx_log);
        while (!ready_log) {
            cv_log.wait(lock);
        }
    }
}