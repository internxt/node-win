#include <Windows.h>
#include "Placeholders.h"

napi_value get_file_identity_impl(napi_env env, napi_callback_info args)
{
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, nullptr, "The path is required for GetFileIdentity");
        return nullptr;
    }

    LPCWSTR fullPath;
    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);
    fullPath = new WCHAR[pathLength + 1];
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(fullPath)), pathLength + 1, nullptr);

    std::string fileIdentity = Placeholders::GetFileIdentity(fullPath);
    fileIdentity.erase(std::remove(fileIdentity.begin(), fileIdentity.end(), '\0'), fileIdentity.end());
    fileIdentity.erase(std::remove(fileIdentity.begin(), fileIdentity.end(), ' '), fileIdentity.end());

    napi_value jsFileIdentity;
    napi_create_string_utf8(env, fileIdentity.c_str(), fileIdentity.length(), &jsFileIdentity);

    delete[] fullPath;
    return jsFileIdentity;
}
