#include "stdafx.h"
#include "Placeholders.h"
#include <winrt/base.h>
#include <shlwapi.h>

void Placeholders::CreateOne(
    _In_ PCWSTR fileName,
    _In_ PCWSTR fileIdentity,
    uint32_t fileSize,
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

        std::wstring fullDestPath = std::wstring(destPath) + L'\\'; // fileName;


        std::wstring relativeName(fileIdentity);
        
        cloudEntry.FileIdentity = relativeName.c_str();
        cloudEntry.FileIdentityLength = static_cast<DWORD>((relativeName.size() + 1) * sizeof(WCHAR));

        cloudEntry.RelativeFileName = fileName;
        cloudEntry.Flags = CF_PLACEHOLDER_CREATE_FLAG_MARK_IN_SYNC;

        cloudEntry.FsMetadata.FileSize.QuadPart = fileSize; // Set the appropriate file size
        cloudEntry.FsMetadata.BasicInfo.FileAttributes = fileAttributes;
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
    
    try
    {
        if (isDirectory)
        {
                if (!CreateDirectoryW(fullDestPath.c_str(), NULL))
                {
                    throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()));
                }
        }
        else
        {
            HANDLE hFile = CreateFileW(fullDestPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, itemAttributes, NULL);
            if (hFile == INVALID_HANDLE_VALUE)
            {
                throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()));
            }

            SetFileTime(hFile, &creationTime, &lastAccessTime, &lastWriteTime);
            
            CloseHandle(hFile);
        }

        wprintf(L"Successfully created %s at %s\n", isDirectory ? L"directory" : L"file", fullDestPath.c_str());
    }
    catch (const winrt::hresult_error &error)
    {
        wprintf(L"Error while creating %s: %s\n", isDirectory ? L"directory" : L"file", error.message().c_str());
    }
}