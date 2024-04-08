#include <Callbacks.h>
#include <Logger.h>
#include <string>
#include <condition_variable>
#include <mutex>

napi_threadsafe_function g_fetch_placeholder_threadsafe_callback = nullptr;

inline std::mutex mtx;
inline std::condition_variable cv;
inline bool ready = false;
inline bool callbackResult = false;

void setup_global_tsfn_fetch_placeholder(napi_threadsafe_function tsfn)
{
    g_fetch_placeholder_threadsafe_callback = tsfn;
}

void CALLBACK fetch_placeholders_callback_wrapper(
    _In_ CONST CF_CALLBACK_INFO *callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS *callbackParameters)
{
    Logger::getInstance().log("Callback fetch_placeholders_callback_wrapper called\n", LogLevel::DEBUG);
}