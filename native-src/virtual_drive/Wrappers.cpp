#include "stdafx.h"
#include <sstream>
#include "Placeholders.h"
#include "SyncRoot.h"
#include "SyncRootWatcher.h"
#include "Callbacks.h"
#include "LoggerPath.h"
#include "DownloadMutexManager.h"
#include "DirectoryWatcher.h"
#include <Logger.h>

napi_value CreatePlaceholderFile(napi_env env, napi_callback_info args)
{
    size_t argc = 8;
    napi_value argv[8];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 8)
    {
        napi_throw_error(env, nullptr, "Insufficient arguments passed to CreatePlaceholderFile");
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

    Placeholders::CreateOne(
        fileName,
        fileIdentity,
        fileSize,
        fileIdentityLength,
        fileAttributes,
        creationTime,
        lastWriteTime,
        lastAccessTime,
        destPath);

    delete[] fileName;
    delete[] fileIdentity;
    delete[] destPath;

    napi_value result;
    napi_get_boolean(env, true, &result);
    return result;
}

napi_value UnregisterSyncRootWrapper(napi_env env, napi_callback_info args)
{
    size_t argc = 1;
    napi_value argv[1];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, nullptr, "The sync root path is required for UnregisterSyncRoot");
        return nullptr;
    }

    LPCWSTR syncRootPath;
    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);
    syncRootPath = new WCHAR[pathLength + 1];
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(syncRootPath)), pathLength + 1, nullptr);

    HRESULT result = SyncRoot::UnregisterSyncRoot();

    delete[] syncRootPath;

    napi_value napiResult;
    napi_create_int32(env, static_cast<int32_t>(result), &napiResult);
    return napiResult;
}

napi_value RegisterSyncRootWrapper(napi_env env, napi_callback_info args)
{
    size_t argc = 5;
    napi_value argv[5];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 5)
    {
        napi_throw_error(env, nullptr, "5 arguments are required for RegisterSyncRoot");
        return nullptr;
    }

    LPCWSTR syncRootPath;
    size_t syncRootPathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &syncRootPathLength);
    syncRootPath = new WCHAR[syncRootPathLength + 1];
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(syncRootPath)), syncRootPathLength + 1, nullptr);

    LPCWSTR providerName;
    size_t providerNameLength;
    napi_get_value_string_utf16(env, argv[1], nullptr, 0, &providerNameLength);
    providerName = new WCHAR[providerNameLength + 1];
    napi_get_value_string_utf16(env, argv[1], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(providerName)), providerNameLength + 1, nullptr);

    LPCWSTR providerVersion;
    size_t providerVersionLength;
    napi_get_value_string_utf16(env, argv[2], nullptr, 0, &providerVersionLength);
    providerVersion = new WCHAR[providerVersionLength + 1];
    napi_get_value_string_utf16(env, argv[2], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(providerVersion)), providerVersionLength + 1, nullptr);

    GUID providerId;
    LPCWSTR providerIdStr;
    size_t providerIdStrLength;
    napi_get_value_string_utf16(env, argv[3], nullptr, 0, &providerIdStrLength);
    providerIdStr = new WCHAR[providerIdStrLength + 1];
    napi_get_value_string_utf16(env, argv[3], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(providerIdStr)), providerIdStrLength + 1, nullptr);

    if (FAILED(CLSIDFromString(providerIdStr, &providerId)))
    {
        napi_throw_error(env, nullptr, "Invalid GUID format");
        delete[] syncRootPath;
        delete[] providerName;
        delete[] providerVersion;
        delete[] providerIdStr;
        return nullptr;
    }

    LPCWSTR logoPath;
    size_t logoPathLength;
    napi_get_value_string_utf16(env, argv[4], nullptr, 0, &logoPathLength);
    logoPath = new WCHAR[logoPathLength + 1];
    napi_get_value_string_utf16(env, argv[4], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(logoPath)), logoPathLength + 1, nullptr);

    HRESULT result = SyncRoot::RegisterSyncRoot(syncRootPath, providerName, providerVersion, providerId, logoPath);

    delete[] providerIdStr;

    napi_value napiResult;
    napi_create_int32(env, static_cast<int32_t>(result), &napiResult);
    return napiResult;
}

