#include <windows.h>
#include "unregister_sync_root.h"

/**
 * v2.5.7 Carlos Gonzalez
 * Added backward compatibility for the default virtual drive identifier "syncRootID".
 * If the provided ID is "syncRootID", it will be unregistered without throwing an error,
 * maintaining support for previous versions that used it instead of a GUID.
 */
napi_value unregister_sync_root_wrapper(napi_env env, napi_callback_info args) {
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    GUID providerId;
    LPCWSTR providerIdStr;
    size_t providerIdStrLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &providerIdStrLength);
    providerIdStr = new WCHAR[providerIdStrLength + 1];
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(providerIdStr)), providerIdStrLength + 1, nullptr);


    HRESULT result;
    HRESULT guidResult = CLSIDFromString(providerIdStr, &providerId);

    if (SUCCEEDED(guidResult)) {
        result = unregister_sync_root(providerId);
    }
    else if (wcscmp(providerIdStr, L"syncRootID") == 0) {
        result = unregister_sync_root(providerIdStr);
    }
    else {
        napi_throw_error(env, nullptr, "Invalid provider ID: must be a GUID or 'syncRootID'");
        delete[] providerIdStr;
        return nullptr;
    }

    delete[] providerIdStr;

    napi_value napiResult;
    napi_create_int32(env, static_cast<int32_t>(result), &napiResult);
    return napiResult;
}
