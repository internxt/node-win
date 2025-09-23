#include <windows.h>
#include "napi_extract_args.h"
#include "Placeholders.h"

napi_value get_placeholder_state_wrapper(napi_env env, napi_callback_info info) {
    auto [path] = napi_extract_args<std::wstring>(env, info);

    FileState state = Placeholders::GetPlaceholderInfo(path);

    napi_value result;
    napi_create_object(env, &result);

    napi_value jsPinState;
    napi_create_int32(env, static_cast<int32_t>(state.pinstate), &jsPinState);
    napi_set_named_property(env, result, "pinState", jsPinState);

    return result;
}
