#include "stdafx.h"
#include "Placeholders.h"
#include <winrt/base.h>
#include <shlwapi.h>
#include "SyncRootWatcher.h"
#include <vector>
#include <filesystem>
#include <fstream>
#include <random>
#include <iostream>

using namespace std;

#pragma comment(lib, "shlwapi.lib")

void Placeholders::CreateOne(
    _In_ PCWSTR fileName,
    _In_ PCWSTR fileIdentity,
    int64_t fileSize,
    DWORD fileIdentityLength,
    uint32_t fileAttributes,
    FILETIME creationTime,
    FILETIME lastWriteTime,
    FILETIME lastAccessTime,
    _In_ PCWSTR destPath)
{
    try
    {
        CF_PLACEHOLDER_CREATE_INFO cloudEntry = {};

        std::wstring fullDestPath = std::wstring(destPath) + L'\\';

        std::wstring relativeName(fileIdentity);

        cloudEntry.FileIdentity = relativeName.c_str();
        cloudEntry.FileIdentityLength = static_cast<DWORD>((relativeName.size() + 1) * sizeof(WCHAR));

        cloudEntry.RelativeFileName = fileName;
        cloudEntry.Flags = CF_PLACEHOLDER_CREATE_FLAG_MARK_IN_SYNC;

        cloudEntry.FsMetadata.FileSize.QuadPart = fileSize;
        cloudEntry.FsMetadata.BasicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;
        cloudEntry.FsMetadata.BasicInfo.CreationTime = Utilities::FileTimeToLargeInteger(creationTime);
        cloudEntry.FsMetadata.BasicInfo.LastWriteTime = Utilities::FileTimeToLargeInteger(lastWriteTime);
        cloudEntry.FsMetadata.BasicInfo.LastAccessTime = Utilities::FileTimeToLargeInteger(lastAccessTime);
        cloudEntry.FsMetadata.BasicInfo.ChangeTime = Utilities::FileTimeToLargeInteger(lastWriteTime);

        try
        {
            winrt::check_hresult(CfCreatePlaceholders(fullDestPath.c_str(), &cloudEntry, 1, CF_CREATE_FLAG_NONE, NULL));
        }
        catch (const winrt::hresult_error &error)
        {
            wprintf(L"Error al crear placeholder: %s", error.message().c_str());
        }
        winrt::StorageProviderItemProperty prop;
        prop.Id(1);
        prop.Value(L"Value1");
        prop.IconResource(L"shell32.dll,-44");

        wprintf(L"Successfully created placeholder file\n");
        UpdateSyncStatus(fullDestPath, true);
    }
    catch (...)
    {
        wprintf(L"Failed to create or customize placeholder with %08x\n", static_cast<HRESULT>(winrt::to_hresult()));
    }
}

