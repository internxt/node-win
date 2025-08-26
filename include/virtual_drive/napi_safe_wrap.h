#ifndef NAPI_SAFE_WRAP_H
#define NAPI_SAFE_WRAP_H

#include <node_api.h>
#include <exception>
#include <string>

template <typename Fn>
napi_value napi_safe_wrap(napi_env env, napi_callback_info info, Fn&& fn) {
    try {
        return fn(env, info);
    } catch (const std::exception& e) {
        napi_throw_error(env, nullptr, e.what());
    } catch (...) {
        napi_throw_error(env, nullptr, "Unknown native error");
    }

    return nullptr;
}

#endif
