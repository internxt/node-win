#include <Windows.h>
#include "Placeholders.h"

napi_value convert_to_placeholder_impl(napi_env env, napi_callback_info args)
{
    size_t argc = 2;
    napi_value argv[2];
    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 2)
    {
        napi_throw_type_error(env, nullptr, "Wrong number of arguments");
        return nullptr;
    }

    char path[1024];
    char serverIdentity[1024];
    size_t pathLen, serverIdentityLen;

    napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &pathLen);
    napi_get_value_string_utf8(env, argv[1], serverIdentity, sizeof(serverIdentity), &serverIdentityLen);

    std::wstring wPath(path, path + pathLen);
    std::wstring wServerIdentity(serverIdentity, serverIdentity + serverIdentityLen);

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
