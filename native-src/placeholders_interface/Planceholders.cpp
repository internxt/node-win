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

using namespace std;

namespace fs = std::filesystem;

#pragma comment(lib, "shlwapi.lib")

bool DirectoryExists(const wchar_t *path)
{
    DWORD attributes = GetFileAttributesW(path);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

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

        wstring fullPath = std::wstring(destPath) + L'\\' + fileName;

        wprintf(L"[CreatePlaceholder] Full path: %s", fullPath.c_str());
        wprintf(L"\n");

        if (std::filesystem::exists(fullPath))
        {
            Placeholders::ConvertToPlaceholder(fullPath, fileIdentity);
            wprintf(L"[CreatePlaceholder] File already exists. Skipping creation. Initializing identity maintenance...\n");
            Placeholders::MaintainIdentity(fullPath, fileIdentity, false);
            return; // No hacer nada si ya existe
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
        }
        catch (const winrt::hresult_error &error)
        {
            wprintf(L"[CreatePlaceholder] error: %s", error.message().c_str());
        }
        winrt::StorageProviderItemProperty prop;
        prop.Id(1);
        prop.Value(L"Value1");
        prop.IconResource(L"shell32.dll,-44");

        wprintf(L"[CreatePlaceholder] Successfully created placeholder file\n");
        // UpdateSyncStatus(fullDestPath, true, false);
    }
    catch (...)
    {
        wprintf(L"[CreatePlaceholder] Failed to create or customize placeholder with %08x\n", static_cast<HRESULT>(winrt::to_hresult()));
    }
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
        wprintf(L"[MaintainIdentity] Identity is not empty\n");
        int len = WideCharToMultiByte(CP_UTF8, 0, itemIdentity, -1, NULL, 0, NULL, NULL);
        if (len > 0)
        {
            std::string itemIdentityStr(len, 0);
            WideCharToMultiByte(CP_UTF8, 0, itemIdentity, -1, &itemIdentityStr[0], len, NULL, NULL);
            std::string cleanIdentity = cleanString(identity);
            std::string cleanItemIdentity = cleanString(itemIdentityStr);
            printf("[MaintainIdentity] cleanCurrentIdentity: %s\n", cleanIdentity.c_str());
            printf("[MaintainIdentity] cleanItemIdentity: %s\n", cleanItemIdentity.c_str());
            if (cleanIdentity == cleanItemIdentity)
            {
                wprintf(L"[MaintainIdentity] Identity is corret not need to update\n");
            }
            else
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
    printf("[MaintainIdentity] End Maintain Identity\n");
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
            Placeholders::ConvertToPlaceholder(fullDestPath, itemIdentity);
            wprintf(L"[CreatePlaceholder] Directory already exists. Skipping creation. Initializing identity maintenance...\n");
            Placeholders::MaintainIdentity(fullDestPath, itemIdentity, true);
            return; // No hacer nada si ya existe
        }

        if (isDirectory) // TODO: the function createEntry is used to create only folders (directories), so this if is always true
        {
            // wprintf(L"Create directory, full destination path: %ls, fullDestPath.c_str()");
            PathRemoveFileSpecW(&fullDestPath[0]);
            HRESULT hr = CfCreatePlaceholders(fullDestPath.c_str(), &cloudEntry, 1, CF_CREATE_FLAG_NONE, NULL);
            if (FAILED(hr))
            {
                wprintf(L"[CreatePlaceholder] Failed to create placeholder directory with HRESULT 0x%08x\n", hr);
                throw winrt::hresult_error(hr);
            }
            else
            {
                wprintf(L"[CreatePlaceholder] Successfully created placeholder directory\n");
            }

            std::wstring finalPath = std::wstring(destPath) + L"\\" + std::wstring(itemName);
            Placeholders::UpdatePinState(finalPath, PinState::OnlineOnly);
            UpdateSyncStatus(finalPath, true, true);
        }

        wprintf(L"[CreatePlaceholder] Successfully created %s at %s\n", isDirectory ? L"directory" : L"file", fullDestPath.c_str());
    }
    catch (const winrt::hresult_error &error)
    {
        wprintf(L"[CreatePlaceholder] Error while creating %s: %s\n", isDirectory ? L"directory" : L"file", error.message().c_str());
    }
}

