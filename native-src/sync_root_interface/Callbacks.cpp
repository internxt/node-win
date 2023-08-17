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

    if (context->callbacks.notifyDeleteCompletionCallbackRef) { // <--- Notar que cambiamos a "Ref" aquí para comprobar si la referencia está establecida.
        wprintf(L"NotifyDeleteCompletionCallbackWrapper 2\n");

        napi_value callbackFn;
        napi_get_reference_value(context->env, context->callbacks.notifyDeleteCompletionCallbackRef, &callbackFn);

        napi_value global;
        napi_get_global(context->env, &global);
        
        napi_value result;
        napi_make_callback(context->env, nullptr, global, callbackFn, 0, nullptr, &result);
    }
}
