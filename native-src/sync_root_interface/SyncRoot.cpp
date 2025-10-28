#include "Callbacks.h"
#include "SyncRoot.h"
#include "stdafx.h"
#include <filesystem>
#include "Logger.h"
#include <iostream>
#include <vector>

std::map<std::wstring, CF_CONNECTION_KEY> connectionMap;

HRESULT SyncRoot::ConnectSyncRoot(const wchar_t *syncRootPath, InputSyncCallbacks syncCallbacks, napi_env env, CF_CONNECTION_KEY *connectionKey)
{
    register_threadsafe_fetch_data_callback("FetchDataThreadSafe", env, syncCallbacks);
    register_threadsafe_cancel_fetch_data_callback("CancelFetchDataThreadSafe", env, syncCallbacks);

    CF_CALLBACK_REGISTRATION callbackTable[] = {
        {CF_CALLBACK_TYPE_FETCH_DATA, fetch_data_callback_wrapper},
        {CF_CALLBACK_TYPE_CANCEL_FETCH_DATA, cancel_fetch_data_callback_wrapper},
        CF_CALLBACK_REGISTRATION_END};

    HRESULT hr = CfConnectSyncRoot(
        syncRootPath,
        callbackTable,
        nullptr,
        CF_CONNECT_FLAG_REQUIRE_PROCESS_INFO | CF_CONNECT_FLAG_REQUIRE_FULL_FILE_PATH,
        connectionKey);

    wprintf(L"Connection key: %llu\n", connectionKey->Internal);

    if (SUCCEEDED(hr))
    {
        connectionMap[syncRootPath] = *connectionKey;
    }

    return hr;
}

// disconection sync root
HRESULT SyncRoot::DisconnectSyncRoot(const wchar_t *syncRootPath)
{
    auto it = connectionMap.find(syncRootPath);
    if (it != connectionMap.end())
    {
        HRESULT hr = CfDisconnectSyncRoot(it->second);
        if (SUCCEEDED(hr))
        {
            connectionMap.erase(it);
        }
        return hr;
    }
    return E_FAIL;
}
