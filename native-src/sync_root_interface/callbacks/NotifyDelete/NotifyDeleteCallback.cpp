#include <Callbacks.h>
#include <Logger.h>
#include <string>
#include <condition_variable>
#include <mutex>

napi_threadsafe_function g_notify_delete_threadsafe_callback = nullptr;

inline std::mutex mtx;
inline std::condition_variable cv;
inline bool ready = false;
inline bool callbackResult = false;

void setup_global_tsfn_delete(napi_threadsafe_function tsfn)
{
    g_notify_delete_threadsafe_callback = tsfn;
}

struct NotifyDeleteArgs
{
    std::wstring targetPathArg;
    std::wstring fileIdentityArg;
};

napi_value response_callback_fn_delete(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    if (argc < 1)
    {
        // Manejar error
        return nullptr;
    }

    bool response;
    napi_get_value_bool(env, argv[0], &response);

    std::lock_guard<std::mutex> lock(mtx);
    ready = true;
    callbackResult = response;
    cv.notify_one();

    return nullptr;
}

void notify_delete_call(napi_env env, napi_value js_callback, void *context, void *data)
{
    std::wstring *receivedData = static_cast<std::wstring *>(data);
    napi_value js_string;
    napi_create_string_utf16(env, reinterpret_cast<const char16_t *>(receivedData->c_str()), receivedData->size(), &js_string);

    // Crear la función C++ como un valor de N-API para pasar a JS
    napi_value js_response_callback_fn;
    napi_create_function(env, "responseCallback", NAPI_AUTO_LENGTH, response_callback_fn_delete, nullptr, &js_response_callback_fn);

    napi_value args_to_js_callback[2] = {js_string, js_response_callback_fn};

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_value result;

    napi_status status = napi_call_function(env, undefined, js_callback, 2, args_to_js_callback, &result);
    if (status != napi_ok)
    {
        Logger::getInstance().log("Failed to call JS function.\n", LogLevel::ERROR);
        return;
    }

    cv.notify_one();

    delete receivedData;
}

// void notify_delete_call(napi_env env, napi_value js_callback, void* context, void* data) {
//     std::wstring* receivedData = static_cast<std::wstring*>(data);
//     napi_value js_string;
//     napi_create_string_utf16(env, reinterpret_cast<const char16_t*>(receivedData->c_str()), receivedData->size(), &js_string);

//     napi_value undefined;
//     napi_get_undefined(env, &undefined);
//     napi_value result;

//     napi_status status = napi_call_function(env, undefined, js_callback, 1, &js_string, &result);
//     if (status != napi_ok) {
//         return;
//     }

//     bool js_result = false;  // Variable para almacenar el resultado booleano
//     status = napi_get_value_bool(env, result, &js_result);  // Obtiene el valor booleano desde el objeto napi_value
//     if (status != napi_ok) {
//         return;
//     }

//     {
//         std::lock_guard<std::mutex> lock(mtx);
//         ready = js_result;
//     }
//     cv.notify_one();

//     delete receivedData;
// }

void register_threadsafe_notify_delete_callback(const std::string &resource_name, napi_env env, InputSyncCallbacks input)
{
    std::u16string converted_resource_name = std::u16string(resource_name.begin(), resource_name.end());

    napi_value resource_name_value;
    napi_create_string_utf16(env, converted_resource_name.c_str(), NAPI_AUTO_LENGTH, &resource_name_value);

    napi_threadsafe_function notify_delete_threadsafe_callback;
    napi_value notify_delete_callback;
    napi_status status_ref = napi_get_reference_value(env, input.notify_delete_callback_ref, &notify_delete_callback);

    if (notify_delete_callback == nullptr)
    {
        Logger::getInstance().log("notify_delete_callback is null\n", LogLevel::WARN);
        return;
    }

    napi_status status = napi_create_threadsafe_function(
        env,
        notify_delete_callback,
        NULL,
        resource_name_value,
        0,
        1,
        NULL,
        NULL,
        NULL,
        notify_delete_call,
        &notify_delete_threadsafe_callback);

    if (status != napi_ok)
    {
        const napi_extended_error_info *errorInfo = NULL;
        napi_get_last_error_info(env, &errorInfo);
        std::stringstream ss;
        ss << "Failed to create threadsafe function: %s\n", errorInfo->error_message;
        std::string message = ss.str();
        Logger::getInstance().log(message, LogLevel::ERROR);

        ss << "N-API Status Code: %d\n", errorInfo->error_code;
        message = ss.str();

        Logger::getInstance().log(message, LogLevel::ERROR);

        ss << "Engine-specific error code: %u\n", errorInfo->engine_error_code;
        message = ss.str();

        Logger::getInstance().log(message, LogLevel::ERROR);
        abort();
    }

    setup_global_tsfn_delete(notify_delete_threadsafe_callback);
}

void CALLBACK notify_delete_callback_wrapper(
    _In_ CONST CF_CALLBACK_INFO *callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS *callbackParameters)
{
    LPCVOID fileIdentity = callbackInfo->FileIdentity;
    DWORD fileIdentityLength = callbackInfo->FileIdentityLength;

    const wchar_t *wchar_ptr = static_cast<const wchar_t *>(fileIdentity);
    std::wstring fileIdentityStr(wchar_ptr, fileIdentityLength / sizeof(wchar_t));

    std::wstring *dataToSend = new std::wstring(fileIdentityStr);

    napi_status status = napi_call_threadsafe_function(g_notify_delete_threadsafe_callback, dataToSend, napi_tsfn_blocking);

    if (status != napi_ok)
    {
        Logger::getInstance().log("Callback called unsuccessfully.\n", LogLevel::ERROR);
    }

    CF_OPERATION_PARAMETERS opParams = {0};

    {
        std::unique_lock<std::mutex> lock(mtx);
        while (!ready)
        {
            cv.wait(lock);
        }
    }

    opParams.AckDelete.CompletionStatus = callbackResult ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    opParams.ParamSize = sizeof(CF_OPERATION_PARAMETERS);

    CF_OPERATION_INFO opInfo = {0};
    opInfo.StructSize = sizeof(CF_OPERATION_INFO);
    opInfo.Type = CF_OPERATION_TYPE_ACK_DELETE;
    opInfo.ConnectionKey = callbackInfo->ConnectionKey;
    opInfo.TransferKey = callbackInfo->TransferKey;

    HRESULT hr = CfExecute(
        &opInfo,
        &opParams);

    {
        std::lock_guard<std::mutex> lock(mtx);
        ready = false; // Reset ready
    }

    if (FAILED(hr))
    {
        std::stringstream ss;
        ss << "Error in CfExecute() delete action, HRESULT: %lx\n", hr;
        std::string message = ss.str();
        Logger::getInstance().log(message, LogLevel::ERROR);
    }
}