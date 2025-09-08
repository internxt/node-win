#include "Callbacks.h"
#include "SyncRoot.h"
#include "stdafx.h"
#include <filesystem>
#include "Logger.h"
#include <iostream>
#include <vector>

namespace fs = std::filesystem;
// variable to disconect
CF_CONNECTION_KEY gloablConnectionKey;
std::map<std::wstring, CF_CONNECTION_KEY> connectionMap;

void AddCustomState(
    _In_ winrt::IVector<winrt::StorageProviderItemPropertyDefinition> &customStates,
    _In_ LPCWSTR displayNameResource,
    _In_ int id)
{
    winrt::StorageProviderItemPropertyDefinition customState;
    customState.DisplayNameResource(displayNameResource);
    customState.Id(id);
    customStates.Append(customState);
}

void SyncRoot::HydrateFile(const wchar_t *filePath)
{
    wprintf(L"Hydration file started %ls\n", filePath);
    DWORD attrib = GetFileAttributesW(filePath);
    if (!(attrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        winrt::handle placeholder(CreateFileW(filePath, 0, FILE_READ_DATA, nullptr, OPEN_EXISTING, 0, nullptr));

        LARGE_INTEGER offset;
        offset.QuadPart = 0;
        LARGE_INTEGER length;
        GetFileSizeEx(placeholder.get(), &length);

        if (attrib & FILE_ATTRIBUTE_PINNED)
        {
            // if (!(attrib & FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS))
            // {
            Logger::getInstance().log("Hydration file init", LogLevel::INFO);

            auto start = std::chrono::steady_clock::now();

            HRESULT hr = CfHydratePlaceholder(placeholder.get(), offset, length, CF_HYDRATE_FLAG_NONE, NULL);

            if (FAILED(hr))
            {
                Logger::getInstance().log("Error hydrating file " + Logger::fromWStringToString(filePath), LogLevel::ERROR);
            }
            else
            {
                auto end = std::chrono::steady_clock::now();
                auto elapsedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

                if (elapsedMilliseconds < 200)
                {
                    wprintf(L"Already Hydrated: %d ms\n", elapsedMilliseconds);
                }
                else
                {
                    wprintf(L"Hydration finished %ls\n", filePath);
                }
            }
        }
    }
}

void SyncRoot::DehydrateFile(const wchar_t *filePath)
{
    // Logger::getInstance().log("Dehydration file init", LogLevel::INFO);
    wprintf(L"Dehydration file init %ls\n", filePath);
    DWORD attrib = GetFileAttributesW(filePath);
    if (!(attrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        winrt::handle placeholder(CreateFileW(filePath, 0, FILE_READ_DATA, nullptr, OPEN_EXISTING, 0, nullptr));

        LARGE_INTEGER offset;
        offset.QuadPart = 0;
        LARGE_INTEGER length;
        GetFileSizeEx(placeholder.get(), &length);

        if (attrib & FILE_ATTRIBUTE_UNPINNED)
        {
            // Logger::getInstance().log("Dehydrating file " + Logger::fromWStringToString(filePath), LogLevel::INFO);
            wprintf(L"Dehydrating file starteeed %ls\n", filePath);
            HRESULT hr = CfDehydratePlaceholder(placeholder.get(), offset, length, CF_DEHYDRATE_FLAG_NONE, NULL);

            if (FAILED(hr))
            {
                DWORD err = HRESULT_CODE(hr);
                if (err == ERROR_SHARING_VIOLATION || err == ERROR_CLOUD_FILE_IN_USE)
                {
                    wprintf(L"Cannot dehydrate because the file is currently in use: %ls\n", filePath);

                    MessageBoxW(
                        nullptr,
                        L"Unable to free up space because the file is currently in use.\nPlease close the file and try again.",
                        L"File in use",
                        MB_OK | MB_ICONWARNING | MB_SYSTEMMODAL);
                }
                else
                {
                    wprintf(L"Error dehydrating file %ls\n", filePath);
                }
                // Logger::getInstance().log("Error dehydrating file " + Logger::fromWStringToString(filePath), LogLevel::ERROR);
            }
            else
            {
                wprintf(L"Dehydration finished %ls\n", filePath);
                // Logger::getInstance().log("Dehydration finished " + Logger::fromWStringToString(filePath), LogLevel::INFO);
            }
        }
    }
}

HRESULT SyncRoot::RegisterSyncRoot(const wchar_t *syncRootPath, const wchar_t *providerName, const wchar_t *providerVersion, const GUID &providerId, const wchar_t *logoPath)
{
    try
    {
        // Convert GUID to string for syncRootID
        wchar_t syncRootID[39];
        StringFromGUID2(providerId, syncRootID, 39);

        winrt::StorageProviderSyncRootInfo info;
        info.Id(syncRootID);

        auto folder = winrt::StorageFolder::GetFolderFromPathAsync(syncRootPath).get();
        info.Path(folder);

        // The string can be in any form acceptable to SHLoadIndirectString.
        info.DisplayNameResource(providerName);

        std::wstring completeIconResource = std::wstring(logoPath) + L",0";

        // This icon is just for the sample. You should provide your own branded icon here
        info.IconResource(completeIconResource.c_str());
        info.HydrationPolicy(winrt::StorageProviderHydrationPolicy::Full);
        info.HydrationPolicyModifier(winrt::StorageProviderHydrationPolicyModifier::None);
        info.PopulationPolicy(winrt::StorageProviderPopulationPolicy::AlwaysFull);
        info.InSyncPolicy(winrt::StorageProviderInSyncPolicy::FileCreationTime | winrt::StorageProviderInSyncPolicy::DirectoryCreationTime);
        info.Version(providerVersion);
        info.ShowSiblingsAsGroup(false);
        info.HardlinkPolicy(winrt::StorageProviderHardlinkPolicy::None);

        winrt::Uri uri(L"https://drive.internxt.com/app/trash");
        info.RecycleBinUri(uri);

        // Context
        std::wstring syncRootIdentity(syncRootPath);
        syncRootIdentity.append(L"#inxt#");
        syncRootIdentity.append(providerName);

        winrt::IBuffer contextBuffer = winrt::CryptographicBuffer::ConvertStringToBinary(syncRootIdentity.data(), winrt::BinaryStringEncoding::Utf8);
        info.Context(contextBuffer);

        winrt::IVector<winrt::StorageProviderItemPropertyDefinition> customStates = info.StorageProviderItemPropertyDefinitions();
        AddCustomState(customStates, L"CustomStateName1", 1);
        AddCustomState(customStates, L"CustomStateName2", 2);
        AddCustomState(customStates, L"CustomStateName3", 3);

        winrt::StorageProviderSyncRootManager::Register(info);

        return S_OK;
    }
    catch (...)
    {
        wprintf(L"Could not register the sync root, hr %08x\n", static_cast<HRESULT>(winrt::to_hresult()));
        return E_FAIL;
    }
}

std::vector<SyncRoots> SyncRoot::GetRegisteredSyncRoots()
{
    std::vector<SyncRoots> syncRootList;
    try
    {
        auto syncRoots = winrt::StorageProviderSyncRootManager::GetCurrentSyncRoots();

        printf("Sync roots count: %d\n", syncRoots.Size());

        for (auto const &info : syncRoots)
        {
            auto contextBuffer = info.Context();
            std::wstring contextString;
            if (contextBuffer)
            {
                contextString = winrt::CryptographicBuffer::ConvertBinaryToString(
                                    winrt::BinaryStringEncoding::Utf8,
                                    contextBuffer)
                                    .c_str();
            }

            /**
             * v2.5.1 Jonathan Arce
             * Sync root register are now filtered using the characters '->' and '#inxt#' to identify our register.
             * Currently, we only use '#inxt#' in the register, but to support previous versions, we are still
             * including '->' in the filter. In future versions, the filtering by '->' should be removed.
             */
            if (contextString.find(L"#inxt#") != std::wstring::npos || contextString.find(L"->") != std::wstring::npos)
            {
                SyncRoots sr;
                sr.id = info.Id();
                sr.path = info.Path().Path();
                sr.displayName = info.DisplayNameResource();
                sr.version = info.Version();
                syncRootList.push_back(sr);
            }
        }
    }
    catch (...)
    {
        Logger::getInstance().log("ERROR: getting sync root", LogLevel::INFO);
    }
    return syncRootList;
}

HRESULT SyncRoot::UnregisterSyncRoot(const GUID &providerId)
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

HRESULT SyncRoot::UnregisterSyncRoot(const wchar_t *providerIdStr)
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

HRESULT SyncRoot::ConnectSyncRoot(const wchar_t *syncRootPath, InputSyncCallbacks syncCallbacks, napi_env env, CF_CONNECTION_KEY *connectionKey)
{
    Utilities::AddFolderToSearchIndexer(syncRootPath);
    register_threadsafe_callbacks(env, syncCallbacks);

    CF_CALLBACK_REGISTRATION callbackTable[] = {
        {CF_CALLBACK_TYPE_FETCH_DATA, fetch_data_callback_wrapper},
        {CF_CALLBACK_TYPE_CANCEL_FETCH_DATA, cancel_fetch_data_callback_wrapper},
        CF_CALLBACK_REGISTRATION_END
    };

    HRESULT hr = CfConnectSyncRoot(
        syncRootPath,
        callbackTable,
        nullptr,
        CF_CONNECT_FLAG_REQUIRE_PROCESS_INFO | CF_CONNECT_FLAG_REQUIRE_FULL_FILE_PATH,
        connectionKey
    );

    wprintf(L"Connection key: %llu\n", connectionKey->Internal);
    
    if (SUCCEEDED(hr)) {
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

// struct
struct FileIdentityInfo
{
    BYTE *FileIdentity;
    size_t FileIdentityLength;
};

void SyncRoot::DeleteFileSyncRoot(const wchar_t *path)
{
    try
    {
        // Mostrar el archivo a eliminar
        wprintf(L"Intentando eliminar: %ls\n", path);

        // Verificar si el archivo es un directorio o un archivo regular
        bool isDirectory = fs::is_directory(path);
        wprintf(L"Es directorio: %d\n", isDirectory);

        // Si es un directorio, eliminar recursivamente
        if (isDirectory)
        {
            fs::remove_all(path); // Esta línea reemplaza la función personalizada DeleteFileOrDirectory
            wprintf(L"Directorio eliminado con éxito: %ls\n", path);
        }
        else
        {
            // Si es un archivo, simplemente eliminar
            if (!DeleteFileW(path))
            {
                wprintf(L"No se pudo eliminar el archivo: %ls\n", path);
            }
            else
            {
                wprintf(L"Archivo eliminado con éxito: %ls\n", path);
            }
        }
    }
    catch (const std::exception &e)
    {
        wprintf(L"Excepción capturada: %hs\n", e.what());
    }
    catch (...)
    {
        wprintf(L"Excepción desconocida capturada\n");
    }
}