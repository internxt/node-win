#pragma once

#include <cfapi.h>

class SyncRoot
{
    public:
        static HRESULT RegisterSyncRoot(const wchar_t* syncRootPath, const wchar_t* providerName, const wchar_t* providerVersion, const GUID& providerId);
        static HRESULT ConnectSyncRoot(const wchar_t* syncRootPath, SyncCallbacks syncCallbacks, CF_CONNECTION_KEY* connectionKey);
        static HRESULT UnregisterSyncRoot();
};