bool DirectoryExists(const wchar_t *path)
{
    DWORD attributes = GetFileAttributesW(path);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

void Placeholders::CreateEntry(
    _In_ PCWSTR itemName,
    _In_ PCWSTR itemIdentity,
    bool isDirectory,
    uint32_t itemSize,
    DWORD itemIdentityLength,
    uint32_t itemAttributes,
    FILETIME creationTime,
    FILETIME lastWriteTime,
    FILETIME lastAccessTime,
    _In_ PCWSTR destPath)
{

    std::wstring fullDestPath = std::wstring(destPath) + L"\\" + std::wstring(itemName);
    CF_PLACEHOLDER_CREATE_INFO cloudEntry = {};
    std::wstring relativeName(itemIdentity);
    cloudEntry.FileIdentity = relativeName.c_str();
    cloudEntry.FileIdentityLength = static_cast<DWORD>((relativeName.size() + 1) * sizeof(WCHAR));
    cloudEntry.RelativeFileName = itemName;
    cloudEntry.Flags = CF_PLACEHOLDER_CREATE_FLAG_DISABLE_ON_DEMAND_POPULATION; // -> desactive download on demand
    cloudEntry.FsMetadata.BasicInfo.FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    cloudEntry.FsMetadata.BasicInfo.CreationTime = Utilities::FileTimeToLargeInteger(creationTime);
    cloudEntry.FsMetadata.BasicInfo.LastWriteTime = Utilities::FileTimeToLargeInteger(lastWriteTime);
    try
    {
        // TODO: si existe o es placeholder return
        if (DirectoryExists(fullDestPath.c_str()))
        {
            wprintf(L"El directorio ya existe. Se omite la creación.\n");
            return; // No hacer nada si ya existe
        }

        if (isDirectory) // TODO: the function createEntry is used to create only folders (directories), so this if is always true
        {
            // wprintf(L"Create directory, full destination path: %ls, fullDestPath.c_str()");
            PathRemoveFileSpecW(&fullDestPath[0]);
            HRESULT hr = CfCreatePlaceholders(fullDestPath.c_str(), &cloudEntry, 1, CF_CREATE_FLAG_NONE, NULL);
            if (FAILED(hr))
            {
                wprintf(L"Failed to create placeholder directory with HRESULT 0x%08x\n", hr);
                throw winrt::hresult_error(hr);
            }
            else
            {
                wprintf(L"Successfully created placeholder directory\n");
            }

            std::wstring finalPath = std::wstring(destPath) + L"\\" + std::wstring(itemName);
            UpdateSyncStatus(finalPath, true, true);
        }

        wprintf(L"Successfully created %s at %s\n", isDirectory ? L"directory" : L"file", fullDestPath.c_str());
    }
    catch (const winrt::hresult_error &error)
    {
        wprintf(L"Error while creating %s: %s\n", isDirectory ? L"directory" : L"file", error.message().c_str());
    }
}

/**
 * @brief Mark a file or directory as synchronized
 * @param filePath path to the file or directory
 * @param isDirectory true if the path is a directory, false if it is a file
 * @return void
 */
void Placeholders::UpdateSyncStatus(const std::wstring &filePath, bool inputSyncState, bool isDirectory = false)
{

    HANDLE fileHandle = CreateFileW(
        filePath.c_str(),
        FILE_WRITE_ATTRIBUTES, // permisson needed to change the state
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        isDirectory ? FILE_FLAG_BACKUP_SEMANTICS : FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        wprintf(L"Error al abrir el archivo: %d\n", GetLastError());
        return;
    }

    // https://learn.microsoft.com/en-us/windows/win32/api/cfapi/nf-cfapi-cfsetinsyncstate
    // https://learn.microsoft.com/en-us/windows/win32/api/cfapi/ne-cfapi-cf_in_sync_state
    CF_IN_SYNC_STATE syncState = inputSyncState ? CF_IN_SYNC_STATE_IN_SYNC : CF_IN_SYNC_STATE_NOT_IN_SYNC;
    // wprintf(L"Marking item as %s: %ls\n", inputSyncState ? L"IN_SYNC" : L"NOT_IN_SYNC", filePath.c_str());
    HRESULT hr = CfSetInSyncState(fileHandle, syncState, CF_SET_IN_SYNC_FLAG_NONE, nullptr);
    // imprimir hresult
    wprintf(L"hr: %ld\n", hr);
    if (FAILED(hr))
    {
        wprintf(L"Error al establecer el estado de sincronización: %ld\n", hr);
    }

    CloseHandle(fileHandle);
}

CF_PLACEHOLDER_STATE Placeholders::GetPlaceholderState(const std::wstring &filePath)
{
    HANDLE fileHandle = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        // Error al abrir el archivo
        return CF_PLACEHOLDER_STATE_INVALID;
    }

    FILE_BASIC_INFO fileBasicInfo;
    if (!GetFileInformationByHandleEx(fileHandle, FileBasicInfo, &fileBasicInfo, sizeof(fileBasicInfo)))
    {
        // Error al obtener la información básica del archivo
        CloseHandle(fileHandle);
        return CF_PLACEHOLDER_STATE_INVALID;
    }

    CF_PLACEHOLDER_STATE placeholderState = CfGetPlaceholderStateFromFileInfo(&fileBasicInfo, FileBasicInfo);
    // Logger::getInstance().log("placeholderState: %d" + placeholderState, LogLevel::DEBUG);
    // printf("placeholderState: %d\n", placeholderState);
    CloseHandle(fileHandle);

    return placeholderState;
}

CF_PLACEHOLDER_STATE GetPlaceholderStateMock(const std::wstring &filePath)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> dis(0, 1);

    if (dis(gen) == 0)
    {
        return CF_PLACEHOLDER_STATE_IN_SYNC;
    }
    else
    {
        return CF_PLACEHOLDER_STATE_SYNC_ROOT;
    }
}

std::vector<std::wstring> Placeholders::GetPlaceholderWithStatePending(const std::wstring &directoryPath)
{
    std::vector<std::wstring> resultPaths;

    for (const auto &entry : std::filesystem::directory_iterator(directoryPath))
    {
        const auto &path = entry.path().wstring();

        if (entry.is_regular_file())
        {
            // Verifica el estado del placeholder y las condiciones adicionales
            CF_PLACEHOLDER_STATE placeholderState = GetPlaceholderStateMock(path);
            if (placeholderState == CF_PLACEHOLDER_STATE_IN_SYNC &&
                IsFileValidForSync(path))
            {
                resultPaths.push_back(path);
            }
        }
        else if (entry.is_directory())
        {
            // Verifica el estado del directorio y las condiciones adicionales
            CF_PLACEHOLDER_STATE folderState = GetPlaceholderStateMock(path);
            if (folderState == CF_PLACEHOLDER_STATE_IN_SYNC)
            {
                std::vector<std::wstring> subfolderPaths = GetPlaceholderWithStatePending(path);
                resultPaths.insert(resultPaths.end(), subfolderPaths.begin(), subfolderPaths.end());
            }
        }
    }

    return resultPaths;
}

bool Placeholders::IsFileValidForSync(const std::wstring& filePath) {
    // Obtener un handle al archivo
    HANDLE fileHandle = CreateFileW(
        filePath.c_str(),
        FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (fileHandle == INVALID_HANDLE_VALUE) {
        // No se pudo abrir el archivo
        return false;
    }

    // Verificar si el archivo está vacío
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(fileHandle, &fileSize)) {
        CloseHandle(fileHandle);
        return false;
    }

    if (fileSize.QuadPart == 0) {
        CloseHandle(fileHandle);
        return false;
    }

    // Verificar el tamaño máximo del archivo
    const int64_t maxFileSize = 20 * 1024 * 1024 * 1024; // 20GB
    if (fileSize.QuadPart > maxFileSize) {
        CloseHandle(fileHandle);
        return false;
    }

    // Verificar la extensión del archivo
    if (std::filesystem::path(filePath).extension().empty()) {
        CloseHandle(fileHandle);
        return false;
    }

    // Cerrar el handle del archivo
    CloseHandle(fileHandle);

    return true;
}
