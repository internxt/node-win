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

    PlaceholderResult result = Placeholders::CreateOne(
        fileName.c_str(),
        fileIdentity.c_str(),
        fileSize,
        creationTime,
        lastWriteTime,
        lastAccessTime,
        destPath.c_str());

    // Create result object
    napi_value resultObj;
    napi_create_object(env, &resultObj);

    // Add success property
    napi_value successValue;
    napi_get_boolean(env, result.success, &successValue);
    napi_set_named_property(env, resultObj, "success", successValue);

    // Add errorMessage property if there is an error
    if (!result.success && !result.errorMessage.empty())
    {
        napi_value errorMessageValue;
        napi_create_string_utf16(env, reinterpret_cast<const char16_t*>(result.errorMessage.c_str()), result.errorMessage.length(), &errorMessageValue);
        napi_set_named_property(env, resultObj, "errorMessage", errorMessageValue);
    }

    return resultObj;
}