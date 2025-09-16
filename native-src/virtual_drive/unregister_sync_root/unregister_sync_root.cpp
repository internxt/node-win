#include <windows.h>
#include "Logger.h"
#include "SyncRoot.h"

HRESULT unregister_sync_root(const GUID &providerId)
{
    try
    {
        // Convert GUID to string for syncRootID
        wchar_t syncRootID[39];
        StringFromGUID2(providerId, syncRootID, 39);

        Logger::getInstance().log("Unregistering sync root.", LogLevel::INFO);
        winrt::StorageProviderSyncRootManager::Unregister(syncRootID);
        return S_OK;
    }
    catch (...)
    {
        wprintf(L"Could not unregister the sync root, hr %08x\n", static_cast<HRESULT>(winrt::to_hresult()));
        return E_FAIL;
    }
}

HRESULT unregister_sync_root(const wchar_t *providerIdStr)
{
    try
    {
        Logger::getInstance().log("Unregistering sync root (string).", LogLevel::INFO);
        winrt::StorageProviderSyncRootManager::Unregister(providerIdStr);
        return S_OK;
    }
    catch (...)
    {
        wprintf(L"Could not unregister the sync root (string), hr %08x\n", static_cast<HRESULT>(winrt::to_hresult()));
        return E_FAIL;
    }
}
