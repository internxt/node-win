#pragma once

#include <cfapi.h>
#include <Callbacks.h>
#include "stdafx.h"
#include <iostream>
#include <vector>

struct SyncRoots
{
    std::wstring id;
    std::wstring path;
    std::wstring displayName;
    std::wstring version;
};

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
    static std::vector<SyncRoots> GetRegisteredSyncRoots();
    static HRESULT ConnectSyncRoot(const wchar_t *syncRootPath, InputSyncCallbacks syncCallbacks, napi_env env, CF_CONNECTION_KEY *connectionKey);
    static HRESULT DisconnectSyncRoot(const wchar_t *syncRootPath);
    static HRESULT UnregisterSyncRoot(const GUID &providerId);
    static std::string GetFileIdentity(const wchar_t *path);
    static void HydrateFile(const wchar_t *filePath);
    static void DehydrateFile(const wchar_t *filePath);
    static void DeleteFileSyncRoot(const wchar_t *path);

private:
    CF_CONNECTION_KEY connectionKey;
};
