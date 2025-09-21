#include <Windows.h>
#include "Placeholders.h"

napi_value create_folder_placeholder_impl(napi_env env, napi_callback_info args)
{
    size_t argc = 9;
    napi_value argv[9];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 9)
    {
        napi_throw_error(env, nullptr, "Insufficient arguments passed to create_folder_placeholder_impl");
        return nullptr;
    }

    LPCWSTR itemName;
    size_t itemNameLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &itemNameLength);
    itemName = new WCHAR[itemNameLength + 1];
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(itemName)), itemNameLength + 1, nullptr);

    LPCWSTR itemIdentity;
    size_t itemIdentityLength;
    napi_get_value_string_utf16(env, argv[1], nullptr, 0, &itemIdentityLength);
    itemIdentity = new WCHAR[itemIdentityLength + 1];
    napi_get_value_string_utf16(env, argv[1], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(itemIdentity)), itemIdentityLength + 1, nullptr);

    bool isDirectory;
    napi_get_value_bool(env, argv[2], &isDirectory);

    uint32_t itemSize;
    napi_get_value_uint32(env, argv[3], &itemSize);

    DWORD itemIdentityLengthDword = static_cast<DWORD>(itemIdentityLength);

    uint32_t itemAttributes;
    napi_get_value_uint32(env, argv[4], &itemAttributes);

    FILETIME creationTime, lastWriteTime, lastAccessTime;

    size_t creationTimeStringLengthFolder;
    napi_get_value_string_utf16(env, argv[5], nullptr, 0, &creationTimeStringLengthFolder);
    std::vector<wchar_t> creationTimeStringBufferFolder(creationTimeStringLengthFolder + 1);
    napi_get_value_string_utf16(env, argv[5], reinterpret_cast<char16_t *>(creationTimeStringBufferFolder.data()), creationTimeStringLengthFolder + 1, nullptr);

    __int64 windowsTimeValue;
    if (swscanf_s(creationTimeStringBufferFolder.data(), L"%lld", &windowsTimeValue) != 1)
    {
        napi_throw_error(env, nullptr, "No se pudo convertir el valor de Windows Time");
        return nullptr;
    }

    creationTime.dwLowDateTime = static_cast<DWORD>(windowsTimeValue & 0xFFFFFFFF);
    creationTime.dwHighDateTime = static_cast<DWORD>((windowsTimeValue >> 32) & 0xFFFFFFFF);

    size_t lastWriteTimeStringLength;
    napi_get_value_string_utf16(env, argv[6], nullptr, 0, &lastWriteTimeStringLength);
    std::vector<wchar_t> lastWriteTimeStringBuffer(lastWriteTimeStringLength + 1);
    napi_get_value_string_utf16(env, argv[6], reinterpret_cast<char16_t *>(lastWriteTimeStringBuffer.data()), lastWriteTimeStringLength + 1, nullptr);

    __int64 windowsTimeValue2;
    if (swscanf_s(lastWriteTimeStringBuffer.data(), L"%lld", &windowsTimeValue2) != 1)
    {
        napi_throw_error(env, nullptr, "No se pudo convertir el valor de Windows Time");
        return nullptr;
    }

    lastWriteTime.dwLowDateTime = static_cast<DWORD>(windowsTimeValue2 & 0xFFFFFFFF);
    lastWriteTime.dwHighDateTime = static_cast<DWORD>((windowsTimeValue2 >> 32) & 0xFFFFFFFF);

    lastAccessTime.dwLowDateTime = 34567890;
    lastAccessTime.dwHighDateTime = 34567890;

    LPCWSTR destPath;
    size_t destPathLength;
    napi_get_value_string_utf16(env, argv[8], nullptr, 0, &destPathLength);
    destPath = new WCHAR[destPathLength + 1];
    napi_get_value_string_utf16(env, argv[8], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(destPath)), destPathLength + 1, nullptr);

    PlaceholderResult result = Placeholders::CreateEntry(
        itemName,
        itemIdentity,
        isDirectory,
        itemSize,
        itemIdentityLengthDword,
        itemAttributes,
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

    delete[] itemName;
    delete[] itemIdentity;
    delete[] destPath;

    return resultObj;
}
