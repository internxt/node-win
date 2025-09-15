#include <Windows.h>
#include "register_sync_root.h"

napi_value register_sync_root_wrapper(napi_env env, napi_callback_info args)
{
    size_t argc = 5;
    napi_value argv[5];
    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    size_t syncRootPathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &syncRootPathLength);
    std::wstring syncRootPath(syncRootPathLength, L'\0');
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t*>(&syncRootPath[0]), syncRootPathLength + 1, nullptr);

    size_t providerNameLength;
    napi_get_value_string_utf16(env, argv[1], nullptr, 0, &providerNameLength);
    std::wstring providerName(providerNameLength, L'\0');
    napi_get_value_string_utf16(env, argv[1], reinterpret_cast<char16_t*>(&providerName[0]), providerNameLength + 1, nullptr);

    size_t providerVersionLength;
    napi_get_value_string_utf16(env, argv[2], nullptr, 0, &providerVersionLength);
    std::wstring providerVersion(providerVersionLength, L'\0');
    napi_get_value_string_utf16(env, argv[2], reinterpret_cast<char16_t*>(&providerVersion[0]), providerVersionLength + 1, nullptr);

    size_t providerIdStrLength;
    napi_get_value_string_utf16(env, argv[3], nullptr, 0, &providerIdStrLength);
    std::wstring providerIdStr(providerIdStrLength, L'\0');
    napi_get_value_string_utf16(env, argv[3], reinterpret_cast<char16_t*>(&providerIdStr[0]), providerIdStrLength + 1, nullptr);

    GUID providerId;
    CLSIDFromString(providerIdStr.c_str(), &providerId);

    size_t logoPathLength;
    napi_get_value_string_utf16(env, argv[4], nullptr, 0, &logoPathLength);
    std::wstring logoPath(logoPathLength, L'\0');
    napi_get_value_string_utf16(env, argv[4], reinterpret_cast<char16_t*>(&logoPath[0]), logoPathLength + 1, nullptr);

    HRESULT result = register_sync_root(
        syncRootPath.c_str(), 
        providerName.c_str(), 
        providerVersion.c_str(), 
        providerId, 
        logoPath.c_str()
    );

    napi_value napiResult;
    napi_create_int32(env, static_cast<int32_t>(result), &napiResult);
    return napiResult;
}
