#include "stdafx.h"
#include "SyncRoot.h"
#include "Callbacks.h"
#include <iostream>

static napi_threadsafe_function global_tsfn = nullptr;

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
    if (global_tsfn == nullptr) {
        // Manejar error aquí.
        return;
    }

    CallbackContext* context = GlobalContextContainer::GetContext();
    if (context == nullptr) {
        wprintf(L"Context is null. Aborting.\n");
        return;
    }

    // Aquí puedes utilizar callbackInfo y callbackParameters como lo necesites.

    std::wstring fileIdentity; // Puedes asignar un valor a fileIdentity aquí, si es necesario.
    context->callbackArgs.notifyDeleteCompletionArgs.fileIdentity = fileIdentity;

    napi_status status = napi_call_threadsafe_function(global_tsfn, nullptr, napi_tsfn_nonblocking);
    if (status != napi_ok) {
        // Manejar error aquí.
    }
}

void SetupGlobalTsfn(napi_threadsafe_function tsfn) {
    global_tsfn = tsfn;
}

void emptyThreadFinalize(napi_env env, void* finalize_data, void* finalize_hint) {
    // No hacer nada
}

void emptyJsCall(napi_env env, napi_value js_callback, void* context, void* data) {
    // No hacer nada
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

    napi_threadsafe_function threadsafe_function;
    napi_value jsFunction;
    napi_status status_ref = napi_get_reference_value(env, input.notifyRenameCallbackRef, &jsFunction);

    napi_valuetype valuetype;
    napi_status type_status = napi_typeof(env, jsFunction, &valuetype);

    if (type_status != napi_ok || valuetype != napi_function) {
        fprintf(stderr, "jsFunction is not a function.\n");
        abort();
    }

    napi_status status = napi_create_threadsafe_function(
        env,
        jsFunction,
        NULL,
        NULL,
        0,
        1,
        NULL,
        emptyThreadFinalize,
        NULL,
        emptyJsCall,
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


    // validate if status is ok


    SetupGlobalTsfn(threadsafe_function);

    sync.notifyDeleteCompletionCallback = NotifyDeleteCompletionCallbackWrapper;
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
            {CF_CALLBACK_TYPE_NOTIFY_DELETE_COMPLETION, transformedCallbacks.notifyDeleteCompletionCallback},
            {CF_CALLBACK_TYPE_NOTIFY_RENAME_COMPLETION, transformedCallbacks.notifyRenameCallback},
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