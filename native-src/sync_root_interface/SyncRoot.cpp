#include "stdafx.h"
#include "SyncRoot.h"
#include "Callbacks.h"
#include <iostream>
#include <iostream>
#include <filesystem>
#include "Logger.h"
#include "DownloadMutexManager.h"

namespace fs = std::filesystem;
// variable to disconect
CF_CONNECTION_KEY gloablConnectionKey;

void TransformInputCallbacksToSyncCallbacks(napi_env env, InputSyncCallbacks input)
{
    register_threadsafe_callbacks(env, input);
}

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
            // Logger::getInstance().log("Hydration file init", LogLevel::INFO);
            wprintf(L"Hydration file init %ls\n", filePath);

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
            // }
            // else
            // {
            //     wprintf(L"File is already hydrated: %ls\n", filePath);
            //     Logger::getInstance().log("File is already hydrated " + Logger::fromWStringToString(filePath), LogLevel::INFO);
            // }
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
                wprintf(L"Error dehydrating file %ls\n", filePath);
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
        Logger::getInstance().log("Registering sync root.", LogLevel::INFO);
        auto syncRootID = providerId;

        winrt::StorageProviderSyncRootInfo info;
        info.Id(L"syncRootID");

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
        info.Version(L"1.0.0");
        info.ShowSiblingsAsGroup(false);
        info.HardlinkPolicy(winrt::StorageProviderHardlinkPolicy::None);

        winrt::Uri uri(L"https://drive.internxt.com/app/trash");
        info.RecycleBinUri(uri);

        // Context
        std::wstring syncRootIdentity(syncRootPath);
        syncRootIdentity.append(L"->");
        syncRootIdentity.append(L"TestProvider");

        wchar_t const contextString[] = L"TestProviderContextString";
        winrt::IBuffer contextBuffer = winrt::CryptographicBuffer::ConvertStringToBinary(syncRootIdentity.data(), winrt::BinaryStringEncoding::Utf8);
        info.Context(contextBuffer);

        winrt::IVector<winrt::StorageProviderItemPropertyDefinition> customStates = info.StorageProviderItemPropertyDefinitions();
        AddCustomState(customStates, L"CustomStateName1", 1);
        AddCustomState(customStates, L"CustomStateName2", 2);
        AddCustomState(customStates, L"CustomStateName3", 3);

        winrt::StorageProviderSyncRootManager::Register(info);

        Sleep(1000);

        return S_OK;
    }
    catch (...)
    {
        wprintf(L"Could not register the sync root, hr %08x\n", static_cast<HRESULT>(winrt::to_hresult()));
    }
}

HRESULT SyncRoot::UnregisterSyncRoot()
{
    try
    {
        Logger::getInstance().log("Unregistering sync root.", LogLevel::INFO);
        winrt::StorageProviderSyncRootManager::Unregister(L"syncRootID");
        return S_OK;
    }
    catch (...)
    {
        wprintf(L"Could not unregister the sync root, hr %08x\n", static_cast<HRESULT>(winrt::to_hresult()));
    }
}

HRESULT SyncRoot::ConnectSyncRoot(const wchar_t *syncRootPath, InputSyncCallbacks syncCallbacks, napi_env env, CF_CONNECTION_KEY *connectionKey)
{
    wprintf(L"Deggugin path: %ls\n", syncRootPath);
    try
    {
        Utilities::AddFolderToSearchIndexer(syncRootPath);

        TransformInputCallbacksToSyncCallbacks(env, syncCallbacks);

        CF_CALLBACK_REGISTRATION callbackTable[] = {
            {CF_CALLBACK_TYPE_NOTIFY_DELETE, notify_delete_callback_wrapper},
            {CF_CALLBACK_TYPE_NOTIFY_RENAME, notify_rename_callback_wrapper},
            {CF_CALLBACK_TYPE_FETCH_PLACEHOLDERS, fetch_placeholders_callback_wrapper},
            {CF_CALLBACK_TYPE_FETCH_DATA, fetch_data_callback_wrapper},
            {CF_CALLBACK_TYPE_CANCEL_FETCH_DATA, cancel_fetch_data_callback_wrapper},
            CF_CALLBACK_REGISTRATION_END};

        HRESULT hr = CfConnectSyncRoot(
            syncRootPath,
            callbackTable,
            nullptr, // Contexto (opcional)
            CF_CONNECT_FLAG_REQUIRE_PROCESS_INFO | CF_CONNECT_FLAG_REQUIRE_FULL_FILE_PATH,
            connectionKey);
        wprintf(L"Connection key: %llu\n", *connectionKey);
        gloablConnectionKey = *connectionKey;
        return hr;
    }
    catch (const std::exception &e)
    {
        wprintf(L"Excepción capturada: %hs\n", e.what());
        // Aquí puedes decidir si retornar un código de error específico o mantener el E_FAIL.
    }
    catch (...)
    {
        wprintf(L"Excepción desconocida capturada\n");
        // Igualmente, puedes decidir el código de error a retornar.
    }
}

// disconection sync root
HRESULT SyncRoot::DisconnectSyncRoot()
{
    Logger::getInstance().log("Disconnecting sync root.", LogLevel::INFO);
    try
    {
        HRESULT hr = CfDisconnectSyncRoot(gloablConnectionKey);
        return hr;
    }
    catch (const std::exception &e)
    {
        Logger::getInstance().log("Exception caught: " + std::string(e.what()), LogLevel::ERROR);
    }
    catch (...)
    {
        Logger::getInstance().log("Unknown exception caught.", LogLevel::ERROR);
    }
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