napi_value ConnectSyncRootWrapper(napi_env env, napi_callback_info args)
{
    try
    {

        size_t argc = 2;
        napi_value argv[2];

        napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

        if (argc < 2)
        {
            napi_throw_error(env, nullptr, "Se requieren más argumentos para ConnectSyncRoot");
            return nullptr;
        }

        LPCWSTR syncRootPath;
        size_t pathLength;
        napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);
        syncRootPath = new WCHAR[pathLength + 1];
        napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(syncRootPath)), pathLength + 1, nullptr);

        // CALLBACKS
        InputSyncCallbacks callbacks = {};

        napi_value notifyDeleteCompletionCallback;
        napi_value notifyRenameCallback;
        napi_value fetchDataCallback;
        napi_value cancelFetchDataCallback;

        if (napi_get_named_property(env, argv[1], "notifyDeleteCallback", &notifyDeleteCompletionCallback) == napi_ok)
        {
            napi_create_reference(env, notifyDeleteCompletionCallback, 1, &callbacks.notify_delete_callback_ref);
        }

        napi_valuetype valuetype;
        napi_status type_status = napi_typeof(env, notifyDeleteCompletionCallback, &valuetype);
        if (type_status != napi_ok || valuetype != napi_function)
        {
            napi_throw_error(env, nullptr, "notifyDeleteCallback should be a function.");
            return nullptr;
        }

        if (napi_get_named_property(env, argv[1], "notifyRenameCallback", &notifyRenameCallback) == napi_ok)
        {
            napi_create_reference(env, notifyRenameCallback, 1, &callbacks.notify_rename_callback_ref);
        }

        napi_valuetype valuetype_rename;
        napi_status type_status_rename = napi_typeof(env, notifyRenameCallback, &valuetype_rename);
        if (type_status_rename != napi_ok || valuetype_rename != napi_function)
        {
            napi_throw_error(env, nullptr, "notifyRenameCallback should be a function.");
            return nullptr;
        }

        if (napi_get_named_property(env, argv[1], "fetchDataCallback", &fetchDataCallback) == napi_ok)
        {
            napi_create_reference(env, fetchDataCallback, 1, &callbacks.fetch_data_callback_ref);
        }

        napi_valuetype valuetype_fetch_data;
        napi_status type_status_fetch_data = napi_typeof(env, fetchDataCallback, &valuetype_fetch_data);
        if (type_status_fetch_data != napi_ok || valuetype_fetch_data != napi_function)
        {
            napi_throw_error(env, nullptr, "fetchDataCallback should be a function.");
            return nullptr;
        }

        if (napi_get_named_property(env, argv[1], "cancelFetchDataCallback", &cancelFetchDataCallback) == napi_ok)
        {
            napi_create_reference(env, cancelFetchDataCallback, 1, &callbacks.cancel_fetch_data_callback_ref);
        }

        napi_valuetype valuetype_cancel_fetch_data;
        napi_status type_status_cancel_fetch_data = napi_typeof(env, cancelFetchDataCallback, &valuetype_cancel_fetch_data);
        if (type_status_cancel_fetch_data != napi_ok || valuetype_cancel_fetch_data != napi_function)
        {
            napi_throw_error(env, nullptr, "cancelFetchDataCallback should be a function.");
            return nullptr;
        }

        CF_CONNECTION_KEY connectionKey;
        HRESULT hr = SyncRoot::ConnectSyncRoot(syncRootPath, callbacks, env, &connectionKey);

        delete[] syncRootPath;

        if (FAILED(hr))
        {
            napi_throw_error(env, nullptr, "ConnectSyncRoot failed");
            return nullptr;
        }

        napi_value resultObj, hrValue, connectionKeyValue;

        napi_create_object(env, &resultObj);

        napi_create_int32(env, static_cast<int32_t>(hr), &hrValue);
        napi_set_named_property(env, resultObj, "hr", hrValue);

        std::wstringstream ss;
        ss << connectionKey.Internal;

        std::wstring connectionKeyString = ss.str();
        napi_create_string_utf16(env, reinterpret_cast<const char16_t *>(connectionKeyString.c_str()), connectionKeyString.length(), &connectionKeyValue);

        napi_set_named_property(env, resultObj, "connectionKey", connectionKeyValue);

        return resultObj;
    }
    catch (...)
    {
        napi_throw_error(env, nullptr, "An unknown error occurred in ConnectSyncRootWrapper");
        return nullptr;
    }
}

