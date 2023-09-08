#include "stdafx.h"
#include <sstream>
#include "Placeholders.h"
#include "SyncRoot.h"
#include "SyncRootWatcher.h"
#include "Callbacks.h"

// void notify_file_added_call(napi_env env, napi_value js_callback, void* context, void* data) {
//     wprintf(L"notify_file_added_call\n");

//     const char* receivedData = static_cast<const char*>(data);

//     napi_value undefined;
//     napi_get_undefined(env, &undefined);
//     napi_value result;
//     napi_call_function(env, undefined, js_callback, 0, nullptr, &result);

//     delete receivedData;
// }

void notify_file_added_call(napi_env env, napi_value js_callback, void* context, void* data) {
    // Convertir el argumento data a std::wstring
    std::wstring* receivedData = static_cast<std::wstring*>(data);

    // Crear un objeto napi_value para la cadena
    napi_value js_string;
    napi_create_string_utf16(env, reinterpret_cast<const char16_t*>(receivedData->c_str()), receivedData->size(), &js_string);

    // Pasar el objeto js_string como argumento al llamar al callback
    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_value result;
    napi_call_function(env, undefined, js_callback, 1, &js_string, &result);

    // Liberar la memoria asignada para receivedData
    delete receivedData;
}


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
    wchar_t* fileIdentity = new wchar_t[fileIdentityLength + 1];
    napi_get_value_string_utf16(env, argv[1], reinterpret_cast<char16_t *>(fileIdentity), fileIdentityLength + 1, nullptr);
    
    if (fileIdentityLength > CF_PLACEHOLDER_MAX_FILE_IDENTITY_LENGTH) {
        napi_throw_error(env, nullptr, "File identity is too long");
        return nullptr;
    }

    uint32_t fileSize;
    napi_get_value_uint32(env, argv[2], &fileSize);

    uint32_t fileAttributes;
    napi_get_value_uint32(env, argv[3], &fileAttributes);

    FILETIME creationTime, lastWriteTime, lastAccessTime;

    creationTime.dwLowDateTime = 12345678;
    creationTime.dwHighDateTime = 87654321;

    lastWriteTime.dwLowDateTime = 98765432;
    lastWriteTime.dwHighDateTime = 23456789;

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

napi_value UnregisterSyncRootWrapper(napi_env env, napi_callback_info args) {
    size_t argc = 1;
    napi_value argv[1];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, nullptr, "The sync root path is required for UnregisterSyncRoot");
        return nullptr;
    }

    LPCWSTR syncRootPath;
    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);
    syncRootPath = new WCHAR[pathLength + 1];
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t*>(const_cast<wchar_t*>(syncRootPath)), pathLength + 1, nullptr);

    HRESULT result = SyncRoot::UnregisterSyncRoot();

    delete[] syncRootPath;

    napi_value napiResult;
    napi_create_int32(env, static_cast<int32_t>(result), &napiResult);
    return napiResult;
}

napi_value RegisterSyncRootWrapper(napi_env env, napi_callback_info args) {
    size_t argc = 4;
    napi_value argv[4];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 4) {
        napi_throw_error(env, nullptr, "4 arguments are required for RegisterSyncRoot");
        return nullptr;
    }

    LPCWSTR syncRootPath;
    size_t syncRootPathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &syncRootPathLength);
    syncRootPath = new WCHAR[syncRootPathLength + 1];
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t*>(const_cast<wchar_t*>(syncRootPath)), syncRootPathLength + 1, nullptr);

    LPCWSTR providerName;
    size_t providerNameLength;
    napi_get_value_string_utf16(env, argv[1], nullptr, 0, &providerNameLength);
    providerName = new WCHAR[providerNameLength + 1];
    napi_get_value_string_utf16(env, argv[1], reinterpret_cast<char16_t*>(const_cast<wchar_t*>(providerName)), providerNameLength + 1, nullptr);

    LPCWSTR providerVersion;
    size_t providerVersionLength;
    napi_get_value_string_utf16(env, argv[2], nullptr, 0, &providerVersionLength);
    providerVersion = new WCHAR[providerVersionLength + 1];
    napi_get_value_string_utf16(env, argv[2], reinterpret_cast<char16_t*>(const_cast<wchar_t*>(providerVersion)), providerVersionLength + 1, nullptr);

    GUID providerId;
    LPCWSTR providerIdStr;
    size_t providerIdStrLength;
    napi_get_value_string_utf16(env, argv[3], nullptr, 0, &providerIdStrLength);
    providerIdStr = new WCHAR[providerIdStrLength + 1];
    napi_get_value_string_utf16(env, argv[3], reinterpret_cast<char16_t*>(const_cast<wchar_t*>(providerIdStr)), providerIdStrLength + 1, nullptr);

    if (FAILED(CLSIDFromString(providerIdStr, &providerId))) {
        napi_throw_error(env, nullptr, "Invalid GUID format");
        delete[] syncRootPath;
        delete[] providerName;
        delete[] providerVersion;
        delete[] providerIdStr;
        return nullptr;
    }    

    HRESULT result = SyncRoot::RegisterSyncRoot(syncRootPath, providerName, providerVersion, providerId);


    delete[] providerIdStr;

    napi_value napiResult;
    napi_create_int32(env, static_cast<int32_t>(result), &napiResult);
    return napiResult;
}

