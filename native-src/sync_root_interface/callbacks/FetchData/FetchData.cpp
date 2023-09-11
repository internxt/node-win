#include <Callbacks.h>
#include <string>
#include <condition_variable>
#include <mutex>

napi_threadsafe_function g_fetch_data_threadsafe_callback = nullptr;

inline std::mutex mtx;
inline std::condition_variable cv;
inline bool ready = false;
inline bool callbackResult = false;

void setup_global_tsfn_fetch_data(napi_threadsafe_function tsfn)
{
    g_fetch_data_threadsafe_callback = tsfn;
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

    // get callbackparameters
    // todo: function to fetch data
    // todo: call napi_call_threadsafe_function
}