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
#include "create_file_placeholder.h"
#include "get_file_identity.h"
#include "connect_sync_root.h"
#include "hydrate_file.h"
#include "convert_to_placeholder.h"
#include "NAPI_SAFE_WRAP.h"

std::string WStringToUTF8(const std::wstring &wstr)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(wstr);
}

napi_value CreatePlaceholderFile(napi_env env, napi_callback_info args)
{
    return NAPI_SAFE_WRAP(env, args, create_file_placeholder_impl);
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
    return NAPI_SAFE_WRAP(env, info, register_sync_root_impl);
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
    return NAPI_SAFE_WRAP(env, args, connect_sync_root_impl);
}

napi_value CreateEntryWrapper(napi_env env, napi_callback_info args)
{
    return NAPI_SAFE_WRAP(env, args, create_folder_placeholder_impl);
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
    return NAPI_SAFE_WRAP(env, args, get_file_identity_impl);
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

napi_value ConvertToPlaceholderWrapper(napi_env env, napi_callback_info args) {
    return NAPI_SAFE_WRAP(env, args, convert_to_placeholder_impl);
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
    return NAPI_SAFE_WRAP(env, args, hydrate_file_impl);
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