napi_value WatchAndWaitWrapper(napi_env env, napi_callback_info args)
{
    size_t argc = 2;
    napi_value argv[2];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 2)
    {
        napi_throw_error(env, nullptr, "Se requieren más argumentos para WatchAndWait");
        return nullptr;
    }

    InputCallbacks input = {};

    // [CALLBACKS] file_added
    napi_value notifyFileAddedCallback;

    if (napi_get_named_property(env, argv[1], "notifyFileAddedCallback", &notifyFileAddedCallback) == napi_ok)
    {
        napi_create_reference(env, notifyFileAddedCallback, 1, &input.notify_file_added_callback_ref);
    }

    napi_valuetype valuetype;
    napi_status type_status = napi_typeof(env, notifyFileAddedCallback, &valuetype);
    if (type_status != napi_ok || valuetype != napi_function)
    {
        napi_throw_error(env, nullptr, "notifyFileAddedCallback should be a function.");
        return nullptr;
    }

    napi_value resource_name_value;

    std::string resource_name = "notify_file_added_callback";
    std::u16string converted_resource_name = std::u16string(resource_name.begin(), resource_name.end());

    napi_create_string_utf16(env, converted_resource_name.c_str(), NAPI_AUTO_LENGTH, &resource_name_value);

    napi_value notify_file_added_callback_value;
    napi_status status_ref = napi_get_reference_value(env, input.notify_file_added_callback_ref, &notify_file_added_callback_value);

    napi_threadsafe_function notify_file_added_threadsafe_callback;

    napi_status status_threadsafe = napi_create_threadsafe_function(
        env,
        notify_file_added_callback_value,
        NULL,
        resource_name_value,
        0,
        1,
        NULL,
        NULL,
        NULL,
        notify_file_added_call,
        &notify_file_added_threadsafe_callback);

    // [CALLBACKS] notify_message
    napi_value notifyMessageCallback;

    if (napi_get_named_property(env, argv[1], "notifyMessageCallback", &notifyMessageCallback) == napi_ok)
    {
        napi_create_reference(env, notifyMessageCallback, 1, &input.notify_message_callback_ref);
    }

    napi_valuetype valuetype_message;
    napi_status type_status_message = napi_typeof(env, notifyMessageCallback, &valuetype_message);
    if (type_status_message != napi_ok || valuetype_message != napi_function)
    {
        napi_throw_error(env, nullptr, "notifyMessageCallback should be a function.");
        return nullptr;
    }

    napi_value resource_name_value_message;

    std::string resource_name_message = "notify_message_callback";
    std::u16string converted_resource_name_message = std::u16string(resource_name_message.begin(), resource_name_message.end());

    napi_create_string_utf16(env, converted_resource_name_message.c_str(), NAPI_AUTO_LENGTH, &resource_name_value_message);

    napi_value notify_message_callback_value;
    napi_status status_ref_message = napi_get_reference_value(env, input.notify_message_callback_ref, &notify_message_callback_value);

    napi_threadsafe_function notify_message_threadsafe_callback;

    napi_status status_threadsafe_message = napi_create_threadsafe_function(
        env,
        notify_message_callback_value,
        NULL,
        resource_name_value_message,
        0,
        1,
        NULL,
        NULL,
        NULL,
        notify_message_call,
        &notify_message_threadsafe_callback);

    InputSyncCallbacksThreadsafe inputThreadsafe = {};
    inputThreadsafe.notify_file_added_threadsafe_callback = notify_file_added_threadsafe_callback;
    inputThreadsafe.notify_message_threadsafe_callback = notify_message_threadsafe_callback;

    LPCWSTR syncRootPath;
    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);
    syncRootPath = new WCHAR[pathLength + 1];
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(syncRootPath)), pathLength + 1, nullptr);

    SyncRootWatcher watcher;
    watcher.WatchAndWait(syncRootPath, env, inputThreadsafe);

    delete[] syncRootPath;

    return nullptr;
}

