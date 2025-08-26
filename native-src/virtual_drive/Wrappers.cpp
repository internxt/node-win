#include "stdafx.h"
#include <sstream>
#include "Placeholders.h"
#include "Callbacks.h"
#include "LoggerPath.h"
#include <Logger.h>
#include <SyncRoot.h>
#include <codecvt>
#include <locale>
#include <vector>
#include "register_sync_root.h"
#include "create_folder_placeholder.h"
#include "napi_safe_wrap.h"

std::string WStringToUTF8(const std::wstring &wstr)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(wstr);
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

/**
 * v2.5.7 Carlos Gonzalez
 * Added backward compatibility for the default virtual drive identifier "syncRootID".
 * If the provided ID is "syncRootID", it will be unregistered without throwing an error,
 * maintaining support for previous versions that used it instead of a GUID.
 */

napi_value UnregisterSyncRootWrapper(napi_env env, napi_callback_info args)
{
    size_t argc = 1;
    napi_value argv[1];

    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, nullptr, "The provider ID is required for UnregisterSyncRoot");
        return nullptr;
    }

    GUID providerId;
    LPCWSTR providerIdStr;
    size_t providerIdStrLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &providerIdStrLength);
    providerIdStr = new WCHAR[providerIdStrLength + 1];
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t *>(const_cast<wchar_t *>(providerIdStr)), providerIdStrLength + 1, nullptr);


    HRESULT result;
    HRESULT guidResult = CLSIDFromString(providerIdStr, &providerId);

    if (SUCCEEDED(guidResult))
    {
        result = SyncRoot::UnregisterSyncRoot(providerId);
    }
    else if (wcscmp(providerIdStr, L"syncRootID") == 0)
    {
        result = SyncRoot::UnregisterSyncRoot(providerIdStr);
    }
    else
    {
        napi_throw_error(env, nullptr, "Invalid provider ID: must be a GUID or 'syncRootID'");
        delete[] providerIdStr;
        return nullptr;
    }

    delete[] providerIdStr;

    napi_value napiResult;
    napi_create_int32(env, static_cast<int32_t>(result), &napiResult);
    return napiResult;
}

napi_value RegisterSyncRootWrapper(napi_env env, napi_callback_info info) {
    return napi_safe_wrap(env, info, register_sync_root_impl);
}

