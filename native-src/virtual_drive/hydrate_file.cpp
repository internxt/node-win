#include <thread>
#include <string>
#include <Windows.h>
#include "Logger.h"
#include "SyncRoot.h"

struct AsyncWork {
    napi_async_work work;
    napi_deferred deferred;
    std::wstring fullPath;
    std::string error;
    bool success;
};

void execute_work(napi_env env, void* data) {
    AsyncWork* asyncWork = static_cast<AsyncWork*>(data);

    try {
        SyncRoot::HydrateFile(asyncWork->fullPath.c_str());
        Logger::getInstance().log("finish... " + Logger::fromWStringToString(asyncWork->fullPath.c_str()), LogLevel::INFO);
        asyncWork->success = true;
    } catch (const std::exception& e) {
        asyncWork->error = e.what();
        asyncWork->success = false;
    } catch (...) {
        asyncWork->error = "Unknown error";
        asyncWork->success = false;
    }
}

void complete_work(napi_env env, napi_status status, void* data) {
    AsyncWork* asyncWork = static_cast<AsyncWork*>(data);

    if (asyncWork->success) {
        napi_value result;
        napi_get_undefined(env, &result);
        napi_resolve_deferred(env, asyncWork->deferred, result);
    } else {
        napi_value error;
        napi_create_string_utf8(env, asyncWork->error.c_str(), NAPI_AUTO_LENGTH, &error);
        napi_reject_deferred(env, asyncWork->deferred, error);
    }

    napi_delete_async_work(env, asyncWork->work);
    delete asyncWork;
}

napi_value hydrate_file_impl(napi_env env, napi_callback_info args)
{
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, args, &argc, argv, nullptr, nullptr);

    // Get string length and content
    size_t pathLength;
    napi_get_value_string_utf16(env, argv[0], nullptr, 0, &pathLength);

    std::wstring fullPath(pathLength, L'\0');
    napi_get_value_string_utf16(env, argv[0], reinterpret_cast<char16_t*>(&fullPath[0]), pathLength + 1, nullptr);

    // Create promise
    napi_deferred deferred;
    napi_value promise;
    napi_create_promise(env, &deferred, &promise);

    // Create and queue async work
    AsyncWork* asyncWork = new AsyncWork{};
    asyncWork->deferred = deferred;
    asyncWork->fullPath = std::move(fullPath);

    napi_value resourceName;
    napi_create_string_utf8(env, "HydrateFileAsync", NAPI_AUTO_LENGTH, &resourceName);

    napi_create_async_work(env, nullptr, resourceName, execute_work, complete_work, asyncWork, &asyncWork->work);
    napi_queue_async_work(env, asyncWork->work);

    return promise;
}