bool Placeholders::ConvertToPlaceholder(const std::wstring &fullPath, const std::wstring &serverIdentity)
{
    try
    {
        if (!std::filesystem::exists(fullPath))
        {
            // El archivo no existe
            wprintf(L"[ConvertToPlaceholder] File does not exist\n");
            return false;
        }

        // Obtener un handle al archivo
        bool isDirectory = fs::is_directory(fullPath);

        // Obtener un handle al archivo o carpeta
        HANDLE fileHandle = CreateFileW(
            fullPath.c_str(),
            FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            isDirectory ? FILE_FLAG_BACKUP_SEMANTICS : 0, // Agregar FILE_FLAG_BACKUP_SEMANTICS si es una carpeta
            nullptr);

        if (fileHandle == INVALID_HANDLE_VALUE)
        {
            // Manejar el error al abrir el archivo
            wprintf(L"[ConvertToPlaceholder] Error opening file: %d\n", GetLastError());
            return false;
        }

        CF_CONVERT_FLAGS convertFlags = CF_CONVERT_FLAG_MARK_IN_SYNC;
        USN convertUsn;
        OVERLAPPED overlapped = {};

        // Convierte la cadena de la identidad del servidor a LPCVOID
        LPCVOID idStrLPCVOID = static_cast<LPCVOID>(serverIdentity.c_str());
        DWORD idStrByteLength = static_cast<DWORD>(serverIdentity.size() * sizeof(wchar_t));

        HRESULT hr = CfConvertToPlaceholder(fileHandle, idStrLPCVOID, idStrByteLength, convertFlags, &convertUsn, &overlapped);

        if (FAILED(hr))
        {
            // Manejar el error al convertir a marcador de posición
            wprintf(L"[ConvertToPlaceholder] Error converting to placeholder, ConvertToPlaceholder failed with HRESULT 0x%X\n", hr);

            // Puedes obtener información detallada sobre el error usando FormatMessage
            LPVOID errorMsg;
            FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                hr,
                0, // Default language
                (LPWSTR)&errorMsg,
                0,
                NULL);

            wprintf(L"[ConvertToPlaceholder] Error details: %s\n", errorMsg);

            // Liberar el buffer de mensaje de error
            LocalFree(errorMsg);

            return false;
        }

        if (!isDirectory)
        {
            CfSetPinState(fileHandle, CF_PIN_STATE_PINNED, CF_SET_PIN_FLAG_NONE, nullptr);
        }

        CloseHandle(fileHandle);
        wprintf(L"[ConvertToPlaceholder] Successfully converted to placeholder: %ls\n", fullPath.c_str());
        return true;
    }
    catch (const winrt::hresult_error &error)
    {
        // Manejar excepciones desconocidas
        wprintf(L"[ConvertToPlaceholder] Unknown exception occurred\n");
        return false;
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
    wprintf(L"[UpdateSyncStatus] Path: %ls\n", filePath.c_str());
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
        wprintf(L"[UpdateSyncStatus] Error al abrir el archivo: %d\n", GetLastError());
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
        wprintf(L"[UpdateSyncStatus] Error al establecer el estado de sincronización: %ld\n", hr);
    }

    CloseHandle(fileHandle);
}

void Placeholders::UpdateFileIdentity(const std::wstring &filePath, const std::wstring &fileIdentity, bool isDirectory)
{
    wprintf(L"[UpdateFileIdentity] Path: %ls\n", filePath.c_str());
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
        wprintf(L"[UpdateFileIdentity] Error updating fileIdentity: 0x%08X\n", hr);
        LPWSTR errorMessage = nullptr;
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       nullptr,
                       HRESULT_CODE(hr),
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       reinterpret_cast<LPWSTR>(&errorMessage),
                       0,
                       nullptr);
        if (errorMessage)
        {
            wprintf(L"[UpdateFileIdentity] Error: %ls\n", errorMessage);
            LocalFree(errorMessage);
        }
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
    CloseHandle(fileHandle);

    return placeholderState;
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

    Logger::getInstance().log("Getting placeholder info for path: " + Logger::fromWStringToString(directoryPath), LogLevel::INFO, FOREGROUND_RED);
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

        printf("placeholderInfo.SyncState: %s\n", syncStateToString(syncState).c_str());
    }
    else
    {
        printf("placeholderInfo.SyncState: No value\n");
    }

    if (pinStateOpt.has_value())
    {

        PinState pinState = pinStateOpt.value();

        printf("placeholderInfo.PinState: %s\n", pinStateToString(pinState).c_str());
    }
    else
    {
        printf("placeholderInfo.PinState: No value\n");
    }

    fileState.pinstate = pinStateOpt.value_or(PinState::Unspecified);
    fileState.syncstate = syncStateOpt.value_or(SyncState::Undefined);

    return fileState;
}