napi_value CreateEntryWrapper(napi_env env, napi_callback_info args)
{
    size_t argc = 9;
    napi_value argv[9];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 9)
    {
        napi_throw_error(env, nullptr, "Insufficient arguments passed to CreateEntryWrapper");
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

    Placeholders::CreateEntry(
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

    delete[] itemName;
    delete[] itemIdentity;
    delete[] destPath;

    napi_value result;
    napi_get_boolean(env, true, &result);
    return result;
}

// disconection wrapper
napi_value DisconnectSyncRootWrapper(napi_env env, napi_callback_info args)
{
    size_t argc = 1;
    napi_value argv[1];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, nullptr, "The sync root path is required for DisconnectSyncRoot");
        return nullptr;
    }

    LPCWSTR syncRootPath;
    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);
    syncRootPath = new WCHAR[pathLength + 1];
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(syncRootPath)), pathLength + 1, nullptr);

    HRESULT result = SyncRoot::DisconnectSyncRoot();
    // wprintf(L"DisconnectSyncRootWrapper: %08x\n", static_cast<HRESULT>(result));
    delete[] syncRootPath;

    napi_value napiResult;
    napi_create_int32(env, static_cast<int32_t>(result), &napiResult);
    return napiResult;
}

napi_value GetFileIdentityWrapper(napi_env env, napi_callback_info args)
{
    printf("GetFileIdentityWrapper\n");
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, nullptr, "The path is required for GetFileIdentity");
        return nullptr;
    }

    LPCWSTR fullPath;
    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);
    fullPath = new WCHAR[pathLength + 1];
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(fullPath)), pathLength + 1, nullptr);

    std::string fileIdentity = Placeholders::GetFileIdentity(fullPath);
    printf("fileIdentity got\n");
    fileIdentity.erase(std::remove(fileIdentity.begin(), fileIdentity.end(), '\0'), fileIdentity.end());
    fileIdentity.erase(std::remove(fileIdentity.begin(), fileIdentity.end(), ' '), fileIdentity.end());

    napi_value jsFileIdentity;
    napi_create_string_utf8(env, fileIdentity.c_str(), fileIdentity.length(), &jsFileIdentity);

    delete[] fullPath;
    return jsFileIdentity;
}

napi_value DeleteFileSyncRootWrapper(napi_env env, napi_callback_info args)
{
    printf("DeleteFileSyncRootWrapper\n");
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, nullptr, "The path is required for DeleteFileSyncRoot");
        return nullptr;
    }

    LPCWSTR fullPath;
    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);
    fullPath = new WCHAR[pathLength + 1];
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(fullPath)), pathLength + 1, nullptr);

    SyncRoot::DeleteFileSyncRoot(fullPath);
    printf("fileIdentity got\n");

    delete[] fullPath;
    return nullptr;
}

napi_value addLoggerPathWrapper(napi_env env, napi_callback_info args)
{
    size_t argc = 1;
    napi_value argv[1];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);
    if (argc < 1)
    {
        napi_throw_error(env, nullptr, "The path is required for addLoggerPath");
        return nullptr;
    }

    // Obtener la longitud de la cadena UTF-16.
    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);

    // Crear un buffer para la cadena UTF-16.
    std::unique_ptr<wchar_t[]> widePath(new wchar_t[pathLength + 1]);

    // Obtener la cadena UTF-16.
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t *>(widePath.get()), pathLength + 1, nullptr);

    // Obtener la longitud necesaria para la cadena UTF-8.
    int utf8Length = WideCharToMultiByte(CP_UTF8, 0, widePath.get(), -1, nullptr, 0, nullptr, nullptr);

    // Crear un buffer para la cadena UTF-8.
    std::unique_ptr<char[]> utf8Path(new char[utf8Length]);

    // Realizar la conversión de UTF-16 a UTF-8.
    WideCharToMultiByte(CP_UTF8, 0, widePath.get(), -1, utf8Path.get(), utf8Length, nullptr, nullptr);

    // Inicializar el logger con la ruta UTF-8.
    LoggerPath::set(std::string(utf8Path.get()));

    // Devolver un valor booleano verdadero.
    napi_value result;
    napi_get_boolean(env, true, &result);
    return result;
}

