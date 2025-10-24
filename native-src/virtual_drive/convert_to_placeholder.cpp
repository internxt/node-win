#include <filesystem>
#include <windows.h>
#include "stdafx.h"
#include "napi_extract_args.h"

void convert_to_placeholder(const std::wstring &path, const std::wstring &placeholderId)
{
    bool isDirectory = std::filesystem::is_directory(path);

    winrt::file_handle fileHandle{CreateFileW(
        path.c_str(),
        FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        isDirectory ? FILE_FLAG_BACKUP_SEMANTICS : 0,
        nullptr)};

    if (!fileHandle)
    {
        throw std::runtime_error("Failed to open item: " + std::to_string(GetLastError()));
    }

    CF_CONVERT_FLAGS convertFlags = CF_CONVERT_FLAG_MARK_IN_SYNC;
    USN convertUsn;
    OVERLAPPED overlapped = {};

    LPCVOID idStrLPCVOID = static_cast<LPCVOID>(placeholderId.c_str());
    DWORD idStrByteLength = static_cast<DWORD>(placeholderId.size() * sizeof(wchar_t));

    HRESULT hr = CfConvertToPlaceholder(fileHandle.get(), idStrLPCVOID, idStrByteLength, convertFlags, &convertUsn, &overlapped);

    // Only throw if it's not "already a placeholder" error
    if (hr != 0x8007017C)
    {
        winrt::check_hresult(hr);
    }
}

napi_value convert_to_placeholder_wrapper(napi_env env, napi_callback_info info)
{
    auto [path, placeholderId] = napi_extract_args<std::wstring, std::wstring>(env, info);

    convert_to_placeholder(path.c_str(), placeholderId.c_str());

    return nullptr;
}
