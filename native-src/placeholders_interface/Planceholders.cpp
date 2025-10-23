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

void Placeholders::MaintainIdentity(std::wstring &fullPath, PCWSTR itemIdentity, bool isDirectory)
{
    std::string identity = Placeholders::GetFileIdentity(fullPath);
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

/**
 * @brief Mark a file or directory as synchronized
 * @param filePath path to the file or directory
 * @param isDirectory true if the path is a directory, false if it is a file
 * @return void
 */
void Placeholders::UpdateSyncStatus(const std::wstring &filePath,
                                    bool inputSyncState,
                                    bool isDirectory /* = false */)
{
    wprintf(L"[UpdateSyncStatus] Path: %ls\n", filePath.c_str());

    DWORD flags = FILE_FLAG_OPEN_REPARSE_POINT;
    if (isDirectory)
        flags |= FILE_FLAG_BACKUP_SEMANTICS;

    HANDLE h = CreateFileW(filePath.c_str(),
                           FILE_WRITE_ATTRIBUTES,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr,
                           OPEN_EXISTING,
                           flags,
                           nullptr);

    if (h == INVALID_HANDLE_VALUE)
    {
        wprintf(L"[UpdateSyncStatus] CreateFileW falló: %lu\n", GetLastError());
        CloseHandle(h);
        return;
    }

    CF_IN_SYNC_STATE sync = inputSyncState ? CF_IN_SYNC_STATE_IN_SYNC
                                           : CF_IN_SYNC_STATE_NOT_IN_SYNC;

    HRESULT hr = CfSetInSyncState(h, sync, CF_SET_IN_SYNC_FLAG_NONE, nullptr);

    if (FAILED(hr))
    {
        switch (HRESULT_CODE(hr))
        {
        case ERROR_RETRY:
            Sleep(50);
            hr = CfSetInSyncState(h, sync, CF_SET_IN_SYNC_FLAG_NONE, nullptr);
            wprintf(L"[UpdateSyncStatus] Retry CfSetInSyncState\n");
            break;

        case ERROR_CLOUD_FILE_PROVIDER_NOT_RUNNING: // 0x1A94
            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, filePath.c_str(), nullptr);
            wprintf(L"[UpdateSyncStatus] Retry CfSetInSyncState\n");
            break;

        case ERROR_CLOUD_FILE_NOT_IN_SYNC:
            convert_to_placeholder(filePath, L"temp_identity");
            hr = CfSetInSyncState(h, sync, CF_SET_IN_SYNC_FLAG_NONE, nullptr);
            wprintf(L"[UpdateSyncStatus] Retry CfSetInSyncState\n");
            break;

        default:
            wprintf(L"[UpdateSyncStatus] CfSetInSyncState 0x%08X\n", hr);
            break;
        }
    }
    else
    {
        wprintf(L"[UpdateSyncStatus] Estado actualizado\n");
    }

    CloseHandle(h);

    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, filePath.c_str(), nullptr);
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

std::string Placeholders::GetFileIdentity(const std::wstring &filePath)
{
    constexpr auto fileIdMaxLength = 128;
    const auto infoSize = sizeof(CF_PLACEHOLDER_BASIC_INFO) + fileIdMaxLength;
    auto info = PlaceHolderInfo(reinterpret_cast<CF_PLACEHOLDER_BASIC_INFO *>(new char[infoSize]), FileHandle::deletePlaceholderInfo);

    HRESULT result = CfGetPlaceholderInfo(handleForPath(filePath).get(), CF_PLACEHOLDER_INFO_BASIC, info.get(), Utilities::sizeToDWORD(infoSize), nullptr);

    if (result == S_OK)
    {
        BYTE *FileIdentity = info->FileIdentity;
        size_t length = info->FileIdentityLength;

        std::string fileIdentityString(reinterpret_cast<const char *>(FileIdentity), length);
        return fileIdentityString;
    }
    else
    {
        return "";
    }
}

FileState Placeholders::GetPlaceholderInfo(const std::wstring &directoryPath)
{

    constexpr auto fileIdMaxLength = 400;
    const auto infoSize = sizeof(CF_PLACEHOLDER_BASIC_INFO) + fileIdMaxLength;
    auto info = PlaceHolderInfo(reinterpret_cast<CF_PLACEHOLDER_BASIC_INFO *>(new char[infoSize]), FileHandle::deletePlaceholderInfo);

    FileState fileState;
    auto fileHandle = handleForPath(directoryPath);

    if (!fileHandle)
    {
        printf("Error: Invalid file handle.\n");
        fileState.pinstate = PinState::Unspecified;
        return fileState;
    }

    HRESULT result = CfGetPlaceholderInfo(fileHandle.get(), CF_PLACEHOLDER_INFO_BASIC, info.get(), Utilities::sizeToDWORD(infoSize), nullptr);

    if (result != S_OK)
    {
        printf("CfGetPlaceholderInfo failed with HRESULT %lx\n", result);
        fileState.pinstate = PinState::Unspecified;
        return fileState;
    }

    auto pinStateOpt = info.pinState();

    if (pinStateOpt.has_value())
    {

        PinState pinState = pinStateOpt.value();
    }

    fileState.pinstate = pinStateOpt.value_or(PinState::Unspecified);

    return fileState;
}

void Placeholders::ForceShellRefresh(const std::wstring &path)
{
    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, path.c_str(), nullptr);
    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, path.c_str(), nullptr);
}

HRESULT Placeholders::UpdatePinState(const std::wstring &path, const PinState state)
{

    const auto cfState = pinStateToCfPinState(state);
    HRESULT result = CfSetPinState(handleForPath(path).get(), cfState, CF_SET_PIN_FLAG_NONE, nullptr);

    // ForceShellRefresh(path);

    if (result != S_OK)
    {
        Logger::getInstance().log("[UpdatePinState] Error updating pin state.", LogLevel::WARN);
    }

    return result;
}
