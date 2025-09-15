#include <Windows.h>
#include "SyncRoot.h"

napi_value connect_sync_root_impl(napi_env env, napi_callback_info args)
{
    size_t argc = 2;
    napi_value argv[2];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 2)
    {
        napi_throw_error(env, nullptr, "Se requieren más argumentos para ConnectSyncRoot");
        return nullptr;
    }

    LPCWSTR syncRootPath;
    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);
    syncRootPath = new WCHAR[pathLength + 1];
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(syncRootPath)), pathLength + 1, nullptr);

    // CALLBACKS
    InputSyncCallbacks callbacks = {};

    napi_value fetchDataCallback;
    napi_value cancelFetchDataCallback;

    if (napi_get_named_property(env, argv[1], "fetchDataCallback", &fetchDataCallback) == napi_ok)
    {
        napi_create_reference(env, fetchDataCallback, 1, &callbacks.fetch_data_callback_ref);
    }

    napi_valuetype valuetype_fetch_data;
    napi_status type_status_fetch_data = napi_typeof(env, fetchDataCallback, &valuetype_fetch_data);
    if (type_status_fetch_data != napi_ok || valuetype_fetch_data != napi_function)
    {
        napi_throw_error(env, nullptr, "fetchDataCallback should be a function.");
        return nullptr;
    }

    if (napi_get_named_property(env, argv[1], "cancelFetchDataCallback", &cancelFetchDataCallback) == napi_ok)
    {
        napi_create_reference(env, cancelFetchDataCallback, 1, &callbacks.cancel_fetch_data_callback_ref);
    }

    napi_valuetype valuetype_cancel_fetch_data;
    napi_status type_status_cancel_fetch_data = napi_typeof(env, cancelFetchDataCallback, &valuetype_cancel_fetch_data);
    if (type_status_cancel_fetch_data != napi_ok || valuetype_cancel_fetch_data != napi_function)
    {
        napi_throw_error(env, nullptr, "cancelFetchDataCallback should be a function.");
        return nullptr;
    }

    CF_CONNECTION_KEY connectionKey;
    HRESULT hr = SyncRoot::ConnectSyncRoot(syncRootPath, callbacks, env, &connectionKey);

    delete[] syncRootPath;

    if (FAILED(hr))
    {
        napi_throw_error(env, nullptr, "ConnectSyncRoot failed");
        return nullptr;
    }

    napi_value resultObj, hrValue, connectionKeyValue;

    napi_create_object(env, &resultObj);

    napi_create_int32(env, static_cast<int32_t>(hr), &hrValue);
    napi_set_named_property(env, resultObj, "hr", hrValue);

    std::wstringstream ss;
    ss << connectionKey.Internal;

    std::wstring connectionKeyString = ss.str();
    napi_create_string_utf16(env, reinterpret_cast<const char16_t *>(connectionKeyString.c_str()), connectionKeyString.length(), &connectionKeyValue);

    napi_set_named_property(env, resultObj, "connectionKey", connectionKeyValue);

    return resultObj;
}
