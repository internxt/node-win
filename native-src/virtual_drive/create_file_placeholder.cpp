#include <filesystem>
#include <windows.h>
#include "Placeholders.h"
#include "Utilities.h"
#include "napi_extract_args.h"

napi_value create_file_placeholder_impl(napi_env env, napi_callback_info info)
{
    auto [name, placeholderId, fileSize, creationTimeMs, lastWriteTimeMs, lastAccessTimeMs, parentPath] =
        napi_extract_args<std::wstring, std::wstring, int64_t, int64_t, int64_t, int64_t, std::wstring>(env, info);

    LARGE_INTEGER creationTime = Utilities::JsTimestampToLargeInteger(creationTimeMs);
    LARGE_INTEGER lastWriteTime = Utilities::JsTimestampToLargeInteger(lastWriteTimeMs);
    LARGE_INTEGER lastAccessTime = Utilities::JsTimestampToLargeInteger(lastAccessTimeMs);

    std::wstring path = parentPath + L'\\' + name;
    
    if (std::filesystem::exists(path)) {
        Placeholders::ConvertToPlaceholder(path, placeholderId);
        Placeholders::MaintainIdentity(path, placeholderId.c_str(), false);
        return nullptr;
    }

    CF_PLACEHOLDER_CREATE_INFO cloudEntry = {};
    cloudEntry.FileIdentity = placeholderId.c_str();
    cloudEntry.FileIdentityLength = static_cast<DWORD>((placeholderId.size() + 1) * sizeof(WCHAR));
    cloudEntry.RelativeFileName = name.c_str();
    cloudEntry.Flags = CF_PLACEHOLDER_CREATE_FLAG_MARK_IN_SYNC;
    cloudEntry.FsMetadata.FileSize.QuadPart = fileSize;
    cloudEntry.FsMetadata.BasicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;
    cloudEntry.FsMetadata.BasicInfo.CreationTime = creationTime;
    cloudEntry.FsMetadata.BasicInfo.LastWriteTime = lastWriteTime;
    cloudEntry.FsMetadata.BasicInfo.LastAccessTime = lastAccessTime;
    cloudEntry.FsMetadata.BasicInfo.ChangeTime = lastWriteTime;

    winrt::check_hresult(CfCreatePlaceholders(parentPath.c_str(), &cloudEntry, 1, CF_CREATE_FLAG_NONE, NULL));

    return nullptr;
}