std::vector<std::wstring> Placeholders::GetPlaceholderWithStatePending(const std::wstring &directoryPath)
{
    std::vector<std::wstring> resultPaths;

    for (const auto &entry : std::filesystem::directory_iterator(directoryPath))
    {
        const auto &path = entry.path().wstring();

        if (entry.is_directory())
        {
            std::vector<std::wstring> subfolderPaths = GetPlaceholderWithStatePending(path);
            resultPaths.insert(resultPaths.end(), subfolderPaths.begin(), subfolderPaths.end());
        }
        else if (entry.is_regular_file())
        {
            FileState placeholderState = Placeholders::GetPlaceholderInfo(path);
            bool isFileValidForSync = (placeholderState.syncstate == SyncState::Undefined || placeholderState.syncstate == SyncState::NotInSync);
            if (isFileValidForSync && IsFileValidForSync(path))
            {
                resultPaths.push_back(path);
            }
        }
    }

    return resultPaths;
}

bool Placeholders::IsFileValidForSync(const std::wstring &filePath)
{
    // Obtener un handle al archivo
    HANDLE fileHandle = CreateFileW(
        filePath.c_str(),
        FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        // No se pudo abrir el archivo
        return false;
    }

    // Verificar si el archivo está vacío
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(fileHandle, &fileSize))
    {
        CloseHandle(fileHandle);
        return false;
    }

    if (fileSize.QuadPart == 0)
    {
        CloseHandle(fileHandle);
        return false;
    }

    LARGE_INTEGER maxFileSize;
    maxFileSize.QuadPart = 20LL * 1024 * 1024 * 1024; // 20GB

    if (fileSize.QuadPart > maxFileSize.QuadPart)
    {
        CloseHandle(fileHandle);
        return false;
    }

    // // Verificar la extensión del archivo
    if (std::filesystem::path(filePath).extension().empty())
    {
        CloseHandle(fileHandle);
        return false;
    }

    // Cerrar el handle del archivo
    CloseHandle(fileHandle);

    return true;
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
    
    ForceShellRefresh(path);

    if (result != S_OK)
    {
        Logger::getInstance().log("[UpdatePinState] Error updating pin state.", LogLevel::WARN);
    }

    return result;
}

PlaceholderAttribute Placeholders::GetAttribute(const std::wstring &filePath)
{
    DWORD attrib = GetFileAttributesW(filePath.c_str());
    if (!(attrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        winrt::handle placeholder(CreateFileW(filePath.c_str(), 0, FILE_READ_DATA, nullptr, OPEN_EXISTING, 0, nullptr));

        LARGE_INTEGER offset;
        offset.QuadPart = 0;
        LARGE_INTEGER length;
        GetFileSizeEx(placeholder.get(), &length);
        // length.QuadPart = MAXLONGLONG;
        // bool isHydrated = fileState.pinstate == PinState::AlwaysLocal && fileState.syncstate == SyncState::InSync;
        if (attrib & FILE_ATTRIBUTE_PINNED) // && !(isHydrated)
        {
            Logger::getInstance().log("Attribute: PINNED", LogLevel::INFO);

            return PlaceholderAttribute::PINNED;
        }
        else if (attrib & FILE_ATTRIBUTE_UNPINNED)
        {
            Logger::getInstance().log("Attribute: NO PINNED", LogLevel::INFO);

            return PlaceholderAttribute::NOT_PINNED;
        }
        Logger::getInstance().log("Attribute: Other", LogLevel::INFO);

        return PlaceholderAttribute::OTHER;
    }

    Logger::getInstance().log("Attribute: Other", LogLevel::DEBUG);

    return PlaceholderAttribute::OTHER;
}