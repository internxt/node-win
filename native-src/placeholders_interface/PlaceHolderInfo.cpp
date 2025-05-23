#include "PlaceHolderInfo.h"
#include <comdef.h>
#include <windows.h>
#include <filesystem>
#include <codecvt>
#include <locale>
#include "Logger.h"

SyncState cfSyncStateToSyncState(CF_IN_SYNC_STATE state)
{
    switch (state)
    {
    case CF_IN_SYNC_STATE_NOT_IN_SYNC:
        return SyncState::NotInSync;
    case CF_IN_SYNC_STATE_IN_SYNC:
        return SyncState::InSync;
    default:
        return SyncState::NotInSync;
    }
}

std::string syncStateToString(SyncState state)
{
    switch (state)
    {
    case SyncState::NotInSync:
        return "NotInSync";
    case SyncState::InSync:
        return "InSync";
    default:
        return "Unknown";
    }
}

PinState cfPinStateToPinState(CF_PIN_STATE state)
{
    switch (state)
    {
    case CF_PIN_STATE_UNSPECIFIED:
        return PinState::Unspecified;
    case CF_PIN_STATE_PINNED:
        return PinState::AlwaysLocal;
    case CF_PIN_STATE_UNPINNED:
        return PinState::OnlineOnly;
    case CF_PIN_STATE_INHERIT:
        return PinState::Inherited;
    case CF_PIN_STATE_EXCLUDED:
        return PinState::Excluded;
    default:
        return PinState::Inherited;
    }
}

CF_PIN_STATE pinStateToCfPinState(PinState state)
{
    switch (state)
    {
    case PinState::Unspecified:
        return CF_PIN_STATE_UNSPECIFIED;
    case PinState::AlwaysLocal:
        return CF_PIN_STATE_PINNED;
    case PinState::OnlineOnly:
        return CF_PIN_STATE_UNPINNED;
    case PinState::Inherited:
        return CF_PIN_STATE_INHERIT;
    case PinState::Excluded:
        return CF_PIN_STATE_EXCLUDED;
    default:
        return CF_PIN_STATE_INHERIT;
    }
}

std::string pinStateToString(PinState state)
{
    switch (state)
    {
    case PinState::Inherited:
        return "Inherited";
    case PinState::AlwaysLocal:
        return "AlwaysLocal";
    case PinState::OnlineOnly:
        return "OnlineOnly";
    case PinState::Unspecified:
        return "Unspecified";
    case PinState::Excluded:
        return "Excluded";
    default:
        return "Unknown";
    }
}

PlaceHolderInfo::PlaceHolderInfo()
    : _data(nullptr, [](CF_PLACEHOLDER_BASIC_INFO *) {})
{
}

PlaceHolderInfo::PlaceHolderInfo(CF_PLACEHOLDER_BASIC_INFO *data, Deleter deleter)
    : _data(data, deleter)
{
}

std::optional<PinState> PlaceHolderInfo::pinState() const
{
    if (!_data)
    {
        return {};
    }

    return cfPinStateToPinState(_data->PinState);
}

std::optional<SyncState> PlaceHolderInfo::syncState() const
{
    if (!_data)
    {
        return {};
    }

    return cfSyncStateToSyncState(_data->InSyncState);
}

std::optional<LARGE_INTEGER> PlaceHolderInfo::FileId() const
{
    if (!_data)
    {
        return {};
    }
    return _data->FileId;
}

std::optional<BYTE> PlaceHolderInfo::FileIdentity() const
{
    if (!_data)
    {
        return {};
    }

    printf("FILE OPTIONAL: %d\n", _data->FileIdentity[0]);
    return _data->FileIdentity[0]; // Devuelve el primer byte del array
}

void FileHandle::deletePlaceholderInfo(CF_PLACEHOLDER_BASIC_INFO *info)
{
    auto byte = reinterpret_cast<char *>(info);
    delete[] byte;
}

FileHandle::FileHandle()
    : _data(
          nullptr, [](void *) {})
{
}

FileHandle::FileHandle(void *data, Deleter deleter)
    : _data(data, deleter)
{
}

FileHandle handleForPath(const std::wstring &wPath)
{
    if (wPath.empty())
    {
        return {};
    }

    /**
     *  v1.0.9 Jonathan Arce
     *
     * We directly use the wPath parameter in handleForPath for several important reasons:
     *
     * 1. Performance optimization: Using wPath directly avoids unnecessary string conversions
     *    between wide strings and UTF-8/ANSI, which would be costly for file operations.
     *
     * 2. Unicode support: Windows APIs like CfOpenFileWithOplock and CreateFileW require wide
     *    character strings (wchar_t) to properly handle Unicode paths with international
     *    characters, spaces, and special symbols.
     */

    std::filesystem::path pathFs(wPath);
    if (!std::filesystem::exists(pathFs))
    {
        return {};
    }

    if (std::filesystem::is_directory(pathFs))
    {
        HANDLE handle = nullptr;
        const HRESULT openResult = CfOpenFileWithOplock(wPath.c_str(), CF_OPEN_FILE_FLAG_NONE, &handle);
        if (openResult == S_OK)
        {
            return {handle, [](HANDLE h)
                    { CfCloseHandle(h); }};
        }
        else
        {
            // Convert only for logging purposes
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::string path = converter.to_bytes(wPath);
            printf("Could not CfOpenFileWithOplock for path: %s with error: %ld\n", path.c_str(), openResult);
        }
    }
    else if (std::filesystem::is_regular_file(pathFs))
    {
        HANDLE handle = CreateFileW(
            wPath.c_str(), // Use wide string path directly
            FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (handle != INVALID_HANDLE_VALUE)
        {
            return {handle, [](HANDLE h)
                    { CloseHandle(h); }};
        }
        else
        {
            // Convert only for logging purposes
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::string path = converter.to_bytes(wPath);
            printf("Could not CreateFile for path: %s with error: %ld\n", path.c_str(), GetLastError());
        }
    }

    return {};
}