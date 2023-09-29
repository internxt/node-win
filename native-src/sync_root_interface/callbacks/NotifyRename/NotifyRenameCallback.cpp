#include <Callbacks.h>
#include <Placeholders.h>
#include <string>
#include <filesystem>

napi_threadsafe_function g_notify_rename_threadsafe_callback = nullptr;

inline std::mutex mtx;
inline std::condition_variable cv;
inline bool ready = false;
inline bool callbackResult = false;

napi_ref g_async_complete_ref = nullptr;

struct NotifyRenameArgs
{
    std::wstring targetPathArg;
    std::wstring fileIdentityArg;
};

napi_value response_callback_fn(napi_env env, napi_callback_info info)
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

void notify_rename_call(napi_env env, napi_value js_callback, void *context, void *data)
{
    NotifyRenameArgs *args = static_cast<NotifyRenameArgs *>(data);
    napi_status status;

    std::u16string u16_targetPath(args->targetPathArg.begin(), args->targetPathArg.end());
    std::u16string u16_fileIdentity(args->fileIdentityArg.begin(), args->fileIdentityArg.end());

    napi_value js_targetPathArg, js_fileIdentityArg, undefined, result;

    status = napi_create_string_utf16(env, u16_targetPath.c_str(), u16_targetPath.size(), &js_targetPathArg);
    if (status != napi_ok)
    {
        fprintf(stderr, "Failed to create targetPath string.\n");
        return;
    }

    status = napi_create_string_utf16(env, u16_fileIdentity.c_str(), u16_fileIdentity.size(), &js_fileIdentityArg);
    if (status != napi_ok)
    {
        fprintf(stderr, "Failed to create fileIdentity string.\n");
        return;
    }

    // Crear la función C++ como un valor de N-API para pasar a JS
    napi_value js_response_callback_fn;
    napi_create_function(env, "responseCallback", NAPI_AUTO_LENGTH, response_callback_fn, nullptr, &js_response_callback_fn);

    napi_value args_to_js_callback[3] = {js_targetPathArg, js_fileIdentityArg, js_response_callback_fn};

    status = napi_get_undefined(env, &undefined);
    if (status != napi_ok)
    {
        fprintf(stderr, "Failed to get undefined value.\n");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        ready = false;
    }

    status = napi_call_function(env, undefined, js_callback, 3, args_to_js_callback, &result);
    if (status != napi_ok)
    {
        fprintf(stderr, "Failed to call JS function.\n");
        return;
    }

    delete args;
}

void setup_global_tsfn_rename(napi_threadsafe_function tsfn)
{
    wprintf(L"setup_global_tsfn_rename called\n");
    g_notify_rename_threadsafe_callback = tsfn;
}

void register_threadsafe_notify_rename_callback(const std::string &resource_name, napi_env env, InputSyncCallbacks input)
{
    std::u16string converted_resource_name = std::u16string(resource_name.begin(), resource_name.end());

    napi_value resource_name_value;
    napi_create_string_utf16(env, converted_resource_name.c_str(), NAPI_AUTO_LENGTH, &resource_name_value);

    napi_threadsafe_function notify_rename_threadsafe_callback;
    napi_value notify_rename_value;
    napi_status status_ref = napi_get_reference_value(env, input.notify_rename_callback_ref, &notify_rename_value);

    napi_valuetype valuetype;
    napi_status type_status = napi_typeof(env, notify_rename_value, &valuetype);

    if (type_status != napi_ok || valuetype != napi_function)
    {
        fprintf(stderr, "notify_rename_value is not a function.\n");
        abort();
    }

    napi_status status = napi_create_threadsafe_function(
        env,
        notify_rename_value,
        NULL,
        resource_name_value,
        0,
        1,
        NULL,
        NULL,
        NULL,
        notify_rename_call,
        &notify_rename_threadsafe_callback);

    if (status != napi_ok)
    {
        const napi_extended_error_info *errorInfo = NULL;
        napi_get_last_error_info(env, &errorInfo);
        fprintf(stderr, "Failed to create threadsafe function: %s\n", errorInfo->error_message);
        fprintf(stderr, "N-API Status Code: %d\n", errorInfo->error_code);
        fprintf(stderr, "Engine-specific error code: %u\n", errorInfo->engine_error_code);
        abort();
    }

    setup_global_tsfn_rename(notify_rename_threadsafe_callback);
}

void CALLBACK notify_rename_callback_wrapper(
    _In_ CONST CF_CALLBACK_INFO *callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS *callbackParameters)
{
    LPCVOID fileIdentity = callbackInfo->FileIdentity;
    DWORD fileIdentityLength = callbackInfo->FileIdentityLength;

    const wchar_t *wchar_ptr = static_cast<const wchar_t *>(fileIdentity);
    std::wstring fileIdentityStr(wchar_ptr, fileIdentityLength / sizeof(wchar_t));

    PCWSTR targetPathArg = callbackParameters->Rename.TargetPath;

    NotifyRenameArgs *args = new NotifyRenameArgs();
    args->targetPathArg = std::wstring(targetPathArg);
    args->fileIdentityArg = fileIdentityStr;
    wprintf(L"Callback notify_rename_callback_wrapper called\n");
    wprintf(L"g_notify_rename_threadsafe_callback = %s\n", g_notify_rename_threadsafe_callback);
    napi_status status = napi_call_threadsafe_function(g_notify_rename_threadsafe_callback, args, napi_tsfn_blocking);

    if (status != napi_ok)
    {
        wprintf(L"Callback called unsuccessfully.\n");
    };

    CF_OPERATION_PARAMETERS opParams = {0};

    {
        std::unique_lock<std::mutex> lock(mtx);
        while (!ready)
        {
            cv.wait(lock);
        }
    }

    opParams.AckRename.CompletionStatus = callbackResult ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    opParams.ParamSize = sizeof(CF_OPERATION_PARAMETERS);

    CF_OPERATION_INFO opInfo = {0};
    opInfo.StructSize = sizeof(CF_OPERATION_INFO);
    opInfo.Type = CF_OPERATION_TYPE_ACK_RENAME;
    opInfo.ConnectionKey = callbackInfo->ConnectionKey;
    opInfo.TransferKey = callbackInfo->TransferKey;

    HRESULT hr = CfExecute(
        &opInfo,
        &opParams);

    printf("Mark item as async: %ls\n", targetPathArg);
    WCHAR systemPath[MAX_PATH];
    if (!GetWindowsDirectoryW(systemPath, sizeof(systemPath) / sizeof(WCHAR)))
    {
        wprintf(L"Error al obtener el directorio de Windows: %d\n", GetLastError());
        return;
    }
    // Extrae la letra de la unidad del directorio del sistema
    std::wstring driveLetter(systemPath, 2);
    // Combina la letra de unidad con la ruta relativa si la ruta no existe
    std::wstring absolutePath = targetPathArg;
    if (PathFileExistsW(absolutePath.c_str()) == FALSE)
    {
        absolutePath = driveLetter + L"\\" + targetPathArg;
    }
    // Imprime la ruta
    wprintf(L"absolutePath: %ls\n", absolutePath.c_str());
    bool isDirectory = std::filesystem::is_directory(absolutePath);
    printf("Is directory: %d\n", isDirectory);

    Placeholders::UpdateSyncStatus(absolutePath, callbackResult, isDirectory);

    if (FAILED(hr))
    {
        wprintf(L"Error in CfExecute().\n");
        wprintf(L"Error in CfExecute(), HRESULT: %lx\n", hr);
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        ready = false; // Reset ready
    }
}