#include "PlaceHolderInfo.h"
#include <comdef.h>
#include <windows.h>
#include <filesystem>
#include <codecvt>
#include <locale>
#include "Logger.h"

PlaceHolderInfo::PlaceHolderInfo()
    : _data(nullptr, [](CF_PLACEHOLDER_BASIC_INFO *) {})
{
}

PlaceHolderInfo::PlaceHolderInfo(CF_PLACEHOLDER_BASIC_INFO *data, Deleter deleter)
    : _data(data, deleter)
{
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