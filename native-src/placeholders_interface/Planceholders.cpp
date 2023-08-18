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

        cloudEntry.FileIdentity = fileIdentity;
        cloudEntry.FileIdentityLength = fileIdentityLength;

        cloudEntry.RelativeFileName = fileName;
        cloudEntry.Flags = CF_PLACEHOLDER_CREATE_FLAG_MARK_IN_SYNC;

        cloudEntry.FsMetadata.FileSize.QuadPart = fileSize; // Set the appropriate file size
        cloudEntry.FsMetadata.BasicInfo.FileAttributes = fileAttributes;
        cloudEntry.FsMetadata.BasicInfo.CreationTime = Utilities::FileTimeToLargeInteger(creationTime);
        cloudEntry.FsMetadata.BasicInfo.LastWriteTime = Utilities::FileTimeToLargeInteger(lastWriteTime);
        cloudEntry.FsMetadata.BasicInfo.LastAccessTime = Utilities::FileTimeToLargeInteger(lastAccessTime);
        cloudEntry.FsMetadata.BasicInfo.ChangeTime = Utilities::FileTimeToLargeInteger(lastWriteTime);

        wprintf(L"Creating placeholder for %s\n", fileName);
        wprintf(L"fullDestPath: %s\n", fullDestPath.c_str());

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

        // wprintf(L"\ndestPath: %s\n", destPath);
        // wprintf(L"fileName: %s\n", fileName);

        // wprintf(L"Applying custom state for %s\n", fileName);
        // Utilities::ApplyCustomStateToPlaceholderFile(destPath, fileName, prop);

        wprintf(L"Finish creatiing placeholder\n\n");
    }
    catch (...)
    {
        wprintf(L"Failed to create or customize placeholder with %08x\n", static_cast<HRESULT>(winrt::to_hresult()));
    }
}

void Placeholders::CreateEntry(
    _In_ PCWSTR itemName,
    _In_ PCWSTR itemIdentity,  // No usaremos itemIdentity para crear archivos o carpetas normales
    bool isDirectory,
    uint32_t itemSize,  // No se necesita para la creación de archivos o carpetas
    DWORD itemIdentityLength,  // No se necesita para la creación de archivos o carpetas
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
            // if (!PathFileExistsW(fullDestPath.c_str()))
            // {
                if (!CreateDirectoryW(fullDestPath.c_str(), NULL))
                {
                    throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()));
                }
            // }
        }
        else
        {
            HANDLE hFile = CreateFileW(fullDestPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, itemAttributes, NULL);
            if (hFile == INVALID_HANDLE_VALUE)
            {
                throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()));
            }

            // Configurar los tiempos del archivo si es necesario
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

// void Placeholders::CreateEntry(
//     _In_ PCWSTR itemName,
//     _In_ PCWSTR itemIdentity,
//     bool isDirectory,
//     uint32_t itemSize,
//     DWORD itemIdentityLength,
//     uint32_t itemAttributes,
//     FILETIME creationTime,
//     FILETIME lastWriteTime,
//     FILETIME lastAccessTime,
//     _In_ PCWSTR destPath)
// {
//     try
//     {
//         CF_PLACEHOLDER_CREATE_INFO cloudEntry = {};

//         std::wstring fullDestPath = std::wstring(destPath);
//         if (fullDestPath.back() != L'\\')
//         {
//             fullDestPath.push_back(L'\\');
//         }

//         // cloudEntry.FileIdentity = itemIdentity;
//         // cloudEntry.FileIdentityLength = itemIdentityLength;
//         // cloudEntry.RelativeFileName = itemName;
//         // cloudEntry.Flags = CF_PLACEHOLDER_CREATE_FLAG_MARK_IN_SYNC;

//         // if (isDirectory) {
//         //     cloudEntry.FsMetadata.FileSize.QuadPart = 0;
//         //     cloudEntry.FsMetadata.BasicInfo.FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
//         //     cloudEntry.Flags |= CF_PLACEHOLDER_CREATE_FLAG_DISABLE_ON_DEMAND_POPULATION;
//         // } else {
//         //     cloudEntry.FsMetadata.FileSize.QuadPart = itemSize;
//         // }

//         // cloudEntry.FsMetadata.BasicInfo.FileAttributes = itemAttributes;
//         // cloudEntry.FsMetadata.BasicInfo.CreationTime = Utilities::FileTimeToLargeInteger(creationTime);
//         // cloudEntry.FsMetadata.BasicInfo.LastWriteTime = Utilities::FileTimeToLargeInteger(lastWriteTime);
//         // cloudEntry.FsMetadata.BasicInfo.LastAccessTime = Utilities::FileTimeToLargeInteger(lastAccessTime);
//         // cloudEntry.FsMetadata.BasicInfo.ChangeTime = Utilities::FileTimeToLargeInteger(lastWriteTime);
//         cloudEntry = {}; 
//         cloudEntry.FileIdentity = L"TestIdentity";
//         cloudEntry.FileIdentityLength = wcslen(L"TestIdentity") * sizeof(wchar_t);
//         cloudEntry.RelativeFileName = L"TestFolder";
//         cloudEntry.Flags = CF_PLACEHOLDER_CREATE_FLAG_NONE;
//         cloudEntry.FsMetadata.FileSize.QuadPart = 0;
//         cloudEntry.FsMetadata.BasicInfo.FileAttributes = FILE_ATTRIBUTE_DIRECTORY;

//         wprintf(L"Creating placeholder for %s\n", itemName);
        
//         // Print cloudEntry details for debugging
//         wprintf(L"fullDestPath: %s\n", fullDestPath.c_str());
//         wprintf(L"FileIdentity: %s\n", cloudEntry.FileIdentity);
//         wprintf(L"FileIdentityLength: %d\n", cloudEntry.FileIdentityLength);
//         wprintf(L"RelativeFileName: %s\n", cloudEntry.RelativeFileName);
//         wprintf(L"Flags: %d\n", cloudEntry.Flags);
//         wprintf(L"FileSize: %lld\n", cloudEntry.FsMetadata.FileSize.QuadPart);
//         wprintf(L"FileAttributes: %d\n", cloudEntry.FsMetadata.BasicInfo.FileAttributes);
//         wprintf(L"CreationTime: %lld\n", cloudEntry.FsMetadata.BasicInfo.CreationTime.QuadPart);
//         wprintf(L"LastWriteTime: %lld\n", cloudEntry.FsMetadata.BasicInfo.LastWriteTime.QuadPart);
//         wprintf(L"LastAccessTime: %lld\n", cloudEntry.FsMetadata.BasicInfo.LastAccessTime.QuadPart);
//         wprintf(L"ChangeTime: %lld\n", cloudEntry.FsMetadata.BasicInfo.ChangeTime.QuadPart);
        
//         try
//         {
//             winrt::check_hresult(CfCreatePlaceholders(fullDestPath.c_str(), &cloudEntry, 1, CF_CREATE_FLAG_NONE, NULL));
//         }
//         catch (const winrt::hresult_error &error)
//         {
//             wprintf(L"Error al crear placeholder: %s\n", error.message().c_str());
//             throw error;
//         }

//     }
//     catch (...)
//     {
//         wprintf(L"Failed to create or customize placeholder with %08x\n", static_cast<HRESULT>(winrt::to_hresult()));
//     }
// }