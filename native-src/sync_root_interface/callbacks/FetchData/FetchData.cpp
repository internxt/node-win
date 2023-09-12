#include <Callbacks.h>
#include <string>
#include <condition_variable>
#include <mutex>

napi_threadsafe_function g_fetch_data_threadsafe_callback = nullptr;

inline std::mutex mtx;
inline std::condition_variable cv;
inline bool ready = false;
inline bool callbackResult = false;

struct FetchDataArgs
{
    std::wstring fileIdentityArg;
};

void setup_global_tsfn_fetch_data(napi_threadsafe_function tsfn)
{
    wprintf(L"setup_global_tsfn_fetch_data called\n");
    g_fetch_data_threadsafe_callback = tsfn;
}

// response callback fn
napi_value response_callback_fn_fetch_data(napi_env env, napi_callback_info info)
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
    wprintf(L"response_callback_fn_fetch_data called\n");
    cv.notify_one();

    return nullptr;
}

void notify_fetch_data_call(napi_env env, napi_value js_callback, void *context, void *data)
{
    wprintf(L"notify_fetch_data_call called\n");
    napi_status status;
    FetchDataArgs *args = static_cast<FetchDataArgs *>(data);
    napi_value js_fileIdentityArg, undefined, result;

    std::u16string u16_fileIdentity(args->fileIdentityArg.begin(), args->fileIdentityArg.end());

    napi_create_string_utf16(env, u16_fileIdentity.c_str(), u16_fileIdentity.size(), &js_fileIdentityArg);

    napi_value js_response_callback_fn;
    napi_create_function(env, "responseCallback", NAPI_AUTO_LENGTH, response_callback_fn_fetch_data, nullptr, &js_response_callback_fn);

    napi_value args_to_js_callback[2] = {js_fileIdentityArg, js_response_callback_fn};

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

    status = napi_call_function(env, undefined, js_callback, 2, args_to_js_callback, &result);
    if (status != napi_ok)
    {
        fprintf(stderr, "Failed to call JS function.\n");
        return;
    }
    delete args;
}

void register_threadsafe_fetch_data_callback(const std::string &resource_name, napi_env env, InputSyncCallbacks input)
{
    std::u16string converted_resource_name = std::u16string(resource_name.begin(), resource_name.end());

    napi_value resource_name_value;
    napi_create_string_utf16(env, converted_resource_name.c_str(), NAPI_AUTO_LENGTH, &resource_name_value);

    napi_threadsafe_function tsfn_fetch_data;
    napi_value fetch_data_value;
    napi_status status_ref = napi_get_reference_value(env, input.fetch_data_callback_ref, &fetch_data_value);

    napi_status status = napi_create_threadsafe_function(
        env,
        fetch_data_value,
        NULL,
        resource_name_value,
        0,
        1,
        NULL,
        NULL,
        NULL,
        notify_fetch_data_call,
        &tsfn_fetch_data);

    if (status != napi_ok)
    {
        fprintf(stderr, "Failed to create threadsafe function.\n");
        return;
    }
    wprintf(L"Threadsafe function created.\n");
    setup_global_tsfn_fetch_data(tsfn_fetch_data);
}

void CALLBACK fetch_data_callback_wrapper(
    _In_ CONST CF_CALLBACK_INFO *callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS *callbackParameters)
{
    wprintf(L"Callback fetch_data_callback_wrapper called\n");

    // get callbackinfo
    wprintf(L"fileId = %s\n", callbackInfo->FileIdentity);
    wprintf(L"fileIdLength = %d\n", callbackInfo->FileIdentityLength);
    wprintf(L"connectionKey = %d\n", callbackInfo->ConnectionKey);
    wprintf(L"transferKey = %d\n", callbackInfo->TransferKey);
    wprintf(L"normalized path = %d\n", callbackInfo->NormalizedPath);

    LPCVOID fileIdentity = callbackInfo->FileIdentity;
    DWORD fileIdentityLength = callbackInfo->FileIdentityLength;

    const wchar_t *wchar_ptr = static_cast<const wchar_t *>(fileIdentity);
    std::wstring fileIdentityStr(wchar_ptr, fileIdentityLength / sizeof(wchar_t));

    FetchDataArgs *args = new FetchDataArgs();
    args->fileIdentityArg = fileIdentityStr;
    wprintf(L"Callback fetch_data_callback_wrapper called\n");
    wprintf(L"g_fetch_data_threadsafe_callback = %s\n", g_fetch_data_threadsafe_callback);
    napi_status status = napi_call_threadsafe_function(g_fetch_data_threadsafe_callback, args, napi_tsfn_blocking);

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

    opParams.AckData.CompletionStatus = STATUS_SUCCESS; // callbackResult ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

    CF_OPERATION_INFO opInfo = {0};
    opInfo.StructSize = sizeof(CF_OPERATION_INFO);
    opInfo.Type = CF_OPERATION_TYPE_ACK_DATA; // todo: check if this is the correct type
    opInfo.ConnectionKey = callbackInfo->ConnectionKey;
    opInfo.TransferKey = callbackInfo->TransferKey;

    HRESULT hr = CfExecute(&opInfo, &opParams);

    if (FAILED(hr))
    {
        wprintf(L"Error in CfExecute().\n");
        wprintf(L"Error in CfExecute(), HRESULT: %lx\n", hr);
    }

    std::lock_guard<std::mutex> lock(mtx);
    ready = false; // Reset ready

    // delete args;
    // todo: function to fetch data
    // todo: call napi_call_threadsafe_function
}