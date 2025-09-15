#include <Windows.h>
#include "Placeholders.h"

napi_value convert_to_placeholder_impl(napi_env env, napi_callback_info args)
{
    size_t argc = 2;
    napi_value argv[2];
    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    size_t pathLen, serverIdentityLen;
    napi_get_value_string_utf8(env, argv[0], nullptr, 0, &pathLen);
    napi_get_value_string_utf8(env, argv[1], nullptr, 0, &serverIdentityLen);

    std::string path(pathLen, '\0');
    std::string serverIdentity(serverIdentityLen, '\0');
    
    napi_get_value_string_utf8(env, argv[0], &path[0], pathLen + 1, nullptr);
    napi_get_value_string_utf8(env, argv[1], &serverIdentity[0], serverIdentityLen + 1, nullptr);

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
