#include "stdafx.h"
#include "SyncRoot.h"
#include "Callbacks.h"
#include <iostream>

static napi_threadsafe_function global_tsfn = nullptr;
static napi_threadsafe_function global_tsfn_delete = nullptr;

void ExecuteAsyncWork(napi_env env, void* data) {
    CallbackContext* context = (CallbackContext*)data;
}

void CompleteDeleteCallbackAsyncWork(napi_env env, napi_status status, void* data) {
    CallbackContext* context = (CallbackContext*)data;
    wprintf(L"CompleteAsyncWork\n");
    if (status != napi_ok) {
        // Manejar error aquí.
    }

    if (context->callbacks.notifyDeleteCompletionCallbackRef) {
        napi_value callbackFn;
        napi_status status = napi_get_reference_value(env, context->callbacks.notifyDeleteCompletionCallbackRef, &callbackFn);
        
        if (status != napi_ok) {
            wprintf(L"Error in obtaining the callback reference value.\n");
            return;
        }

        std::string utf8Str(
            context->callbackArgs.notifyDeleteCompletionArgs.fileIdentity.begin(),
            context->callbackArgs.notifyDeleteCompletionArgs.fileIdentity.end()
        );

        napi_value global;
        napi_get_global(env, &global);

        napi_value arg;
        napi_create_string_utf8(env, utf8Str.c_str(), NAPI_AUTO_LENGTH, &arg);
        
        napi_value result;
        napi_make_callback(env, nullptr, global, callbackFn, 1, &arg, &result);
    }
}

void CompleteRenameCallbackAsyncWork(napi_env env, napi_status status, void* data) {
    CallbackContext* context = (CallbackContext*)data;
    wprintf(L"CompleteAsyncWork\n");
    if (status != napi_ok) {
        // Manejar error aquí.
    }

    if (context->callbacks.notifyRenameCallbackRef) {
        napi_value callbackFn;
        napi_status status = napi_get_reference_value(env, context->callbacks.notifyRenameCallbackRef, &callbackFn);

        if (status != napi_ok) {
            wprintf(L"Error in obtaining the callback reference value.\n");
            return;
        }
        
        napi_value global;
        napi_get_global(env, &global);

        napi_value arg;
        napi_create_string_utf8(env, "Hola", NAPI_AUTO_LENGTH, &arg);

        napi_value result;
        napi_make_callback(env, nullptr, global, callbackFn, 0, nullptr, &result);
    }
}

