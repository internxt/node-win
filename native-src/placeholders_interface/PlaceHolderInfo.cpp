#include "PlaceHolderInfo.h"
#include <comdef.h>
#include <windows.h>
#include <filesystem>
#include <codecvt>
#include <locale>

PinState cfPinStateToPinState(CF_PIN_STATE state)
{
    switch (state) {
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

std::string pinStateToString(PinState state) {
    switch (state) {
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
{}

PlaceHolderInfo::PlaceHolderInfo(CF_PLACEHOLDER_BASIC_INFO *data, Deleter deleter)
    : _data(data, deleter)
{}

std::optional<PinState> PlaceHolderInfo::pinState() const
{
    if (!_data) {
        return {};
    }

    return cfPinStateToPinState(_data->PinState);
}

FileHandle::FileHandle()
    : _data(nullptr, [](void *) {})
{
}

FileHandle::FileHandle(void *data, Deleter deleter)
    : _data(data, deleter)
{
}


FileHandle handleForPath(const std::wstring &wPath)
{
    if (wPath.empty()) {
        return {};
    }

    // Convertir std::wstring a std::string
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::string path = converter.to_bytes(wPath);

    printf("path IN HANDLERCREATOR: %s\n", path.c_str());
    LPCSTR pPath = path.c_str();

    std::filesystem::path pathFs(path);
    if (!std::filesystem::exists(pathFs)) {
        return {};
    }

    if (std::filesystem::is_directory(pathFs)) {
        HANDLE handle = nullptr;
        const HRESULT openResult = CfOpenFileWithOplock(wPath.c_str(), CF_OPEN_FILE_FLAG_NONE, &handle);
        if (openResult == S_OK) {
            return {handle, [](HANDLE h) { CfCloseHandle(h); }};
        } else {
            printf("Could not CfOpenFileWithOplock for path: %s with error: %ld\n", path.c_str(), openResult);
        }
    } else if (std::filesystem::is_regular_file(pathFs)) {
        HANDLE handle = CreateFile(
            pPath,
            FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (handle != INVALID_HANDLE_VALUE) {
            return {handle, [](HANDLE h) { CloseHandle(h); }};
        } else {
            printf("Could not CreateFile for path: %s with error: %ld\n", path.c_str(), GetLastError());
        }
    }

    return {};
}

// FileHandle handleForPath(const std::wstring &path)
// {
//     if (path.empty()) {
//         return {};
//     };

//     std::filesystem::path pathFs(path);
//     if (!std::filesystem::exists(pathFs)) {
//         return {};
//     };

//     LPCWSTR pathC = path.c_str();

//     if (std::filesystem::is_directory(pathFs)) {
//         HANDLE handle = nullptr;
//         const HRESULT openResult = CfOpenFileWithOplock(path.c_str(), CF_OPEN_FILE_FLAG_NONE, &handle);
//         if (openResult == S_OK) {
//             return {handle, [](HANDLE h) { CfCloseHandle(h); }};
//         } else {
//             printf("Could not CfOpenFileWithOplock for path: %ls with error: %d\n", path.c_str(), openResult);
//         }
//     } else if (std::filesystem::is_regular_file(pathFs)) {
//         HANDLE handle = CreateFile(pathC, 0, 0, nullptr,
//                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
//         if (handle != INVALID_HANDLE_VALUE) {
//             return {handle, [](HANDLE h) { CloseHandle(h); }};
//         } else {
//             printf("Could not CreateFile for path: %ls with error: %d\n", path.c_str(), GetLastError());
//         }
//     }

//     return {};
// }