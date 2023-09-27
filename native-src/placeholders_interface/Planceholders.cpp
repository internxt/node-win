#include "stdafx.h"
#include "Placeholders.h"
#include <winrt/base.h>
#include <shlwapi.h>
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
    }
    catch (...)
    {
        wprintf(L"Failed to create or customize placeholder with %08x\n", static_cast<HRESULT>(winrt::to_hresult()));
    }
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
    try
    {
        if (isDirectory) // TODO: the function createEntry is used to create only folders (directories), so this if is always true
        {
            wprintf(L"create placeholder folder: \n");
            PathRemoveFileSpecW(&fullDestPath[0]);
            wprintf(L"Full destination path: %ls\n", fullDestPath.c_str());
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

            // HANDLE fileHandle = CreateFileW(
            //     fullDestPath.c_str(),
            //     FILE_READ_DATA | WRITE_DAC,
            //     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            //     nullptr,
            //     OPEN_EXISTING,
            //     FILE_FLAG_BACKUP_SEMANTICS,
            //     nullptr
            // );

            // if (fileHandle == INVALID_HANDLE_VALUE)
            // {
            //     wprintf(L"Error al abrir el archivo: %d\n", GetLastError());
            //     return;
            // }

            // // https://learn.microsoft.com/en-us/windows/win32/api/cfapi/nf-cfapi-cfsetinsyncstate
            // // https://learn.microsoft.com/en-us/windows/win32/api/cfapi/ne-cfapi-cf_in_sync_state
            // hr = CfSetInSyncState(fileHandle, CF_IN_SYNC_STATE_IN_SYNC, CF_SET_IN_SYNC_FLAG_NONE, nullptr );
            // if (FAILED(hr))
            // {
            //     wprintf(L"Error al establecer el estado de sincronización: %ld\n", hr);
            // }

        }

        wprintf(L"Successfully created %s at %s\n", isDirectory ? L"directory" : L"file", fullDestPath.c_str());
    }
    catch (const winrt::hresult_error &error)
    {
        wprintf(L"Error while creating %s: %s\n", isDirectory ? L"directory" : L"file", error.message().c_str());
    }
}

void Placeholders::MarkItemAsSync(const std::wstring &filePath, bool isDirectory = false)
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
    HRESULT hr = CfSetInSyncState(fileHandle, CF_IN_SYNC_STATE_IN_SYNC, CF_SET_IN_SYNC_FLAG_NONE, nullptr);
    if (FAILED(hr))
    {
        wprintf(L"Error al establecer el estado de sincronización: %ld\n", hr);
    }

    CloseHandle(fileHandle);
}