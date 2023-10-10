#include "stdafx.h"
#include "SyncRoot.h"
#include "Callbacks.h"
#include <iostream>
#include <iostream>
#include <filesystem>

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

HRESULT SyncRoot::RegisterSyncRoot(const wchar_t *syncRootPath, const wchar_t *providerName, const wchar_t *providerVersion, const GUID &providerId, const wchar_t *logoPath)
{
    try
    {
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
        winrt::StorageProviderSyncRootManager::Unregister(L"syncRootID");
        return S_OK;
    }
    catch (...)
    {
        // winrt::to_hresult() will eat the exception if it is a result of winrt::check_hresult,
        // otherwise the exception will get rethrown and this method will crash out as it should
        wprintf(L"Could not unregister the sync root, hr %08x\n", static_cast<HRESULT>(winrt::to_hresult()));
    }
}

HRESULT SyncRoot::ConnectSyncRoot(const wchar_t *syncRootPath, InputSyncCallbacks syncCallbacks, napi_env env, CF_CONNECTION_KEY *connectionKey)
{
    try
    {
        Utilities::AddFolderToSearchIndexer(syncRootPath);

        TransformInputCallbacksToSyncCallbacks(env, syncCallbacks);

        CF_CALLBACK_REGISTRATION callbackTable[] = {
            {CF_CALLBACK_TYPE_NOTIFY_DELETE, notify_delete_callback_wrapper},
            {CF_CALLBACK_TYPE_NOTIFY_RENAME, notify_rename_callback_wrapper},
            {CF_CALLBACK_TYPE_FETCH_PLACEHOLDERS, fetch_placeholders_callback_wrapper},
            {CF_CALLBACK_TYPE_FETCH_DATA, fetch_data_callback_wrapper},
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
    try
    {
        HRESULT hr = CfDisconnectSyncRoot(gloablConnectionKey);
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

// struct
struct FileIdentityInfo
{
    BYTE *FileIdentity;
    size_t FileIdentityLength;
};

std::vector<std::wstring> fileIdentities; // Vector para almacenar FileIdentity como strings

void EnumerateAndQueryPlaceholders(const std::wstring &directoryPath)
{

    for (const auto &entry : std::filesystem::directory_iterator(directoryPath))
    {
        HANDLE hFile = CreateFileW(
            entry.path().c_str(),
            FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            int size = sizeof(CF_PLACEHOLDER_STANDARD_INFO) + 300;
            CF_PLACEHOLDER_STANDARD_INFO PlaceholderInfo;
            DWORD returnlength(0);
            HRESULT hr = CfGetPlaceholderInfo(hFile, CF_PLACEHOLDER_INFO_STANDARD, &PlaceholderInfo, size, &returnlength);
            if (SUCCEEDED(hr))
            {
                LARGE_INTEGER FileId = PlaceholderInfo.FileId;
                BYTE *FileIdentity = PlaceholderInfo.FileIdentity;
                // Convertir FileIdentity a una cadena wstring
                size_t identityLength = PlaceholderInfo.FileIdentityLength / sizeof(wchar_t);
                std::wstring fileIdentityString(reinterpret_cast<const wchar_t *>(FileIdentity), identityLength);
                fileIdentities.push_back(fileIdentityString);
            }
            else
            {
                wprintf(L"La llamada a CfGetPlaceholderInfo falló con el código de error 0x%X\n", hr);
            }
            CloseHandle(hFile);
            if (entry.is_directory())
            {
                EnumerateAndQueryPlaceholders(entry.path().c_str());
            }
        }
        else
        {
            wprintf(L"Invalid Item: %ls\n", entry.path().c_str());
        }
    }
}
// get items sync root
HRESULT SyncRoot::GetItemsSyncRoot(const wchar_t *syncRootPath, std::vector<std::wstring> &getFileIdentities)
{
    try
    {
        EnumerateAndQueryPlaceholders(syncRootPath);
        getFileIdentities = fileIdentities;
        fileIdentities.clear();
        return S_OK;
    }
    catch (const std::exception &e)
    {
        wprintf(L"Excepción capturada: %hs\n", e.what());
    }
}