napi_value ConnectSyncRootWrapper(napi_env env, napi_callback_info args) {
    try {

    size_t argc = 2;
    napi_value argv[2];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 2) {
        napi_throw_error(env, nullptr, "Se requieren más argumentos para ConnectSyncRoot");
        return nullptr;
    }

    LPCWSTR syncRootPath;
    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);
    syncRootPath = new WCHAR[pathLength + 1];
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t*>(const_cast<wchar_t*>(syncRootPath)), pathLength + 1, nullptr);

    // CALLBACKS
    InputSyncCallbacks callbacks = {};

    napi_value notifyDeleteCompletionCallback;
    napi_value notifyRenameCallback;

    if (napi_get_named_property(env, argv[1], "notifyDeleteCallback", &notifyDeleteCompletionCallback) == napi_ok) {
        napi_create_reference(env, notifyDeleteCompletionCallback, 1, &callbacks.notify_delete_callback_ref);
    }

    napi_valuetype valuetype;
    napi_status type_status = napi_typeof(env, notifyDeleteCompletionCallback, &valuetype);
    if (type_status != napi_ok || valuetype != napi_function) {
        napi_throw_error(env, nullptr, "notifyDeleteCallback should be a function.");
        return nullptr;
    }

    if (napi_get_named_property(env, argv[1], "notifyRenameCallback", &notifyRenameCallback) == napi_ok) {
        napi_create_reference(env, notifyRenameCallback, 1, &callbacks.notify_rename_callback_ref);
    }

    napi_valuetype valuetype_rename;
    napi_status type_status_rename = napi_typeof(env, notifyRenameCallback, &valuetype_rename);
    if (type_status_rename != napi_ok || valuetype_rename != napi_function) {
        napi_throw_error(env, nullptr, "notifyRenameCallback should be a function.");
        return nullptr;
    }

    CF_CONNECTION_KEY connectionKey;
    HRESULT hr = SyncRoot::ConnectSyncRoot(syncRootPath, callbacks, env, &connectionKey);

    delete[] syncRootPath;



    if (FAILED(hr)) {
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
    napi_create_string_utf16(env, reinterpret_cast<const char16_t*>(connectionKeyString.c_str()), connectionKeyString.length(), &connectionKeyValue);
        
    napi_set_named_property(env, resultObj, "connectionKey", connectionKeyValue);

    return resultObj;
    }
    catch (...) {
        napi_throw_error(env, nullptr, "An unknown error occurred in ConnectSyncRootWrapper");
        return nullptr;
    }
}

napi_value WatchAndWaitWrapper(napi_env env, napi_callback_info args) {
    size_t argc = 2;
    napi_value argv[2];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 2) {
        napi_throw_error(env, nullptr, "Se requieren más argumentos para WatchAndWait");
        return nullptr;
    }

    InputCallbacks input = {};

    napi_value notifyFileAddedCallback;

    if(napi_get_named_property(env, argv[1], "notifyFileAddedCallback", &notifyFileAddedCallback) == napi_ok) {
        napi_create_reference(env, notifyFileAddedCallback, 1, &input.notify_file_added_callback_ref);
    }

    napi_valuetype valuetype;
    napi_status type_status = napi_typeof(env, notifyFileAddedCallback, &valuetype);
    if (type_status != napi_ok || valuetype != napi_function) {
        napi_throw_error(env, nullptr, "notifyFileAddedCallback should be a function.");
        return nullptr;
    }


    //==============================
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
        &notify_file_added_threadsafe_callback
    );


    InputSyncCallbacksThreadsafe inputThreadsafe = {};
    inputThreadsafe.notify_file_added_threadsafe_callback = notify_file_added_threadsafe_callback;

    LPCWSTR syncRootPath;
    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);
    syncRootPath = new WCHAR[pathLength + 1];
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t*>(const_cast<wchar_t*>(syncRootPath)), pathLength + 1, nullptr);

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

    // Aquí se debe obtener los valores de FILETIME de los argumentos. Para simplificar, estoy usando valores ficticios.
    creationTime.dwLowDateTime = 12345678;
    creationTime.dwHighDateTime = 87654321;

    lastWriteTime.dwLowDateTime = 98765432;
    lastWriteTime.dwHighDateTime = 23456789;

    lastAccessTime.dwLowDateTime = 34567890;
    lastAccessTime.dwHighDateTime = 78901234;

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
