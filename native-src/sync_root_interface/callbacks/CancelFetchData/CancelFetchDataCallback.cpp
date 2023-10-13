#include "stdafx.h"
#include <Callbacks.h>
#include <cfapi.h>

napi_threadsafe_function g_cancel_delete_fetch_data_threadsafe_callback = nullptr;

inline std::mutex mtx;
inline std::condition_variable cv;
inline bool ready = false;

struct CancelFetchDataArgs
{
    std::wstring fileIdentityArg;
};

void setup_global_tsfn_cancel_fetch_data(napi_threadsafe_function tsfn)
{
    wprintf(L"setup_global_tsfn_cancel_fetch_data called\n");
    g_cancel_delete_fetch_data_threadsafe_callback = tsfn;
}

void notify_cancel_fetch_data_call(napi_env env, napi_value js_callback, void *context, void *data)
{
    CancelFetchDataArgs *args = static_cast<CancelFetchDataArgs *>(data);
    std::u16string u16_fileIdentity(args->fileIdentityArg.begin(), args->fileIdentityArg.end());

    napi_value js_string;
    napi_create_string_utf16(env, u16_fileIdentity.c_str(), u16_fileIdentity.size(), &js_string);

    napi_value args_to_js_callback_cancel_fetch[1] = {js_string};

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_value result;

    napi_status status = napi_call_function(env, undefined, js_callback, 1, args_to_js_callback_cancel_fetch, &result);
    if (status != napi_ok)
    {
        fprintf(stderr, "Failed to call JS function.\n");
        return;
    }

    cv.notify_one();

    delete args;
}

void register_threadsafe_cancel_fetch_data_callback(const std::string &resource_name, napi_env env, InputSyncCallbacks input)
{
    std::u16string converted_resource_name = std::u16string(resource_name.begin(), resource_name.end());

    napi_value resource_name_value;
    napi_create_string_utf16(env, converted_resource_name.c_str(), NAPI_AUTO_LENGTH, &resource_name_value);

    napi_threadsafe_function tsfn_cancel_fetch_data;
    napi_value cancel_fetch_data_value;
    napi_status status_ref = napi_get_reference_value(env, input.cancel_fetch_data_callback_ref, &cancel_fetch_data_value);

    napi_status status = napi_create_threadsafe_function(
        env,
        cancel_fetch_data_value,
        NULL,
        resource_name_value,
        0,
        1,
        NULL,
        NULL,
        NULL,
        notify_cancel_fetch_data_call,
        &tsfn_cancel_fetch_data);

    if (status != napi_ok)
    {
        fprintf(stderr, "Failed to create threadsafe function.\n");
        return;
    }
    wprintf(L"Threadsafe function created.\n");
    setup_global_tsfn_cancel_fetch_data(tsfn_cancel_fetch_data);
}

void CALLBACK cancel_fetch_data_callback_wrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {
    printf("fetch_data_callback_wrapper\n");

    LPCVOID fileIdentity = callbackInfo->FileIdentity;
    DWORD fileIdentityLength = callbackInfo->FileIdentityLength;

    const wchar_t *wchar_ptr = static_cast<const wchar_t *>(fileIdentity);
    std::wstring fileIdentityStr(wchar_ptr, fileIdentityLength / sizeof(wchar_t));

    if (g_cancel_delete_fetch_data_threadsafe_callback == nullptr)
    {
        wprintf(L"Callback fetch_data_callback_wrapper called but g_fetch_data_threadsafe_callback is null\n");
        return;
    }

    CancelFetchDataArgs *args = new CancelFetchDataArgs();
    args->fileIdentityArg = fileIdentityStr;

    napi_status status = napi_call_threadsafe_function(g_cancel_delete_fetch_data_threadsafe_callback, args, napi_tsfn_blocking);

    if (status != napi_ok)
    {
        wprintf(L"Callback called unsuccessfully.\n");
    };

    {
        std::unique_lock<std::mutex> lock(mtx);
        while (!ready)
        {
            cv.wait(lock);
        }
    }

    ready = false; 
}