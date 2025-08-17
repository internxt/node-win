#include <Windows.h>
#include "SyncRoot.h"

napi_value register_sync_root_impl(napi_env env, napi_callback_info args)
{
    size_t argc = 5;
    napi_value argv[5];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 5)
    {
        napi_throw_error(env, nullptr, "5 arguments are required for RegisterSyncRoot");
        return nullptr;
    }

    LPCWSTR syncRootPath;
    size_t syncRootPathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &syncRootPathLength);
    syncRootPath = new WCHAR[syncRootPathLength + 1];
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(syncRootPath)), syncRootPathLength + 1, nullptr);

    LPCWSTR providerName;
    size_t providerNameLength;
    napi_get_value_string_utf16(env, argv[1], nullptr, 0, &providerNameLength);
    providerName = new WCHAR[providerNameLength + 1];
    napi_get_value_string_utf16(env, argv[1], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(providerName)), providerNameLength + 1, nullptr);

    LPCWSTR providerVersion;
    size_t providerVersionLength;
    napi_get_value_string_utf16(env, argv[2], nullptr, 0, &providerVersionLength);
    providerVersion = new WCHAR[providerVersionLength + 1];
    napi_get_value_string_utf16(env, argv[2], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(providerVersion)), providerVersionLength + 1, nullptr);

    GUID providerId;
    LPCWSTR providerIdStr;
    size_t providerIdStrLength;
    napi_get_value_string_utf16(env, argv[3], nullptr, 0, &providerIdStrLength);
    providerIdStr = new WCHAR[providerIdStrLength + 1];
    napi_get_value_string_utf16(env, argv[3], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(providerIdStr)), providerIdStrLength + 1, nullptr);

    if (FAILED(CLSIDFromString(providerIdStr, &providerId)))
    {
        napi_throw_error(env, nullptr, "Invalid GUID format");
        delete[] syncRootPath;
        delete[] providerName;
        delete[] providerVersion;
        delete[] providerIdStr;
        return nullptr;
    }

    LPCWSTR logoPath;
    size_t logoPathLength;
    napi_get_value_string_utf16(env, argv[4], nullptr, 0, &logoPathLength);
    logoPath = new WCHAR[logoPathLength + 1];
    napi_get_value_string_utf16(env, argv[4], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(logoPath)), logoPathLength + 1, nullptr);

    HRESULT result = SyncRoot::RegisterSyncRoot(syncRootPath, providerName, providerVersion, providerId, logoPath);

    delete[] providerIdStr;

    napi_value napiResult;
    napi_create_int32(env, static_cast<int32_t>(result), &napiResult);
    return napiResult;
}