napi_value GetRegisteredSyncRootsWrapper(napi_env env, napi_callback_info args)
{
    try
    {
        std::vector<SyncRoots> roots = SyncRoot::GetRegisteredSyncRoots();

        napi_value jsArray;
        napi_status status = napi_create_array_with_length(env, roots.size(), &jsArray);
        if (status != napi_ok)
            throw std::runtime_error("Error creating the array");

        for (size_t i = 0; i < roots.size(); i++)
        {
            napi_value jsObj;
            status = napi_create_object(env, &jsObj);
            if (status != napi_ok)
                throw std::runtime_error("Error creating the object");

            std::string id = WStringToUTF8(roots[i].id);
            napi_value napiId;
            status = napi_create_string_utf8(env, id.c_str(), id.size(), &napiId);
            if (status != napi_ok)
                throw std::runtime_error("Error creating the string id");
            napi_set_named_property(env, jsObj, "id", napiId);

            std::string path = WStringToUTF8(roots[i].path);
            napi_value napiPath;
            status = napi_create_string_utf8(env, path.c_str(), path.size(), &napiPath);
            if (status != napi_ok)
                throw std::runtime_error("Error creating the string path");
            napi_set_named_property(env, jsObj, "path", napiPath);

            std::string displayName = WStringToUTF8(roots[i].displayName);
            napi_value napiDisplayName;
            status = napi_create_string_utf8(env, displayName.c_str(), displayName.size(), &napiDisplayName);
            if (status != napi_ok)
                throw std::runtime_error("Error creating the string displayName");
            napi_set_named_property(env, jsObj, "displayName", napiDisplayName);

            std::string version = WStringToUTF8(roots[i].version);
            napi_value napiVersion;
            status = napi_create_string_utf8(env, version.c_str(), version.size(), &napiVersion);
            if (status != napi_ok)
                throw std::runtime_error("Error creating the string version");
            napi_set_named_property(env, jsObj, "version", napiVersion);

            status = napi_set_element(env, jsArray, i, jsObj);
            if (status != napi_ok)
                throw std::runtime_error("Error setting the element in the array");
        }

        return jsArray;
    }
    catch (const std::exception &ex)
    {
        napi_throw_error(env, nullptr, ex.what());
        return nullptr;
    }
    catch (...)
    {
        napi_throw_error(env, nullptr, "An unknown error occurred in GetRegisteredSyncRootsWrapper");
        return nullptr;
    }
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

        napi_value fetchDataCallback;
        napi_value cancelFetchDataCallback;

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

napi_value CreateEntryWrapper(napi_env env, napi_callback_info args)
{
    return napi_safe_wrap(env, args, create_folder_placeholder_impl);
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

    HRESULT result = SyncRoot::DisconnectSyncRoot(syncRootPath);

    delete[] syncRootPath;

    napi_value napiResult;
    napi_create_int32(env, static_cast<int32_t>(result), &napiResult);
    return napiResult;
}

napi_value GetFileIdentityWrapper(napi_env env, napi_callback_info args)
{
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
    FileState state = Placeholders::GetPlaceholderInfo(widePath.get());

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

napi_value ConvertToPlaceholderWrapper(napi_env env, napi_callback_info args)
{
    size_t argc = 2;
    napi_value argv[2];
    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 2)
    {
        napi_throw_type_error(env, nullptr, "Wrong number of arguments");
        return nullptr;
    }

    char path[1024];
    char serverIdentity[1024];
    size_t pathLen, serverIdentityLen;

    napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &pathLen);
    napi_get_value_string_utf8(env, argv[1], serverIdentity, sizeof(serverIdentity), &serverIdentityLen);

    std::wstring wPath(path, path + pathLen);
    std::wstring wServerIdentity(serverIdentity, serverIdentity + serverIdentityLen);

    PlaceholderResult result = Placeholders::ConvertToPlaceholder(wPath, wServerIdentity);

    napi_value resultObj;
    napi_create_object(env, &resultObj);

    napi_value successValue;
    napi_get_boolean(env, result.success, &successValue);
    napi_set_named_property(env, resultObj, "success", successValue);

    if (!result.success)
    {
        std::string errorMessage(result.errorMessage.begin(), result.errorMessage.end());
        napi_value errorValue;
        napi_create_string_utf8(env, errorMessage.c_str(), errorMessage.length(), &errorValue);
        napi_set_named_property(env, resultObj, "errorMessage", errorValue);
    }

    return resultObj;
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

napi_value HydrateFileWrapper(napi_env env, napi_callback_info args)
{
    size_t argc = 1;
    napi_value argv[1];
    napi_value thisArg;
    napi_get_cb_info(env, args, &argc, argv, &thisArg, nullptr);

    if (argc < 1)
    {
        napi_throw_type_error(env, nullptr, "The file path is required for HydrateFile");
        return nullptr;
    }

    // Obtener el argumento de JavaScript y convertirlo a una cadena de C++
    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);
    std::wstring fullPath(pathLength, L'\0');
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t *>(&fullPath[0]), pathLength + 1, nullptr);

    // Crear una promesa
    napi_deferred deferred;
    napi_value promise;
    napi_create_promise(env, &deferred, &promise);

    // Lanzar la operación asíncrona en un hilo separado
    std::thread([deferred, fullPath, env]()
                {
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
        } })
        .detach();

    return promise;
}

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

napi_value GetPlaceholderAttributeWrapper(napi_env env, napi_callback_info args)
{
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_type_error(env, nullptr, "Both source and destination paths are required");
        return nullptr;
    }

    // Obtener los argumentos de JavaScript y convertirlos a cadenas de C++
    std::wstring sourcePath, destinationPath;
    size_t sourcePathLength, destinationPathLength;

    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &sourcePathLength);
    sourcePath.resize(sourcePathLength);
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t *>(&sourcePath[0]), sourcePathLength + 1, nullptr);

    // Llamar a la función TransferData
    PlaceholderAttribute attribute = Placeholders::GetAttribute(sourcePath);

    napi_value result;
    napi_create_object(env, &result);

    napi_value jsAtrtibute;
    napi_create_int32(env, static_cast<int32_t>(attribute), &jsAtrtibute);
    napi_set_named_property(env, result, "attribute", jsAtrtibute);

    return result;
}