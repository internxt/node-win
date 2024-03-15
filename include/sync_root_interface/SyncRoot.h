#pragma once

#include <cfapi.h>
#include <Callbacks.h>
#include "stdafx.h"

struct ItemInfo
{
    std::wstring path;
    std::wstring fileIdentity;
    bool isPlaceholder;
};

class SyncRoot
{
public:
    static HRESULT RegisterSyncRoot(const wchar_t *syncRootPath, const wchar_t *providerName, const wchar_t *providerVersion, const GUID &providerId, const wchar_t *logoPath);
    static HRESULT ConnectSyncRoot(const wchar_t *syncRootPath, InputSyncCallbacks syncCallbacks, napi_env env, CF_CONNECTION_KEY *connectionKey);
    static HRESULT DisconnectSyncRoot();
    static HRESULT UnregisterSyncRoot();
    static std::list<ItemInfo> GetItemsSyncRoot(const wchar_t *syncRootPath);
    static std::string GetFileIdentity(const wchar_t *path);
    static void DeleteFileSyncRoot(const wchar_t *path);
};