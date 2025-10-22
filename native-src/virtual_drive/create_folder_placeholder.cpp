#include <Windows.h>
#include "Placeholders.h"
#include "Utilities.h"
#include "napi_extract_args.h"

napi_value create_folder_placeholder_impl(napi_env env, napi_callback_info info)
{
    auto [folderName, placeholderId, creationTimeMs, lastWriteTimeMs, lastAccessTimeMs, parentPath] =
        napi_extract_args<std::wstring, std::wstring, int64_t, int64_t, int64_t, std::wstring>(env, info);

    LARGE_INTEGER creationTime = Utilities::JsTimestampToLargeInteger(creationTimeMs);
    LARGE_INTEGER lastWriteTime = Utilities::JsTimestampToLargeInteger(lastWriteTimeMs);
    LARGE_INTEGER lastAccessTime = Utilities::JsTimestampToLargeInteger(lastAccessTimeMs);

    Placeholders::CreateEntry(
        folderName,
        placeholderId,
        creationTime,
        lastWriteTime,
        lastAccessTime,
        parentPath);

    return nullptr;
}