napi_value UpdateSyncStatusWrapper(napi_env env, napi_callback_info args)
{
    size_t argc = 3;
    napi_value argv[3];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);
    if (argc < 3)
    {
        napi_throw_error(env, nullptr, "Three arguments are required for UpdateSyncStatus");
        return nullptr;
    }

    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);

    std::unique_ptr<wchar_t[]> widePath(new wchar_t[pathLength + 1]);
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t *>(widePath.get()), pathLength + 1, nullptr);

    bool inputSyncState;
    napi_get_value_bool(env, argv[1], &inputSyncState);

    bool isDirectory;
    napi_get_value_bool(env, argv[2], &isDirectory);

    Placeholders::UpdateSyncStatus(widePath.get(), inputSyncState, isDirectory);

    napi_value result;
    napi_get_boolean(env, true, &result);
    return result;
}

napi_value GetPlaceholderStateWrapper(napi_env env, napi_callback_info args)
{
    size_t argc = 1;
    napi_value argv[1];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);
    if (argc < 1)
    {
        napi_throw_error(env, nullptr, "The path is required for GetPlaceholderState");
        return nullptr;
    }

    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);

    std::unique_ptr<wchar_t[]> widePath(new wchar_t[pathLength + 1]);

    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t *>(widePath.get()), pathLength + 1, nullptr);

    // DWORD state = Placeholders::GetPlaceholderState(widePath.get());
    FileState state = DirectoryWatcher::getPlaceholderInfo(widePath.get());

    napi_value result;
    napi_create_object(env, &result);

    napi_value jsPinState;
    napi_create_int32(env, static_cast<int32_t>(state.pinstate), &jsPinState);
    napi_set_named_property(env, result, "pinState", jsPinState);

    napi_value jsSyncState;
    napi_create_int32(env, static_cast<int32_t>(state.syncstate), &jsSyncState);
    napi_set_named_property(env, result, "syncState", jsSyncState);

    return result;
}

napi_value GetPlaceholderWithStatePendingWrapper(napi_env env, napi_callback_info args)
{
    size_t argc = 1;
    napi_value argv[1];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);
    if (argc < 1)
    {
        napi_throw_error(env, nullptr, "The path is required for GetPlaceholderWithStatePending");
        return nullptr;
    }

    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);

    std::unique_ptr<char16_t[]> widePath(new char16_t[pathLength + 1]);

    napi_get_value_string_utf16(env, argv[0], widePath.get(), pathLength + 1, nullptr);

    std::vector<std::wstring> state = Placeholders::GetPlaceholderWithStatePending(reinterpret_cast<wchar_t *>(widePath.get()));

    napi_value result;
    napi_create_array_with_length(env, state.size(), &result);
    for (size_t i = 0; i < state.size(); ++i)
    {
        napi_value jsString;
        napi_create_string_utf16(env, reinterpret_cast<const char16_t *>(state[i].c_str()), state[i].length(), &jsString);
        napi_set_element(env, result, i, jsString);
    }

    return result;
}

napi_value ConvertToPlaceholderWrapper(napi_env env, napi_callback_info args)
{
    size_t argc = 2;
    napi_value argv[2];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 2)
    {
        napi_throw_error(env, nullptr, "Both full path and placeholder ID are required for ConvertToPlaceholder");
        return nullptr;
    }

    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);

    std::unique_ptr<char16_t[]> widePath(new char16_t[pathLength + 1]);

    napi_get_value_string_utf16(env, argv[0], widePath.get(), pathLength + 1, nullptr);

    size_t placeholderIdLength;
    napi_get_value_string_utf16(env, argv[1], nullptr, 0, &placeholderIdLength);

    std::unique_ptr<char16_t[]> widePlaceholderId(new char16_t[placeholderIdLength + 1]);

    napi_get_value_string_utf16(env, argv[1], widePlaceholderId.get(), placeholderIdLength + 1, nullptr);

    bool success = Placeholders::ConvertToPlaceholder(
        reinterpret_cast<wchar_t *>(widePath.get()),
        reinterpret_cast<wchar_t *>(widePlaceholderId.get()));

    napi_value result;
    napi_get_boolean(env, success, &result);

    return result;
}

