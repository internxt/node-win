#include "Callbacks.h"
#include "DirectoryWatcher.h"
#include "Utilities.h"
#include "ProcessTypes.h"

inline std::mutex mtx;
inline std::condition_variable cv;
inline bool ready = false;
inline bool callbackResult = false;
inline std::wstring server_identity;
#include <filesystem>

struct ProcessNotifications
{
    std::wstring message;
    std::wstring action;
    std::wstring errorName;
};

napi_value response_callback_fn_nofify(napi_env env, napi_callback_info info)
{
    // just 1 argument
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    if (argc < 1)
    {
        wprintf(L"[Error] This function must receive at least two arguments");
        return nullptr;
    }
    napi_valuetype valueType;

    // Verificar el primer argumento: debería ser un booleano
    napi_typeof(env, argv[0], &valueType);
    if (valueType != napi_boolean)
    {
        wprintf(L"[Error] First argument should be boolean\n");
        return nullptr;
    }
    bool confirmation_response;
    napi_get_value_bool(env, argv[0], &confirmation_response);

    std::lock_guard<std::mutex> lock(mtx);
    ready = true;
    callbackResult = confirmation_response;

    cv.notify_one();

    return nullptr;
}

void notify_message_call(napi_env env, napi_value js_callback, void *context, void *data)
{
    napi_status status;
    // std::wstring *receivedData = static_cast<std::wstring *>(data);
    ProcessNotifications *args = static_cast<ProcessNotifications *>(data);
    napi_value js_message, js_action, js_errorName, js_response_callback_fn, undefined, result;
    // asign data to the js_string
    std::u16string u16_action(args->action.begin(), args->action.end());
    std::u16string u16_errorName(args->errorName.begin(), args->errorName.end());
    std::u16string u16_message(args->message.begin(), args->message.end());
    // napi_create_string_utf16(env, reinterpret_cast<const char16_t *>(receivedData->c_str()), receivedData->size(), &js_string1);

    status = napi_create_string_utf16(env, u16_action.c_str(), u16_action.size(), &js_action);
    if (status != napi_ok)
    {
        fprintf(stderr, "[Error] Failed to create u16_action string.\n");
        return;
    }

    status = napi_create_string_utf16(env, u16_errorName.c_str(), u16_errorName.size(), &js_errorName);
    if (status != napi_ok)
    {
        fprintf(stderr, "[Error] Failed to create u16_errorName string.\n");
        return;
    }

    status = napi_create_string_utf16(env, u16_message.c_str(), u16_message.size(), &js_message);
    if (status != napi_ok)
    {
        fprintf(stderr, "[Error] Failed to create u16_message string.\n");
        return;
    }

    // asign the nofify callback function to the js_response_callback_fn
    napi_create_function(env, "responseCallback", NAPI_AUTO_LENGTH, response_callback_fn_nofify, nullptr, &js_response_callback_fn);

    napi_value args_to_js_callback[4] = {js_message, js_action, js_errorName, js_response_callback_fn};
    status = napi_get_undefined(env, &undefined);
    if (status != napi_ok)
    {
        fprintf(stderr, "[Error] Failed to get undefined value.\n");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        ready = false;
    }

    status = napi_call_function(env, undefined, js_callback, 4, args_to_js_callback, &result);
    if (status != napi_ok)
    {
        fprintf(stderr, "[Error] Failed to call JS function.\n");
        return;
    }
    delete args;
}

void register_threadsafe_message_callback(FileChange &change, const std::string &resource_name, napi_env env, InputSyncCallbacksThreadsafe input)
{
    ProcessNotifications *args = new ProcessNotifications();
    switch (change.type)
    {
    case ERROR_FILE_SIZE_EXCEEDED:
        args->action = Utilities::FileOperationErrorToWString(FileOperationError::UPLOAD_ERROR);
        args->errorName = Utilities::ProcessErrorNameToWString(ProcessErrorName::FILE_TOO_BIG);
        args->message = change.message;
        break;
    default:
        break;
    }

    napi_status status = napi_call_threadsafe_function(input.notify_message_threadsafe_callback, args, napi_tsfn_blocking);

    {
        std::unique_lock<std::mutex> lock(mtx);
        while (!ready)
        {
            cv.wait(lock);
        }
    }
};