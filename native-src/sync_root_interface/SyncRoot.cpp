#include "stdafx.h"
#include "SyncRoot.h"
#include "Callbacks.h"
#include <iostream>
#include <iostream>
#include <filesystem>
#include "Logger.h"

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

HRESULT SyncRoot::HydrateFile(const std::wstring& filePath)
{
    DWORD attrib = GetFileAttributesW(filePath.c_str());
    if (!(attrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        winrt::handle placeholder(CreateFileW(filePath.c_str(), 0, FILE_READ_DATA, nullptr, OPEN_EXISTING, 0, nullptr));

        LARGE_INTEGER offset;
        offset.QuadPart = 0;
        LARGE_INTEGER length;
        GetFileSizeEx(placeholder.get(), &length);

        if (attrib & FILE_ATTRIBUTE_PINNED)
        {
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
                    Logger::getInstance().log("Already Hydrated: " + std::to_string(elapsedMilliseconds) + " ms", LogLevel::WARN);
                }
                else
                {
                    Logger::getInstance().log("Hydration finished " + Logger::fromWStringToString(filePath), LogLevel::INFO);
                }
            }
        }
    }
}

HRESULT SyncRoot::DehydrateFile(const std::wstring& filePath)
{
    DWORD attrib = GetFileAttributesW(filePath.c_str());
    if (!(attrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        winrt::handle placeholder(CreateFileW(filePath.c_str(), 0, FILE_READ_DATA, nullptr, OPEN_EXISTING, 0, nullptr));

        LARGE_INTEGER offset;
        offset.QuadPart = 0;
        LARGE_INTEGER length;
        GetFileSizeEx(placeholder.get(), &length);

        if (attrib & FILE_ATTRIBUTE_UNPINNED)
        {
            Logger::getInstance().log("Dehydrating file " + Logger::fromWStringToString(filePath), LogLevel::INFO);
            HRESULT hr = CfDehydratePlaceholder(placeholder.get(), offset, length, CF_DEHYDRATE_FLAG_NONE, NULL);

            if (FAILED(hr))
            {
                Logger::getInstance().log("Error dehydrating file " + Logger::fromWStringToString(filePath), LogLevel::ERROR);
            }
            else
            {
                Logger::getInstance().log("Dehydration finished " + Logger::fromWStringToString(filePath), LogLevel::INFO);
            }
        }
    }
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

void EnumerateAndQueryPlaceholders(const std::wstring &directoryPath, std::list<ItemInfo> &itemInfo)
{
    int count = 0;
    // iterator
    for (const auto &entry : std::filesystem::directory_iterator(directoryPath))
    {
        printf("Entry: %ls\n", entry.path().c_str());
        // bool isDirectory = entry.is_directory();
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
                    printf("FileIdentity: %ls\n", fileIdentityString.c_str());
                    item.fileIdentity = fileIdentityString;
                    item.isPlaceholder = true;
                    itemInfo.push_back(item);
                }
                else
                {
                    printf("Error likely not a placeholder \n");
                    item.fileIdentity = L"";
                    item.isPlaceholder = false;
                }
                // limpiar memoria para repetir el proceso en el for
                CloseHandle(hFile);

                if (isDirectory)
                {
                    printf("IsDirectory: ");
                    EnumerateAndQueryPlaceholders(entry.path().c_str(), itemInfo);
                }
            }
            else
            {
                wprintf(L"Invalid Item: %ls\n", entry.path().c_str());
            }
            // CloseHandle(hFile);
            // hFile = INVALID_HANDLE_VALUE;
        }
        catch (const std::exception &ex)
        {
            // Manejar excepciones estándar de C++
            printf("Exception occurred: %s\n", ex.what());
        }
        catch (...)
        {
            // Manejar cualquier otra excepción
            printf("Unknown exception occurred.\n");
        }

        count++;
    }
    printf("Count items: %d\n", count);
    return;
}

// get items sync root
std::list<ItemInfo> SyncRoot::GetItemsSyncRoot(const wchar_t *syncRootPath)
{
    try
    {
        printf("[Start] GetItemsSyncRoot\n");
        std::list<ItemInfo> itemInfo;
        EnumerateAndQueryPlaceholders(syncRootPath, itemInfo);
        // print first file id
        printf("[End] GetItemsSyncRoot\n");
        return itemInfo;
    }
    catch (const std::exception &e)
    {
        wprintf(L"Excepción capturada: %hs\n", e.what());
    }
}

// get fileIdentity by path
std::string SyncRoot::GetFileIdentity(const wchar_t *path)
{
    try
    {
        printf("GetFileIdentity: %ls\n", path);
        bool isDirectory = fs::is_directory(path);
        printf("IsDirectory: %d\n", isDirectory);
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
            printf("CfGetPlaceholderInfo: %d\n", hr);
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
                printf("Error likely not a placeholder \n");
                return "";
            }
            CloseHandle(hFile);
            delete[] standard_info;
        }
        else
        {
            wprintf(L"Invalid Item: %ls\n", path);
            return "";
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