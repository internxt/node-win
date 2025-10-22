#include <Windows.h>
#include "Placeholders.h"
#include "Utilities.h"
#include "napi_extract_args.h"

napi_value create_file_placeholder_impl(napi_env env, napi_callback_info info)
{
    auto [fileName, fileIdentity, fileSize, creationTimeMs, lastWriteTimeMs, lastAccessTimeMs, destPath] =
        napi_extract_args<std::wstring, std::wstring, int64_t, int64_t, int64_t, int64_t, std::wstring>(env, info);

    LARGE_INTEGER creationTime = Utilities::JsTimestampToLargeInteger(creationTimeMs);
    LARGE_INTEGER lastWriteTime = Utilities::JsTimestampToLargeInteger(lastWriteTimeMs);
    LARGE_INTEGER lastAccessTime = Utilities::JsTimestampToLargeInteger(lastAccessTimeMs);

    Placeholders::CreateOne(
        fileName.c_str(),
        fileIdentity.c_str(),
        fileSize,
        creationTime,
        lastWriteTime,
        lastAccessTime,
        destPath.c_str());

    return nullptr;
}