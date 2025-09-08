#include <Windows.h>
#include "SyncRoot.h"

napi_value connect_sync_root_impl(napi_env env, napi_callback_info args)
{
    size_t argc = 2;
    napi_value argv[2];
    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);
    std::wstring syncRootPath(pathLength, L'\0');
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t*>(&syncRootPath[0]), pathLength + 1, nullptr);

    InputSyncCallbacks callbacks = {};

    napi_value fetchDataCallback;
    napi_get_named_property(env, argv[1], "fetchDataCallback", &fetchDataCallback);
    napi_create_reference(env, fetchDataCallback, 1, &callbacks.fetch_data_callback_ref);

    napi_value cancelFetchDataCallback;
    napi_get_named_property(env, argv[1], "cancelFetchDataCallback", &cancelFetchDataCallback);
    napi_create_reference(env, cancelFetchDataCallback, 1, &callbacks.cancel_fetch_data_callback_ref);

    CF_CONNECTION_KEY connectionKey;
    HRESULT hr = SyncRoot::ConnectSyncRoot(syncRootPath.c_str(), callbacks, env, &connectionKey);

    if (FAILED(hr)) {
        napi_throw_error(env, nullptr, "ConnectSyncRoot failed");
        return nullptr;
    }

    napi_value resultObj, hrValue, connectionKeyValue;
    napi_create_object(env, &resultObj);

    napi_create_int32(env, static_cast<int32_t>(hr), &hrValue);
    napi_set_named_property(env, resultObj, "hr", hrValue);

    std::wstring connectionKeyString = std::to_wstring(connectionKey.Internal);
    napi_create_string_utf16(env, reinterpret_cast<const char16_t*>(connectionKeyString.c_str()), connectionKeyString.length(), &connectionKeyValue);
    napi_set_named_property(env, resultObj, "connectionKey", connectionKeyValue);

    return resultObj;
}
