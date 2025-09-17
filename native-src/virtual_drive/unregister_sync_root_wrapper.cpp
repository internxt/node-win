#include <windows.h>
#include <node_api.h>
#include <string>
#include "stdafx.h"

napi_value unregister_sync_root_wrapper(napi_env env, napi_callback_info args) {
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    size_t providerIdLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &providerIdLength);
    std::wstring providerId(providerIdLength, L'\0');
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t*>(&providerId[0]), providerIdLength + 1, nullptr);

    winrt::StorageProviderSyncRootManager::Unregister(providerId);

    return nullptr;
}
