#include "stdafx.h"
#include "Callbacks.h"
#include <iostream>
#include <fstream>

CallbackContext* GlobalContextContainer::currentContext = nullptr;

void CALLBACK NotifyDeleteCompletionCallbackWrapper(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {
    wprintf(L"Value of callbackInfo: %p\n", callbackInfo);
    CallbackContext* context = GlobalContextContainer::GetContext();
    
    if (context == nullptr) {
        wprintf(L"Context is null. Aborting.\n");
        return;
    }

    wprintf(L"Value of context: %p\n", context);
    wprintf(L"NotifyDeleteCompletionCallbackWrapper 1\n");
    wprintf(L"Value of callback: %p\n", context->callbacks.notifyDeleteCompletionCallbackRef);

    // Encolar el trabajo asíncrono.
    napi_queue_async_work(context->env, context->work);
}

void CALLBACK DeleteDataNotificationCallback (
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {
    MessageBoxW(NULL, L"Delete Data Notification Callback triggered!", L"Callback Activated", MB_OK);
}
