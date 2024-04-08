#include "stdafx.h"
#include "SyncRoot.h"
#include "Callbacks.h"
#include "Logger.h"
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
        std::stringstream ss;
        ss << "Could not register the sync root, hr %08x\n", static_cast<HRESULT>(winrt::to_hresult());
        std::string message = ss.str();
        Logger::getInstance().log(message, LogLevel::ERROR);
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
        std::stringstream ss;
        ss << "Could not unregister the sync root, hr %08x\n", static_cast<HRESULT>(winrt::to_hresult());
        std::string message = ss.str();
        Logger::getInstance().log(message, LogLevel::ERROR);
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
            {CF_CALLBACK_TYPE_CANCEL_FETCH_DATA, cancel_fetch_data_callback_wrapper},
            CF_CALLBACK_REGISTRATION_END};

        HRESULT hr = CfConnectSyncRoot(
            syncRootPath,
            callbackTable,
            nullptr, // Contexto (opcional)
            CF_CONNECT_FLAG_REQUIRE_PROCESS_INFO | CF_CONNECT_FLAG_REQUIRE_FULL_FILE_PATH,
            connectionKey);
        std::stringstream ss;
        ss << "Connection key: %llu\n", *connectionKey;
        std::string message = ss.str();
        Logger::getInstance().log(message, LogLevel::INFO);
        gloablConnectionKey = *connectionKey;
        return hr;
    }
    catch (const std::exception &e)
    {
        std::stringstream ss;
        ss << "Excepción capturada: %hs\n", e.what();
        std::string message = ss.str();
        Logger::getInstance().log(message, LogLevel::ERROR);
        // Aquí puedes decidir si retornar un código de error específico o mantener el E_FAIL.
    }
    catch (...)
    {
        Logger::getInstance().log("Excepción capturada desconocida", LogLevel::ERROR);
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
        std::stringstream ss;
        ss << "Excepción capturada: %hs\n", e.what();
        std::string message = ss.str();
        Logger::getInstance().log(message, LogLevel::ERROR);
        // Aquí puedes decidir si retornar un código de error específico o mantener el E_FAIL.
    }
    catch (...)
    {
        Logger::getInstance().log("Excepción capturada desconocida", LogLevel::ERROR);
        // Igualmente, puedes decidir el código de error a retornar.
    }
}

// struct
struct FileIdentityInfo
{
    BYTE *FileIdentity;
    size_t FileIdentityLength;
};

void EnumerateAndQueryPlaceholders(const std::wstring &directoryPath, std::list<ItemInfo> &itemInfo)
{
    int count = 0;
    // iterator
    for (const auto &entry : std::filesystem::directory_iterator(directoryPath))
    {

        Logger::getInstance().log("Entry:" + Logger::fromWStringToString(entry.path()), LogLevel::INFO);
        ItemInfo item;
        item.path = entry.path().c_str();
        try
        {
            bool isDirectory = entry.is_directory();
            HANDLE hFile = CreateFileW(
                entry.path().c_str(),
                FILE_READ_ATTRIBUTES,
                FILE_SHARE_READ,
                nullptr,
                OPEN_EXISTING,
                isDirectory ? FILE_FLAG_BACKUP_SEMANTICS : FILE_ATTRIBUTE_NORMAL,
                nullptr);
            if (hFile)
            {
                int size = sizeof(CF_PLACEHOLDER_STANDARD_INFO) + 5000;
                CF_PLACEHOLDER_STANDARD_INFO *standard_info = (CF_PLACEHOLDER_STANDARD_INFO *)new BYTE[size];
                DWORD returnlength(0);
                HRESULT hr = CfGetPlaceholderInfo(hFile, CF_PLACEHOLDER_INFO_STANDARD, standard_info, size, &returnlength);

                if (SUCCEEDED(hr))
                {
                    // LARGE_INTEGER FileId = PlaceholderInfoForIds.FileId;
                    BYTE *FileIdentity = standard_info->FileIdentity;
                    // Convertir FileIdentity a una cadena wstring
                    size_t identityLength = standard_info->FileIdentityLength / sizeof(wchar_t);
                    std::wstring fileIdentityString(reinterpret_cast<const wchar_t *>(FileIdentity), identityLength);
                    Logger::getInstance().log("FileIdentity: " + Logger::fromWStringToString(fileIdentityString), LogLevel::INFO);
                    item.fileIdentity = fileIdentityString;
                    item.isPlaceholder = true;
                    itemInfo.push_back(item);
                }
                else
                {
                    Logger::getInstance().log("Error likely not a placeholder \n", LogLevel::ERROR);
                    item.fileIdentity = L"";
                    item.isPlaceholder = false;
                }
                // limpiar memoria para repetir el proceso en el for
                CloseHandle(hFile);

                if (isDirectory)
                {
                    Logger::getInstance().log("IsDirectory:  \n", LogLevel::INFO);
                    EnumerateAndQueryPlaceholders(entry.path().c_str(), itemInfo);
                }
            }
            else
            {
                Logger::getInstance().log("Invalid Item: " + Logger::fromWStringToString(entry.path()), LogLevel::WARN);
            }
            // CloseHandle(hFile);
            // hFile = INVALID_HANDLE_VALUE;
        }
        catch (const std::exception &e)
        {
            std::stringstream ss;
            ss << "Excepción capturada: %hs\n", e.what();
            std::string message = ss.str();
            Logger::getInstance().log(message, LogLevel::ERROR);
        }
        catch (...)
        {
            Logger::getInstance().log("Excepción capturada desconocida", LogLevel::ERROR);
        }

        count++;
    }
    std::stringstream ss;
    ss << "Count items: %d\n", count;
    std::string message = ss.str();
    Logger::getInstance().log(message, LogLevel::ERROR);
    return;
}

