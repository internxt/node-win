#include "stdafx.h"
#include "CallbacksBase.h"
#include "NotifyRename.h"

void CALLBACK NotifyRenameCallbackWrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {
    CallbackContext* context = GlobalContextContainer::GetContext();

    wprintf(L"this 2");
    if (context == nullptr) {
        wprintf(L"Context is null. Aborting.\n");
        return;
    }

    LPCVOID fileIdentity = callbackInfo->FileIdentity;
    DWORD fileIdentityLength = callbackInfo->FileIdentityLength;

    wprintf(L"NotifyRenameCallbackWrapper: %s\n", fileIdentity);
    wprintf(L"NotifyRenameCallbackWrapper: %d\n", fileIdentityLength);

    // const wchar_t* wchar_ptr = static_cast<const wchar_t*>(fileIdentity);
    // std::wstring fileIdentityStr(wchar_ptr, fileIdentityLength / sizeof(wchar_t));

    // context->callbackArgs.notifyDeleteCompletionArgs.fileIdentity = fileIdentityStr;

    napi_queue_async_work(context->env, context->callbacksWorks.notifyRenameCompletionCallbackWork);
}