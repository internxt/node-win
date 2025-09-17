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

using namespace std;

namespace fs = std::filesystem;

#pragma comment(lib, "shlwapi.lib")

bool DirectoryExists(const wchar_t *path)
{
    DWORD attributes = GetFileAttributesW(path);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

PlaceholderResult Placeholders::CreateOne(
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
    PlaceholderResult result = {false, L""};

    try
    {
        CF_PLACEHOLDER_CREATE_INFO cloudEntry = {};

        std::wstring fullDestPath = std::wstring(destPath) + L'\\';

        wstring fullPath = std::wstring(destPath) + L'\\' + fileName;

        if (std::filesystem::exists(fullPath))
        {
            Placeholders::ConvertToPlaceholder(fullPath, fileIdentity);
            Placeholders::MaintainIdentity(fullPath, fileIdentity, false);
            result.success = true;
            return result;
        }

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
            Placeholders::UpdatePinState(fullPath, PinState::OnlineOnly);
            result.success = true;
        }
        catch (const winrt::hresult_error &error)
        {
            result.errorMessage = error.message().c_str();
            wprintf(L"[CreatePlaceholder] error: %s", error.message().c_str());
        }
        winrt::StorageProviderItemProperty prop;
        prop.Id(1);
        prop.Value(L"Value1");
        prop.IconResource(L"shell32.dll,-44");

        // UpdateSyncStatus(fullDestPath, true, false);
    }
    catch (...)
    {
        result.errorMessage = L"Failed to create or customize placeholder";
        wprintf(L"[CreatePlaceholder] Failed to create or customize placeholder with %08x\n", static_cast<HRESULT>(winrt::to_hresult()));
    }

    return result;
}

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

PlaceholderResult Placeholders::CreateEntry(
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
    PlaceholderResult result = {false, L""};

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
            Placeholders::ConvertToPlaceholder(fullDestPath, itemIdentity);
            Placeholders::MaintainIdentity(fullDestPath, itemIdentity, true);
            result.success = true;
            return result;
        }

        if (isDirectory) // TODO: the function createEntry is used to create only folders (directories), so this if is always true
        {
            // wprintf(L"Create directory, full destination path: %ls, fullDestPath.c_str()");
            PathRemoveFileSpecW(&fullDestPath[0]);
            HRESULT hr = CfCreatePlaceholders(fullDestPath.c_str(), &cloudEntry, 1, CF_CREATE_FLAG_NONE, NULL);
            if (FAILED(hr))
            {
                result.errorMessage = L"Failed to create placeholder directory";
                wprintf(L"[CreatePlaceholder] Failed to create placeholder directory with HRESULT 0x%08x\n", hr);
                throw winrt::hresult_error(hr);
            }

            std::wstring finalPath = std::wstring(destPath) + L"\\" + std::wstring(itemName);
            Placeholders::UpdatePinState(finalPath, PinState::OnlineOnly);
            UpdateSyncStatus(finalPath, true, true);
            result.success = true;
        }
    }
    catch (const winrt::hresult_error &error)
    {
        result.errorMessage = error.message().c_str();
        wprintf(L"[CreatePlaceholder] Error while creating %s: %s\n", isDirectory ? L"directory" : L"file", error.message().c_str());
    }
    catch (...)
    {
        result.errorMessage = L"Unknown error occurred";
        wprintf(L"[CreatePlaceholder] Unknown error occurred\n");
    }

    return result;
}

PlaceholderResult Placeholders::ConvertToPlaceholder(const std::wstring &fullPath, const std::wstring &serverIdentity)
{
    PlaceholderResult result = {false, L""};

    if (!std::filesystem::exists(fullPath))
    {
        result.errorMessage = L"File does not exist";
        return result;
    }

    wprintf(L"[ConvertToPlaceholder] Full path: %ls\n", fullPath.c_str());
    bool isDirectory = fs::is_directory(fullPath);

    HANDLE fileHandle = CreateFileW(
        fullPath.c_str(),
        FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        isDirectory ? FILE_FLAG_BACKUP_SEMANTICS : 0,
        nullptr);

    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        result.errorMessage = L"Failed to open file: " + std::to_wstring(GetLastError());
        return result;
    }

    CF_CONVERT_FLAGS convertFlags = CF_CONVERT_FLAG_MARK_IN_SYNC;
    USN convertUsn;
    OVERLAPPED overlapped = {};

    LPCVOID idStrLPCVOID = static_cast<LPCVOID>(serverIdentity.c_str());
    DWORD idStrByteLength = static_cast<DWORD>(serverIdentity.size() * sizeof(wchar_t));

    HRESULT hr = CfConvertToPlaceholder(fileHandle, idStrLPCVOID, idStrByteLength, convertFlags, &convertUsn, &overlapped);

    if (FAILED(hr))
    {
        CloseHandle(fileHandle);
        if (hr != 0x8007017C) // Ignorar error específico de "ya es un placeholder"
        {
            result.errorMessage = L"Failed to convert to placeholder. HRESULT: 0x" + std::to_wstring(static_cast<unsigned int>(hr));
            return result;
        }
        return result;
    }

    if (!isDirectory)
    {
        HRESULT hrPinState = CfSetPinState(fileHandle, CF_PIN_STATE_PINNED, CF_SET_PIN_FLAG_NONE, nullptr);
        if (FAILED(hrPinState))
        {
            CloseHandle(fileHandle);
            result.errorMessage = L"Failed to set pin state. HRESULT: 0x" + std::to_wstring(static_cast<unsigned int>(hrPinState));
            return result;
        }
    }

    CloseHandle(fileHandle);
    wprintf(L"[ConvertToPlaceholder] Successfully converted to placeholder: %ls\n", fullPath.c_str());
    result.success = true;
    return result;
}

std::wstring GetErrorMessageFromHRESULT(HRESULT hr)
{
    LPWSTR errorMessage = nullptr;
    DWORD result = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        hr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&errorMessage),
        0,
        nullptr);

    std::wstring message;
    if (result > 0 && errorMessage)
    {
        message = errorMessage;
        LocalFree(errorMessage);
    }
    else
    {
        message = L"Error desconocido";
    }

    return message;
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
            ConvertToPlaceholder(filePath, L"temp_identity");
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
        fileState.syncstate = SyncState::Undefined;
        return fileState;
    }

    HRESULT result = CfGetPlaceholderInfo(fileHandle.get(), CF_PLACEHOLDER_INFO_BASIC, info.get(), Utilities::sizeToDWORD(infoSize), nullptr);

    if (result != S_OK)
    {
        printf("CfGetPlaceholderInfo failed with HRESULT %lx\n", result);
        fileState.pinstate = PinState::Unspecified;
        fileState.syncstate = SyncState::Undefined;
        return fileState;
    }

    auto pinStateOpt = info.pinState();
    auto syncStateOpt = info.syncState();

    if (syncStateOpt.has_value())
    {

        SyncState syncState = syncStateOpt.value();
    }

    if (pinStateOpt.has_value())
    {

        PinState pinState = pinStateOpt.value();
    }

    fileState.pinstate = pinStateOpt.value_or(PinState::Unspecified);
    fileState.syncstate = syncStateOpt.value_or(SyncState::Undefined);

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
