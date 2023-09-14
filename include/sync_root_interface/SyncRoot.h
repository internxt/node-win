#pragma once

#include <cfapi.h>
#include <Callbacks.h>

class SyncRoot
{
public:
    static HRESULT RegisterSyncRoot(const wchar_t *syncRootPath, const wchar_t *providerName, const wchar_t *providerVersion, const GUID &providerId);
    static HRESULT ConnectSyncRoot(const wchar_t *syncRootPath, InputSyncCallbacks syncCallbacks, napi_env env, CF_CONNECTION_KEY *connectionKey);
    static HRESULT DisconnectSyncRoot();
    static HRESULT UnregisterSyncRoot();
};