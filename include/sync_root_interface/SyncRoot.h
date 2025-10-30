#pragma once

#include <cfapi.h>
#include <Callbacks.h>
#include "stdafx.h"
#include <iostream>
#include <vector>

struct ItemInfo
{
    std::wstring path;
    std::wstring fileIdentity;
    bool isPlaceholder;
};

class SyncRoot
{
public:
    static HRESULT ConnectSyncRoot(const wchar_t *syncRootPath, InputSyncCallbacks syncCallbacks, napi_env env, CF_CONNECTION_KEY *connectionKey);
    static HRESULT DisconnectSyncRoot(const wchar_t *syncRootPath);
    static void HydrateFile(const wchar_t *filePath);

private:
    CF_CONNECTION_KEY connectionKey;
};
