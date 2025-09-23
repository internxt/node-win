#include <Windows.h>
#include "napi_extract_args.h"
#include "Placeholders.h"

napi_value convert_to_placeholder_impl(napi_env env, napi_callback_info info) {
    auto [path, serverIdentity] = napi_extract_args<std::wstring, std::wstring>(env, info);

    std::wstring wPath(path.begin(), path.end());
    std::wstring wServerIdentity(serverIdentity.begin(), serverIdentity.end());

    PlaceholderResult result = Placeholders::ConvertToPlaceholder(wPath, wServerIdentity);

    napi_value resultObj;
    napi_create_object(env, &resultObj);

    napi_value successValue;
    napi_get_boolean(env, result.success, &successValue);
    napi_set_named_property(env, resultObj, "success", successValue);

    if (!result.success)
    {
        std::string errorMessage(result.errorMessage.begin(), result.errorMessage.end());
        napi_value errorValue;
        napi_create_string_utf8(env, errorMessage.c_str(), errorMessage.length(), &errorValue);
        napi_set_named_property(env, resultObj, "errorMessage", errorValue);
    }

    return resultObj;
}
