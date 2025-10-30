#include "stdafx.h"
#include "Placeholders.h"
#include "Logger.h"
#include "PlaceholderInfo.h"
#include <winrt/base.h>
#include <shlwapi.h>
#include <vector>
#include <filesystem>
#include <fstream>
#include <random>
#include <iostream>
#include <Utilities.h>
#include <winbase.h>
#include <string>
#include <cctype>
#include <windows.h>
#include <shlobj.h>
#include "convert_to_placeholder.h"

using namespace std;

namespace fs = std::filesystem;

#pragma comment(lib, "shlwapi.lib")

std::string cleanString(const std::string &str)
{
    std::string cleanedStr;
    for (char ch : str)
    {
        if (std::isprint(static_cast<unsigned char>(ch)))
        {
            cleanedStr.push_back(ch);
        }
    }
    return cleanedStr;
}

void Placeholders::MaintainIdentity(const std::wstring &fullPath, PCWSTR itemIdentity, bool isDirectory)
{
    std::string identity = Placeholders::GetPlaceholderInfo(fullPath).placeholderId;
    if (!identity.empty())
    {
        int len = WideCharToMultiByte(CP_UTF8, 0, itemIdentity, -1, NULL, 0, NULL, NULL);
        if (len > 0)
        {
            std::string itemIdentityStr(len, 0);
            WideCharToMultiByte(CP_UTF8, 0, itemIdentity, -1, &itemIdentityStr[0], len, NULL, NULL);
            std::string cleanIdentity = cleanString(identity);
            std::string cleanItemIdentity = cleanString(itemIdentityStr);
            if (cleanIdentity != cleanItemIdentity)
            {
                wprintf(L"[MaintainIdentity] Identity is incorrect, updating...\n");
                std::wstring itemIdentityStrW(itemIdentity);
                Placeholders::UpdateFileIdentity(fullPath, itemIdentityStrW, isDirectory);
            }
        }
        else
        {
            // Handle error as needed
        }
    }
}

void Placeholders::UpdateSyncStatus(const std::wstring &path, bool isDirectory)
{
    DWORD flags = FILE_FLAG_OPEN_REPARSE_POINT;
    if (isDirectory)
        flags |= FILE_FLAG_BACKUP_SEMANTICS;

    winrt::file_handle fileHandle{CreateFileW(
        path.c_str(),
        FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        flags,
        nullptr)};

    if (!fileHandle)
    {
        throw std::runtime_error("Failed to open item: " + std::to_string(GetLastError()));
    }

    winrt::check_hresult(CfSetInSyncState(
        fileHandle.get(),
        CF_IN_SYNC_STATE_IN_SYNC,
        CF_SET_IN_SYNC_FLAG_NONE,
        nullptr));

    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, path.c_str(), nullptr);
}

void Placeholders::UpdateFileIdentity(const std::wstring &filePath, const std::wstring &fileIdentity, bool isDirectory)
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
        DWORD errorCode = GetLastError();
        wprintf(L"[UpdateFileIdentity] Error opening file: %d\n", errorCode);
        LPWSTR errorMessage = nullptr;
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       nullptr,
                       errorCode,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       reinterpret_cast<LPWSTR>(&errorMessage),
                       0,
                       nullptr);
        if (errorMessage)
        {
            wprintf(L"[UpdateFileIdentity] Error: %ls\n", errorMessage);
            LocalFree(errorMessage);
        }
        return;
    }

    HRESULT hr = CfUpdatePlaceholder(
        fileHandle,                                                // Handle del archivo.
        nullptr,                                                   // CF_FS_METADATA opcional.
        fileIdentity.c_str(),                                      // Identidad del archivo.
        static_cast<DWORD>(fileIdentity.size() * sizeof(wchar_t)), // Longitud de la identidad del archivo.
        nullptr,                                                   // Rango a deshidratar, opcional.
        0,                                                         // Conteo de rangos a deshidratar, debe ser 0 si no se usa.
        CF_UPDATE_FLAG_NONE,                                       // Flags de actualización.
        nullptr,                                                   // USN opcional.
        nullptr                                                    // OVERLAPPED opcional.
    );

    if (FAILED(hr))
    {
        std::wstring errorMessage = Utilities::GetErrorMessageCloudFiles(hr);
        wprintf(L"[UpdateFileIdentity] Error updating fileIdentity: %ls\n", errorMessage.c_str());
        CloseHandle(fileHandle);
        return;
    }

    CloseHandle(fileHandle);
}

FileState Placeholders::GetPlaceholderInfo(const std::wstring &path)
{
    constexpr DWORD fileIdMaxLength = 400;
    constexpr DWORD infoSize = sizeof(CF_PLACEHOLDER_BASIC_INFO) + fileIdMaxLength;

    std::vector<char> buffer(infoSize);
    auto *info = reinterpret_cast<CF_PLACEHOLDER_BASIC_INFO *>(buffer.data());

    auto fileHandle = handleForPath(path);
    if (!fileHandle)
    {
        throw std::runtime_error("Failed to get file handle: " + std::to_string(GetLastError()));
    }

    winrt::check_hresult(CfGetPlaceholderInfo(
        fileHandle.get(),
        CF_PLACEHOLDER_INFO_BASIC,
        info,
        infoSize,
        nullptr));

    return FileState{
        std::string(reinterpret_cast<const char *>(info->FileIdentity), info->FileIdentityLength),
        info->PinState};
}
