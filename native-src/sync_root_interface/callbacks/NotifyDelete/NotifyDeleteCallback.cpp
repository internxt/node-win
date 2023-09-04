#include <Callbacks.h>

void CALLBACK NotifyDeleteCallbackWrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {
    CallbackContext* context = GlobalCallbackContextContainer::GetContext();

    LPCVOID fileIdentity = callbackInfo->FileIdentity;
    DWORD fileIdentityLength = callbackInfo->FileIdentityLength;

    const wchar_t* wchar_ptr = static_cast<const wchar_t*>(fileIdentity);
    std::wstring fileIdentityStr(wchar_ptr, fileIdentityLength / sizeof(wchar_t));

    std::wstring* dataToSend = new std::wstring(fileIdentityStr);

    napi_status status = napi_call_threadsafe_function(context->threadsafe_callbacks.notify_delete_threadsafe_callback, dataToSend, napi_tsfn_blocking);
    
    wprintf(L"status: %d\n", status);
    
    if (status != napi_ok) {
        wprintf(L"Callback called unsuccessfully.\n");
    }

    CF_OPERATION_PARAMETERS opParams = {0};
    opParams.AckDelete.CompletionStatus = STATUS_SUCCESS;
    opParams.ParamSize = sizeof(CF_OPERATION_PARAMETERS);

    CF_OPERATION_INFO opInfo = {0};
    opInfo.StructSize = sizeof(CF_OPERATION_INFO);
    opInfo.Type = CF_OPERATION_TYPE_ACK_DELETE;
    opInfo.ConnectionKey = callbackInfo->ConnectionKey;
    opInfo.TransferKey = callbackInfo->TransferKey;

    HRESULT hr = CfExecute(
        &opInfo,
        &opParams
    );

    if (FAILED(hr))
    {
        wprintf(L"Error in CfExecute().\n");
        wprintf(L"Error in CfExecute(), HRESULT: %lx\n", hr);
    }
}