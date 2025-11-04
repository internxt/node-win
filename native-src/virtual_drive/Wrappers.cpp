#include <stdafx.h>
#include <sstream>
#include <Placeholders.h>
#include <Callbacks.h>
#include <LoggerPath.h>
#include <Logger.h>
#include <SyncRoot.h>
#include <codecvt>
#include <locale>
#include <vector>
#include <register_sync_root_wrapper.h>
#include <create_folder_placeholder.h>
#include <create_file_placeholder.h>
#include <connect_sync_root.h>
#include <hydrate_file.h>
#include <convert_to_placeholder.h>
#include <get_registered_sync_roots_wrapper.h>
#include <unregister_sync_root_wrapper.h>
#include <dehydrate_file.h>
#include <disconnect_sync_root.h>
#include <get_placeholder_state_wrapper.h>
#include <update_sync_status_wrapper.h>
#include <NAPI_SAFE_WRAP.h>

napi_value CreateFilePlaceholderWrapper(napi_env env, napi_callback_info args)
{
    return NAPI_SAFE_WRAP(env, args, create_file_placeholder_impl);
}

napi_value UnregisterSyncRootWrapper(napi_env env, napi_callback_info args)
{
    return NAPI_SAFE_WRAP(env, args, unregister_sync_root_wrapper);
}

napi_value RegisterSyncRootWrapper(napi_env env, napi_callback_info info)
{
    return NAPI_SAFE_WRAP(env, info, register_sync_root_wrapper);
}

napi_value GetRegisteredSyncRootsWrapper(napi_env env, napi_callback_info args)
{
    return NAPI_SAFE_WRAP(env, args, get_registered_sync_roots_wrapper);
}

napi_value ConnectSyncRootWrapper(napi_env env, napi_callback_info args)
{
    return NAPI_SAFE_WRAP(env, args, connect_sync_root_impl);
}

napi_value CreateFolderPlaceholderWrapper(napi_env env, napi_callback_info args)
{
    return NAPI_SAFE_WRAP(env, args, create_folder_placeholder_impl);
}

napi_value DisconnectSyncRootWrapper(napi_env env, napi_callback_info args)
{
    return NAPI_SAFE_WRAP(env, args, disconnect_sync_root);
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
    return NAPI_SAFE_WRAP(env, args, update_sync_status_wrapper);
}

napi_value GetPlaceholderStateWrapper(napi_env env, napi_callback_info args)
{
    return NAPI_SAFE_WRAP(env, args, get_placeholder_state_wrapper);
}

napi_value ConvertToPlaceholderWrapper(napi_env env, napi_callback_info args)
{
    return NAPI_SAFE_WRAP(env, args, convert_to_placeholder_wrapper);
}

napi_value HydrateFileWrapper(napi_env env, napi_callback_info args)
{
    return NAPI_SAFE_WRAP(env, args, hydrate_file_impl);
}

napi_value DehydrateFileWrapper(napi_env env, napi_callback_info args)
{
    return NAPI_SAFE_WRAP(env, args, dehydrate_file);
}
