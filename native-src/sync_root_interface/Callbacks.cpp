#include "stdafx.h"
#include "Callbacks.h"
#include <iostream>
#include <fstream>

CallbackContext* GlobalContextContainer::currentContext = nullptr;

std::wstring s2ws(const std::string& s) {
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
}


void CALLBACK NotifyDeleteCompletionCallbackWrapper(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {    
    CallbackContext* context = GlobalContextContainer::GetContext();
    
    if (context == nullptr) {
        wprintf(L"Context is null. Aborting.\n");
        return;
    }

    LPCVOID fileIdentity = callbackInfo->FileIdentity;
    DWORD fileIdentityLength = callbackInfo->FileIdentityLength;

    // Convertimos el puntero a un puntero wchar_t, y luego construimos una cadena con él.
    const wchar_t* wchar_ptr = static_cast<const wchar_t*>(fileIdentity);
    std::wstring fileIdentityStr(wchar_ptr, fileIdentityLength / sizeof(wchar_t));

    // Imprimimos la cadena.
    wprintf(L"File Identity: %s\n", fileIdentityStr.c_str());

    napi_queue_async_work(context->env, context->work);
}

void DeleteDataNotificationCallback(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {
    CallbackContext* context = GlobalContextContainer::GetContext();

    if (context == nullptr) {
        wprintf(L"Context is null. Aborting.\n");
        return;
    }

    napi_value callbackFn;
    napi_status status = napi_get_reference_value(context->env, context->callbacks.notifyDeleteCompletionCallbackRef, &callbackFn);

    napi_valuetype valuetype;
    status = napi_typeof(context->env, callbackFn, &valuetype);
    if (status == napi_ok && valuetype == napi_function) {
        status = napi_call_function(context->env, callbackFn, nullptr, 0, nullptr, nullptr);
    } else {
        wprintf(L"Callback is not a function!\n");
    }
}