napi_value UpdateFileIdentityWrapper(napi_env env, napi_callback_info args)
{

    size_t argc = 3;
    napi_value argv[3];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 3)
    {
        napi_throw_error(env, nullptr, "Both full path and placeholder ID are required for UpdateFileIdentityWrapper");
        return nullptr;
    }

    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);

    std::unique_ptr<char16_t[]> widePath(new char16_t[pathLength + 1]);

    napi_get_value_string_utf16(env, argv[0], widePath.get(), pathLength + 1, nullptr);

    size_t placeholderIdLength;
    napi_get_value_string_utf16(env, argv[1], nullptr, 0, &placeholderIdLength);

    std::unique_ptr<char16_t[]> widePlaceholderId(new char16_t[placeholderIdLength + 1]);

    napi_get_value_string_utf16(env, argv[1], widePlaceholderId.get(), placeholderIdLength + 1, nullptr);

    bool isDirectory;
    napi_get_value_bool(env, argv[2], &isDirectory);

    Placeholders::UpdateFileIdentity(
        reinterpret_cast<wchar_t *>(widePath.get()),
        reinterpret_cast<wchar_t *>(widePlaceholderId.get()),
        isDirectory);

    napi_value result;
    return result;
}

napi_value CloseMutexWrapper(napi_env env, napi_callback_info args)
{

    DownloadMutexManager &mutexManager = DownloadMutexManager::getInstance();
    mutexManager.setReady(true);

    napi_value result;
    napi_get_boolean(env, true, &result);

    return result;
}

napi_value HydrateFileWrapper(napi_env env, napi_callback_info args) {
    size_t argc = 1;
    napi_value argv[1];
    napi_value thisArg;
    napi_get_cb_info(env, args, &argc, argv, &thisArg, nullptr);

    if (argc < 1) {
        napi_throw_type_error(env, nullptr, "The file path is required for HydrateFile");
        return nullptr;
    }

    // Obtener el argumento de JavaScript y convertirlo a una cadena de C++
    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);
    std::wstring fullPath(pathLength, L'\0');
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t*>(&fullPath[0]), pathLength + 1, nullptr);

    // Crear una promesa
    napi_deferred deferred;
    napi_value promise;
    napi_create_promise(env, &deferred, &promise);

    // Lanzar la operación asíncrona en un hilo separado
    std::thread([deferred, fullPath, env]() {
        try {
            SyncRoot::HydrateFile(fullPath.c_str());
             Logger::getInstance().log("finish... " + Logger::fromWStringToString(fullPath.c_str()), LogLevel::INFO);

            napi_value result;
            napi_get_undefined(env, &result);
            napi_resolve_deferred(env, deferred, result);
        } catch (const std::exception& e) {
            napi_value error;
            napi_create_string_utf8(env, e.what(), NAPI_AUTO_LENGTH, &error);
            napi_reject_deferred(env, deferred, error);
        } catch (...) {
            napi_value error;
            napi_create_string_utf8(env, "Unknown error", NAPI_AUTO_LENGTH, &error);
            napi_reject_deferred(env, deferred, error);
        }
    }).detach();

    return promise;
}

// Wrapper for DehydrateFile
napi_value DehydrateFileWrapper(napi_env env, napi_callback_info args)
{
    size_t argc = 1;
    napi_value argv[1];
    napi_value thisArg;
    napi_get_cb_info(env, args, &argc, argv, &thisArg, nullptr);

    if (argc < 1)
    {
        napi_throw_type_error(env, nullptr, "The file path is required for DehydrateFile");
        return nullptr;
    }

    // Obtener el argumento de JavaScript y convertirlo a una cadena de C++
    LPCWSTR fullPath;
    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);
    fullPath = new WCHAR[pathLength + 1];
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(fullPath)), pathLength + 1, nullptr);

    // Llamar a la función DehydrateFile
    SyncRoot::DehydrateFile(fullPath);

    napi_value result;
    napi_get_boolean(env, true, &result);

    return result;
}