#include <Windows.h>
#include "Logger.h"
#include "SyncRoot.h"

napi_value hydrate_file_impl(napi_env env, napi_callback_info args)
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
      
    wprintf(L"FINISHHHHHn");

    return promise;
}