// get items sync root
std::list<ItemInfo> SyncRoot::GetItemsSyncRoot(const wchar_t *syncRootPath)
{
    try
    {
        Logger::getInstance().log("[Start] GetItemsSyncRoot\n", LogLevel::INFO);
        std::list<ItemInfo> itemInfo;
        EnumerateAndQueryPlaceholders(syncRootPath, itemInfo);
        Logger::getInstance().log("[End] GetItemsSyncRoot\n", LogLevel::INFO);
        return itemInfo;
    }
    catch (const std::exception &e)
    {
        std::stringstream ss;
        ss << "Excepción capturada: %hs\n", e.what();
        std::string message = ss.str();
        Logger::getInstance().log(message, LogLevel::ERROR);
    }
}

// get fileIdentity by path
std::string SyncRoot::GetFileIdentity(const wchar_t *path)
{
    try
    {
        Logger::getInstance().log("GetFileIdentity: " + Logger::fromWStringToString(path), LogLevel::INFO);

        bool isDirectory = fs::is_directory(path);
        std::stringstream ss;
        ss << "IsDirectory: %d\n", isDirectory;
        std::string message = ss.str();
        Logger::getInstance().log(message, LogLevel::INFO);

        HANDLE hFile = CreateFileW(
            path,
            FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            isDirectory ? FILE_FLAG_BACKUP_SEMANTICS : FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (hFile)
        {

            int size = sizeof(CF_PLACEHOLDER_STANDARD_INFO) + 5000;
            CF_PLACEHOLDER_STANDARD_INFO *standard_info = (CF_PLACEHOLDER_STANDARD_INFO *)new BYTE[size];
            DWORD returnlength(0);
            HRESULT hr = CfGetPlaceholderInfo(hFile, CF_PLACEHOLDER_INFO_STANDARD, standard_info, size, &returnlength);
            std::stringstream ss;
            ss << "CfGetPlaceholderInfo: %d\n", hr;
            std::string message = ss.str();
            Logger::getInstance().log(message, LogLevel::INFO);
            if (SUCCEEDED(hr))
            {

                BYTE *FileIdentity = standard_info->FileIdentity;
                size_t identityLength = standard_info->FileIdentityLength;

                // Crear la cadena directamente desde los datos binarios
                std::string fileIdentityString(reinterpret_cast<const char *>(FileIdentity), identityLength);

                return fileIdentityString;
            }
            else
            {
                Logger::getInstance().log("Error likely not a placeholder \n", LogLevel::WARN);
                return "";
            }
            CloseHandle(hFile);
            delete[] standard_info;
        }
        else
        {
            Logger::getInstance().log("Invalid Item: " + Logger::fromWStringToString(path), LogLevel::WARN);
            return "";
        }
    }
    catch (const std::exception &e)
    {
        std::stringstream ss;
        ss << "Excepción capturada: %hs\n", e.what();
        std::string message = ss.str();
        Logger::getInstance().log(message, LogLevel::ERROR);
    }
    catch (...)
    {
        Logger::getInstance().log("Excepción desconocida capturada\n", LogLevel::ERROR);
    }
}

void SyncRoot::DeleteFileSyncRoot(const wchar_t *path)
{
    try
    {
        bool isDirectory = fs::is_directory(path);
        Logger::getInstance().log("Intentando eliminar:" + Logger::fromWStringToString(path), LogLevel::INFO);

        if (isDirectory)
        {
            fs::remove_all(path);
            Logger::getInstance().log("Directorio eliminado con éxito: " + Logger::fromWStringToString(path), LogLevel::INFO);
        }
        else
        {
            if (!DeleteFileW(path))
            {
                Logger::getInstance().log("No se pudo eliminar el archivo: " + Logger::fromWStringToString(path), LogLevel::WARN);
            }
            else
            {
                Logger::getInstance().log("Archivo eliminado con éxito: " + Logger::fromWStringToString(path), LogLevel::INFO);
            }
        }
    }
    catch (const std::exception &e)
    {
        std::stringstream ss;
        ss << "Excepción capturada: %hs\n", e.what();
        std::string message = ss.str();
        Logger::getInstance().log(message, LogLevel::ERROR);
    }
    catch (...)
    {
        Logger::getInstance().log("Excepción desconocida capturada\n", LogLevel::ERROR);
    }
}