void CALLBACK NotifyRenameCompletionCallbackWrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {
    wprintf(L"NotifyRenameCompletionCallbackWrapper 1\n");
    if (global_tsfn == nullptr) {
        // Manejar error aquí.
        return;
    }

    wprintf(L"NotifyRenameCompletionCallbackWrapper 2\n");
    CallbackContext* context = GlobalContextContainer::GetContext();
    if (context == nullptr) {
        wprintf(L"Context is null. Aborting.\n");
        return;
    }

    wprintf(L"NotifyRenameCompletionCallbackWrapper 3\n");
    std::wstring fileIdentity;
    context->callbackArgs.notifyDeleteCompletionArgs.fileIdentity = fileIdentity;

    wprintf(L"NotifyRenameCompletionCallbackWrapper 4\n");
    napi_status status = napi_call_threadsafe_function(global_tsfn, nullptr, napi_tsfn_blocking);
    if (status != napi_ok) {
        // Manejar error aquí.
        wprintf(L"Callback called unsuccessfully.\n");
    } else {
        wprintf(L"Callback called successfully.\n");
    }

    CF_OPERATION_PARAMETERS opParams = {0};
    opParams.AckDelete.CompletionStatus = STATUS_SUCCESS;
    opParams.ParamSize = sizeof(CF_OPERATION_PARAMETERS);

    CF_OPERATION_INFO opInfo = {0};
    opInfo.StructSize = sizeof(CF_OPERATION_INFO);
    opInfo.Type = CF_OPERATION_TYPE_ACK_RENAME;
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

void CALLBACK NotifyDeleteCallbackWrapper(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo,
    _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {
    wprintf(L"NotifyDeleteCallbackWrapper 1\n");
    if (global_tsfn_delete == nullptr) {
        // Manejar error aquí.
        return;
    }

    wprintf(L"NotifyDeleteCallbackWrapper 2\n");
    CallbackContext* context = GlobalContextContainer::GetContext();
    if (context == nullptr) {
        wprintf(L"Context is null. Aborting.\n");
        return;
    }

    wprintf(L"NotifyDeleteCallbackWrapper 3\n");
    LPCVOID fileIdentity = callbackInfo->FileIdentity;
    DWORD fileIdentityLength = callbackInfo->FileIdentityLength;

    const wchar_t* wchar_ptr = static_cast<const wchar_t*>(fileIdentity);
    std::wstring fileIdentityStr(wchar_ptr, fileIdentityLength / sizeof(wchar_t));
    wprintf(L"fileIdentityStr: %s\n", fileIdentityStr.c_str());

    wprintf(L"NotifyDeleteCallbackWrapper 4\n");
    //static_cast<void*>(&fileIdentity)
    std::wstring* dataToSend = new std::wstring(fileIdentityStr);

    napi_status status = napi_call_threadsafe_function(global_tsfn_delete, dataToSend, napi_tsfn_blocking);
    
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

void SetupGlobalTsfn(napi_threadsafe_function tsfn) {
    global_tsfn = tsfn;
}

void SetupGlobalTsfnDelete(napi_threadsafe_function tsfn) {
    global_tsfn_delete = tsfn;
}

void emptyThreadFinalize(napi_env env, void* finalize_data, void* finalize_hint) {
    wprintf(L"emptyThreadFinalize\n");
}

void emptyJsCall(napi_env env, napi_value js_callback, void* context, void* data) {
    wprintf(L"emptyJsCall\n");
}

void js_thread_cb(napi_env env, napi_value js_callback, void* context, void* data) {
    std::wstring* receivedData = static_cast<std::wstring*>(data);
    napi_value js_string;
    napi_create_string_utf16(env, reinterpret_cast<const char16_t*>(receivedData->c_str()), receivedData->size(), &js_string);

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_value result;
    napi_call_function(env, undefined, js_callback, 1, &js_string, &result);

    delete receivedData;
}


SyncCallbacks TransformInputCallbacksToSyncCallbacks(napi_env env, InputSyncCallbacks input) {
    SyncCallbacks sync;

    wprintf(L"TransformInputCallbacksToSyncCallbacks\n");
    CallbackContext *context = new CallbackContext{env, {}, input};

    //DeleteCompletion
    napi_value deleteCallback;
    napi_create_string_utf8(env, "DeleteCallback", NAPI_AUTO_LENGTH, &deleteCallback);
    napi_create_async_work(env, NULL, deleteCallback, ExecuteAsyncWork, CompleteDeleteCallbackAsyncWork, context, &context->callbacksWorks.notifyDeleteCompletionCallbackWork);

    //Remame Completion
    napi_value renameCallback;
    napi_create_string_utf8(env, "RenameCallback", NAPI_AUTO_LENGTH, &renameCallback);
    napi_create_async_work(env, NULL, renameCallback, ExecuteAsyncWork, CompleteRenameCallbackAsyncWork, context, &context->callbacksWorks.notifyRenameCompletionCallbackWork);


    GlobalContextContainer::SetContext(context);

    // Create a threadsafe function for rename callback
    napi_threadsafe_function threadsafe_function;
    napi_value jsFunction;
    napi_status status_ref = napi_get_reference_value(env, input.notifyRenameCallbackRef, &jsFunction);

    napi_valuetype valuetype;
    napi_status type_status = napi_typeof(env, jsFunction, &valuetype);

    if (type_status != napi_ok || valuetype != napi_function) {
        fprintf(stderr, "jsFunction is not a function.\n");
        abort();
    }

    napi_value resource_name;
    napi_create_string_utf8(env, "asyncWork", NAPI_AUTO_LENGTH, &resource_name);
    napi_status status = napi_create_threadsafe_function(
        env,
        jsFunction,
        NULL,
        resource_name,
        0,
        1,
        NULL,
        NULL,
        NULL,
        NULL,
        &threadsafe_function
    );

    if (status != napi_ok) {
        const napi_extended_error_info* errorInfo = NULL;
        napi_get_last_error_info(env, &errorInfo);
        fprintf(stderr, "Failed to create threadsafe function: %s\n", errorInfo->error_message);
        fprintf(stderr, "N-API Status Code: %d\n", errorInfo->error_code);
        fprintf(stderr, "Engine-specific error code: %u\n", errorInfo->engine_error_code);
        // No imprima `engine_reserved` a menos que esté seguro de lo que contiene,
        // ya que podría ser un puntero a una ubicación desconocida
        abort(); // Esto finalizará el programa
    }

    // Create a threadsafe function for delete callback
    napi_threadsafe_function threadsafe_function_delete;
    napi_value jsFunctionDelete;
    napi_status status_ref_delete = napi_get_reference_value(env, input.notifyDeleteCompletionCallbackRef, &jsFunctionDelete);

    napi_valuetype valuetype_delete;
    napi_status type_status_delete = napi_typeof(env, jsFunctionDelete, &valuetype_delete);

    if (type_status_delete != napi_ok || valuetype_delete != napi_function) {
        fprintf(stderr, "jsFunction is not a function.\n");
        abort();
    }

    napi_value resource_name_delete;
    napi_create_string_utf8(env, "asyncWorkDelete", NAPI_AUTO_LENGTH, &resource_name_delete);
    napi_status status_delete = napi_create_threadsafe_function(
        env,
        jsFunctionDelete,
        NULL,
        resource_name_delete,
        0,
        1,
        NULL,
        NULL,
        NULL,
        js_thread_cb,
        &threadsafe_function_delete
    );

    if (status_delete != napi_ok) {
        const napi_extended_error_info* errorInfo = NULL;
        napi_get_last_error_info(env, &errorInfo);
        fprintf(stderr, "Failed to create threadsafe function: %s\n", errorInfo->error_message);
        fprintf(stderr, "N-API Status Code: %d\n", errorInfo->error_code);
        fprintf(stderr, "Engine-specific error code: %u\n", errorInfo->engine_error_code);
        // No imprima `engine_reserved` a menos que esté seguro de lo que contiene,
        // ya que podría ser un puntero a una ubicación desconocida
        abort(); // Esto finalizará el programa
    }


    // validate if status is ok


    SetupGlobalTsfn(threadsafe_function);
    SetupGlobalTsfnDelete(threadsafe_function_delete);

    sync.notifyDeleteCompletionCallback = NotifyDeleteCallbackWrapper;
    sync.notifyRenameCallback = NotifyRenameCompletionCallbackWrapper;

    return sync;
}

void AddCustomState(
    _In_ winrt::IVector<winrt::StorageProviderItemPropertyDefinition> &customStates,
    _In_ LPCWSTR displayNameResource,
    _In_ int id)
{
    winrt::StorageProviderItemPropertyDefinition customState;
    customState.DisplayNameResource(displayNameResource);
    customState.Id(id);
    customStates.Append(customState);
}

HRESULT SyncRoot::RegisterSyncRoot(const wchar_t *syncRootPath, const wchar_t *providerName, const wchar_t *providerVersion, const GUID &providerId)
{
    try
    {
        auto syncRootID = providerId;

        winrt::StorageProviderSyncRootInfo info;
        info.Id(L"syncRootID");

        auto folder = winrt::StorageFolder::GetFolderFromPathAsync(syncRootPath).get();
        info.Path(folder);

        // The string can be in any form acceptable to SHLoadIndirectString.
        info.DisplayNameResource(providerName);

        // This icon is just for the sample. You should provide your own branded icon here
        info.IconResource(L"%SystemRoot%\\system32\\charmap.exe,0");
        info.HydrationPolicy(winrt::StorageProviderHydrationPolicy::Full);
        info.HydrationPolicyModifier(winrt::StorageProviderHydrationPolicyModifier::None);
        info.PopulationPolicy(winrt::StorageProviderPopulationPolicy::AlwaysFull);
        info.InSyncPolicy(winrt::StorageProviderInSyncPolicy::FileCreationTime | winrt::StorageProviderInSyncPolicy::DirectoryCreationTime);
        info.Version(L"1.0.0");
        info.ShowSiblingsAsGroup(false);
        info.HardlinkPolicy(winrt::StorageProviderHardlinkPolicy::None);

        winrt::Uri uri(L"http://cloudmirror.example.com/recyclebin");
        info.RecycleBinUri(uri);

        // Context
        std::wstring syncRootIdentity(syncRootPath);
        syncRootIdentity.append(L"->");
        syncRootIdentity.append(L"TestProvider");

        wchar_t const contextString[] = L"TestProviderContextString";
        winrt::IBuffer contextBuffer = winrt::CryptographicBuffer::ConvertStringToBinary(syncRootIdentity.data(), winrt::BinaryStringEncoding::Utf8);
        info.Context(contextBuffer);

        winrt::IVector<winrt::StorageProviderItemPropertyDefinition> customStates = info.StorageProviderItemPropertyDefinitions();
        AddCustomState(customStates, L"CustomStateName1", 1);
        AddCustomState(customStates, L"CustomStateName2", 2);
        AddCustomState(customStates, L"CustomStateName3", 3);

        winrt::StorageProviderSyncRootManager::Register(info);

        Sleep(1000);

        return S_OK;
    }
    catch (...)
    {
        wprintf(L"Could not register the sync root, hr %08x\n", static_cast<HRESULT>(winrt::to_hresult()));
    }
}

HRESULT SyncRoot::UnregisterSyncRoot()
{
    try
    {
        winrt::StorageProviderSyncRootManager::Unregister(L"syncRootID");
        return S_OK;
    }
    catch (...)
    {
        // winrt::to_hresult() will eat the exception if it is a result of winrt::check_hresult,
        // otherwise the exception will get rethrown and this method will crash out as it should
        wprintf(L"Could not unregister the sync root, hr %08x\n", static_cast<HRESULT>(winrt::to_hresult()));
    }
}

HRESULT SyncRoot::ConnectSyncRoot(const wchar_t *syncRootPath, InputSyncCallbacks syncCallbacks, napi_env env, CF_CONNECTION_KEY *connectionKey)
{
    try
    {
        Utilities::AddFolderToSearchIndexer(syncRootPath);

        SyncCallbacks transformedCallbacks = TransformInputCallbacksToSyncCallbacks(env, syncCallbacks);

        CF_CALLBACK_REGISTRATION callbackTable[] = {
            {CF_CALLBACK_TYPE_NOTIFY_DELETE, transformedCallbacks.notifyDeleteCompletionCallback},
            {CF_CALLBACK_TYPE_NOTIFY_RENAME, transformedCallbacks.notifyRenameCallback},
            CF_CALLBACK_REGISTRATION_END
        };

        HRESULT hr = CfConnectSyncRoot(
            syncRootPath,
            callbackTable,
            nullptr, // Contexto (opcional)
            CF_CONNECT_FLAG_REQUIRE_PROCESS_INFO | CF_CONNECT_FLAG_REQUIRE_FULL_FILE_PATH,
            connectionKey
        );

        CallbackContext* context = GlobalContextContainer::GetContext();

        return hr;
    }
    catch (const std::exception &e)
    {
        wprintf(L"Excepción capturada: %hs\n", e.what());
        // Aquí puedes decidir si retornar un código de error específico o mantener el E_FAIL.
    }
    catch (...)
    {
        wprintf(L"Excepción desconocida capturada\n");
        // Igualmente, puedes decidir el código de error a retornar.
    }
}