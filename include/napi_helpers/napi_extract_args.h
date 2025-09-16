#include <node_api.h>
#include <string>

template<size_t N>
std::array<std::wstring, N> napi_extract_args(napi_env env, napi_callback_info info) {
    size_t argc = N;
    napi_value argv[N];
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    
    std::array<std::wstring, N> result;
    
    for (size_t i = 0; i < N; ++i) {
        size_t length;
        napi_get_value_string_utf16(env, argv[i], nullptr, 0, &length);
        result[i].resize(length);
        napi_get_value_string_utf16(env, argv[i], reinterpret_cast<char16_t*>(result[i].data()), length + 1, nullptr);
    }
    
    return result;
}
