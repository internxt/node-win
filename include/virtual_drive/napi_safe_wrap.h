#ifndef NAPI_SAFE_WRAP_H
#define NAPI_SAFE_WRAP_H

#include <node_api.h>
#include <exception>
#include <string>

template <typename Fn>
napi_value napi_safe_wrap(napi_env env, napi_callback_info info, Fn&& fn, const char* function_name) {
    try {
        return fn(env, info);
    } catch (const std::exception& e) {
        std::string error_msg = std::string("[") + function_name + "] " + e.what();
        napi_throw_error(env, nullptr, error_msg.c_str());
    } catch (...) {
        std::string error_msg = std::string("[") + function_name + "] Unknown native error";
        napi_throw_error(env, nullptr, error_msg.c_str());
    }

    return nullptr;
}

#define NAPI_SAFE_WRAP(env, info, fn) \
    napi_safe_wrap(env, info, fn, __FUNCTION__)

#endif
