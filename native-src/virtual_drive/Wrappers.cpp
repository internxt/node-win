#include "stdafx.h"
#include <node_api.h>
#include <sstream>
#include "Wrappers.h"
#include "Placeholders.h"
#include "SyncRoot.h"
#include "SyncRootWatcher.h"
#include "Callbacks.h"

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

    LPCWSTR fileIdentity;
    size_t fileIdentityLength;
    napi_get_value_string_utf16(env, argv[1], nullptr, 0, &fileIdentityLength);
    fileIdentity = new WCHAR[fileIdentityLength + 1];
    napi_get_value_string_utf16(env, argv[1], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(fileIdentity)), fileIdentityLength + 1, nullptr);

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
    size_t argc = 1;
    napi_value argv[1];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, nullptr, "Se requiere la ruta del sync root para ConnectSyncRoot");
        return nullptr;
    }

    LPCWSTR syncRootPath;
    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);
    syncRootPath = new WCHAR[pathLength + 1];
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t*>(const_cast<wchar_t*>(syncRootPath)), pathLength + 1, nullptr);

    // CALLBACKS
    InputSyncCallbacks callbacks = {};

    napi_value fetchDataCallbackJS;
    if (napi_get_named_property(env, argv[1], "fetchDataCallback", &fetchDataCallbackJS) == napi_ok) {
        callbacks.fetchDataCallback = reinterpret_cast<NAPI_CALLBACK_FUNCTION>(fetchDataCallbackJS);
    }

    napi_value validateDataCallback;
    if (napi_get_named_property(env, argv[1], "validateDataCallback", &validateDataCallback) == napi_ok) {
        callbacks.validateDataCallback = reinterpret_cast<NAPI_CALLBACK_FUNCTION>(validateDataCallback);
    }

    napi_value cancelFetchDataCallback;
    if (napi_get_named_property(env, argv[1], "cancelFetchDataCallback", &cancelFetchDataCallback) == napi_ok) {
        callbacks.cancelFetchDataCallback = reinterpret_cast<NAPI_CALLBACK_FUNCTION>(cancelFetchDataCallback);
    }

    napi_value fetchPlaceholdersCallback;
    if (napi_get_named_property(env, argv[1], "fetchPlaceholdersCallback", &fetchPlaceholdersCallback) == napi_ok) {
        callbacks.fetchPlaceholdersCallback = reinterpret_cast<NAPI_CALLBACK_FUNCTION>(fetchPlaceholdersCallback);
    }

    napi_value cancelFetchPlaceholdersCallback;
    if (napi_get_named_property(env, argv[1], "cancelFetchPlaceholdersCallback", &cancelFetchPlaceholdersCallback) == napi_ok) {
        callbacks.cancelFetchPlaceholdersCallback = reinterpret_cast<NAPI_CALLBACK_FUNCTION>(cancelFetchPlaceholdersCallback);
    }

    napi_value notifyFileOpenCompletionCallback;
    if (napi_get_named_property(env, argv[1], "notifyFileOpenCompletionCallback", &notifyFileOpenCompletionCallback) == napi_ok) {
        callbacks.notifyFileOpenCompletionCallback = reinterpret_cast<NAPI_CALLBACK_FUNCTION>(notifyFileOpenCompletionCallback);
    }

    napi_value notifyFileCloseCompletionCallback;
    if (napi_get_named_property(env, argv[1], "notifyFileCloseCompletionCallback", &notifyFileCloseCompletionCallback) == napi_ok) {
        callbacks.notifyFileCloseCompletionCallback = reinterpret_cast<NAPI_CALLBACK_FUNCTION>(notifyFileCloseCompletionCallback);
    }

    napi_value notifyDehydrateCallback;
    if (napi_get_named_property(env, argv[1], "notifyDehydrateCallback", &notifyDehydrateCallback) == napi_ok) {
        callbacks.notifyDehydrateCallback = reinterpret_cast<NAPI_CALLBACK_FUNCTION>(notifyDehydrateCallback);
    }

    napi_value notifyDehydrateCompletionCallback;
    if (napi_get_named_property(env, argv[1], "notifyDehydrateCompletionCallback", &notifyDehydrateCompletionCallback) == napi_ok) {
        callbacks.notifyDehydrateCompletionCallback = reinterpret_cast<NAPI_CALLBACK_FUNCTION>(notifyDehydrateCompletionCallback);
    }

    napi_value notifyDeleteCallback;
    if (napi_get_named_property(env, argv[1], "notifyDeleteCallback", &notifyDeleteCallback) == napi_ok) {
        callbacks.notifyDeleteCallback = reinterpret_cast<NAPI_CALLBACK_FUNCTION>(notifyDeleteCallback);
    }

    napi_value notifyDeleteCompletionCallback;
    if (napi_get_named_property(env, argv[1], "notifyDeleteCompletionCallback", &notifyDeleteCompletionCallback) == napi_ok) {
        callbacks.notifyDeleteCompletionCallback = reinterpret_cast<NAPI_CALLBACK_FUNCTION>(notifyDeleteCompletionCallback);
    }

    napi_value notifyRenameCallback;
    if (napi_get_named_property(env, argv[1], "notifyRenameCallback", &notifyRenameCallback) == napi_ok) {
        callbacks.notifyRenameCallback = reinterpret_cast<NAPI_CALLBACK_FUNCTION>(notifyRenameCallback);
    }

    napi_value notifyRenameCompletionCallback;
    if (napi_get_named_property(env, argv[1], "notifyRenameCompletionCallback", &notifyRenameCompletionCallback) == napi_ok) {
        callbacks.notifyRenameCompletionCallback = reinterpret_cast<NAPI_CALLBACK_FUNCTION>(notifyRenameCompletionCallback);
    }

    napi_value noneCallback;
    if (napi_get_named_property(env, argv[1], "noneCallback", &noneCallback) == napi_ok) {
        callbacks.noneCallback = reinterpret_cast<NAPI_CALLBACK_FUNCTION>(noneCallback);
    }

    CF_CONNECTION_KEY connectionKey;
    HRESULT hr = SyncRoot::ConnectSyncRoot(syncRootPath, callbacks, &connectionKey);

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

napi_value WatchAndWaitWrapper(napi_env env, napi_callback_info args) {
    size_t argc = 1;
    napi_value argv[1];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, nullptr, "The sync root path is required for WatchAndWait");
        return nullptr;
    }

    LPCWSTR syncRootPath;
    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);
    syncRootPath = new WCHAR[pathLength + 1];
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t*>(const_cast<wchar_t*>(syncRootPath)), pathLength + 1, nullptr);

    SyncRootWatcher::WatchAndWait(syncRootPath);

    delete[] syncRootPath;

    return nullptr;
}