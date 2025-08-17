#include <Windows.h>
#include "Placeholders.h"

napi_value create_file_placeholder_impl(napi_env env, napi_callback_info args)
{
    size_t argc = 8;
    napi_value argv[8];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 8)
    {
        napi_throw_error(env, nullptr, "Insufficient arguments passed to create_file_placeholder_impl");
        return nullptr;
    }

    LPCWSTR fileName;
    size_t fileNameLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &fileNameLength);
    fileName = new WCHAR[fileNameLength + 1];
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(fileName)), fileNameLength + 1, nullptr);

    size_t fileIdentityLength;
    napi_get_value_string_utf16(env, argv[1], nullptr, 0, &fileIdentityLength);
    wchar_t *fileIdentity = new wchar_t[fileIdentityLength + 1];
    napi_get_value_string_utf16(env, argv[1], reinterpret_cast<char16_t *>(fileIdentity), fileIdentityLength + 1, nullptr);

    if (fileIdentityLength > CF_PLACEHOLDER_MAX_FILE_IDENTITY_LENGTH)
    {
        napi_throw_error(env, nullptr, "File identity is too long");
        return nullptr;
    }

    int64_t fileSize;
    napi_get_value_int64(env, argv[2], &fileSize);

    uint32_t fileAttributes;
    napi_get_value_uint32(env, argv[3], &fileAttributes);

    FILETIME creationTime, lastWriteTime, lastAccessTime;

    size_t creationTimeStringLength;
    napi_get_value_string_utf16(env, argv[4], nullptr, 0, &creationTimeStringLength);
    std::vector<wchar_t> creationTimeStringBuffer(creationTimeStringLength + 1);
    napi_get_value_string_utf16(env, argv[4], reinterpret_cast<char16_t *>(creationTimeStringBuffer.data()), creationTimeStringLength + 1, nullptr);

    __int64 windowsTimeValue;
    if (swscanf_s(creationTimeStringBuffer.data(), L"%lld", &windowsTimeValue) != 1)
    {
        napi_throw_error(env, nullptr, "No se pudo convertir el valor de Windows Time");
        return nullptr;
    }

    creationTime.dwLowDateTime = static_cast<DWORD>(windowsTimeValue & 0xFFFFFFFF);
    creationTime.dwHighDateTime = static_cast<DWORD>((windowsTimeValue >> 32) & 0xFFFFFFFF);

    size_t lastWriteTimeStringLength;
    napi_get_value_string_utf16(env, argv[5], nullptr, 0, &lastWriteTimeStringLength);
    std::vector<wchar_t> lastWriteTimeStringBuffer(lastWriteTimeStringLength + 1);
    napi_get_value_string_utf16(env, argv[5], reinterpret_cast<char16_t *>(lastWriteTimeStringBuffer.data()), lastWriteTimeStringLength + 1, nullptr);

    __int64 windowsTimeValue2;
    if (swscanf_s(lastWriteTimeStringBuffer.data(), L"%lld", &windowsTimeValue2) != 1)
    {
        napi_throw_error(env, nullptr, "No se pudo convertir el valor de Windows Time");
        return nullptr;
    }

    lastWriteTime.dwLowDateTime = static_cast<DWORD>(windowsTimeValue2 & 0xFFFFFFFF);
    lastWriteTime.dwHighDateTime = static_cast<DWORD>((windowsTimeValue2 >> 32) & 0xFFFFFFFF);

    lastAccessTime.dwLowDateTime = 34567890;
    lastAccessTime.dwHighDateTime = 78901234;

    LPCWSTR destPath;
    size_t destPathLength;
    napi_get_value_string_utf16(env, argv[7], nullptr, 0, &destPathLength);
    destPath = new WCHAR[destPathLength + 1];
    napi_get_value_string_utf16(env, argv[7], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(destPath)), destPathLength + 1, nullptr);

    PlaceholderResult result = Placeholders::CreateOne(
        fileName,
        fileIdentity,
        fileSize,
        fileIdentityLength,
        fileAttributes,
        creationTime,
        lastWriteTime,
        lastAccessTime,
        destPath);

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

    delete[] fileName;
    delete[] fileIdentity;
    delete[] destPath;

    return resultObj;
}