#include "stdafx.h"
#include "CallbacksBase.h"
#include "NotifyDeleteCompletion.h"

CallbackContext* GlobalContextContainer::currentContext = nullptr;

void CALLBACK NotifyDeleteCompletionCallbackWrapper(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {    
    CallbackContext* context = GlobalContextContainer::GetContext();
    
    wprintf(L"this 1\n");

    if (context == nullptr) {
        wprintf(L"Context is null. Aborting.\n");
        return;
    }

    LPCVOID fileIdentity = callbackInfo->FileIdentity;
    DWORD fileIdentityLength = callbackInfo->FileIdentityLength;

    const wchar_t* wchar_ptr = static_cast<const wchar_t*>(fileIdentity);
    std::wstring fileIdentityStr(wchar_ptr, fileIdentityLength / sizeof(wchar_t));

    context->callbackArgs.notifyDeleteCompletionArgs.fileIdentity = fileIdentityStr;

    napi_queue_async_work(context->env, context->callbacksWorks.notifyDeleteCompletionCallbackWork);
    
    // napi_env env = context->env;
    // napi_handle_scope scope; // <-- Declare a handle scope variable
    // napi_open_handle_scope(env, &scope); // <-- Open a handle scope

    // if (context->callbacks.notifyDeleteCompletionCallbackRef) {
    //     napi_env env = context->env;
    //     napi_value callbackFn;
    //     napi_status status = napi_get_reference_value(env, context->callbacks.notifyDeleteCompletionCallbackRef, &callbackFn);

    //     if (status != napi_ok) {
    //         wprintf(L"Error in obtaining the callback reference value.\n");
    //         return;
    //     }

    //     std::string utf8Str(
    //         context->callbackArgs.notifyDeleteCompletionArgs.fileIdentity.begin(),
    //         context->callbackArgs.notifyDeleteCompletionArgs.fileIdentity.end()
    //     );

    //     napi_value global;
    //     napi_get_global(env, &global);

    //     napi_value arg;
    //     napi_create_string_utf8(env, utf8Str.c_str(), NAPI_AUTO_LENGTH, &arg);

    //     napi_value result;
    //     napi_call_function(env, global, callbackFn, 1, &arg, &result); // Synchronous call to the JavaScript callback
    // }

    // napi_close_handle_scope(env, scope); // <-- Close the handle scope
}
