#include "stdafx.h"
#include "SyncRoot.h"
#include "Callbacks.h"

void ExecuteAsyncWork(napi_env env, void* data) {
    wprintf(L"ExecuteAsyncWork iniciado\n");
    CallbackContext* context = (CallbackContext*)data;
    // Realizar operaciones costosas aquí.
    // NO llames a ninguna función de la API de Node.js desde aquí.
}

void CompleteAsyncWork(napi_env env, napi_status status, void* data) {
    wprintf(L"CompleteAsyncWork iniciado\n");
    CallbackContext* context = (CallbackContext*)data;

    // Si hubo un error en `ExecuteAsyncWork`, el parámetro `status` no será `napi_ok`.
    if (status != napi_ok) {
        // Manejar error aquí.
    }

    // Si necesitas invocar un callback de JavaScript o realizar otras operaciones que requieran la API de Node.js, hazlo aquí.
    if (context->callbacks.notifyDeleteCompletionCallbackRef) {
        napi_value callbackFn;
        napi_status status = napi_get_reference_value(env, context->callbacks.notifyDeleteCompletionCallbackRef, &callbackFn);
    
        if (status != napi_ok) {
            wprintf(L"Error al obtener el valor de referencia del callback\n");
            return;
        }
    
        napi_value global;
        napi_get_global(env, &global);
        
        napi_value result;
        napi_make_callback(env, nullptr, global, callbackFn, 0, nullptr, &result);
    }

    // Limpiar el trabajo asíncrono y cualquier otra limpieza necesaria.
    napi_delete_async_work(env, context->work);
    // GlobalContextContainer::ClearContext();
    // delete context;
}

SyncCallbacks TransformInputCallbacksToSyncCallbacks(napi_env env, InputSyncCallbacks input) {
    SyncCallbacks sync;

    CallbackContext *context = new CallbackContext{env, input};

    napi_value resource_name;
    napi_create_string_utf8(env, "CloudStorage", NAPI_AUTO_LENGTH, &resource_name);

    napi_create_async_work(env, NULL, resource_name, ExecuteAsyncWork, CompleteAsyncWork, context, &context->work);

    GlobalContextContainer::SetContext(context);

    sync.notifyDeleteCompletionCallback = NotifyDeleteCompletionCallbackWrapper;

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

        // Give the cache some time to invalidate
        Sleep(1000);

        return S_OK;
    }
    catch (...)
    {
        // winrt::to_hresult() will eat the exception if it is a result of winrt::check_hresult,
        // otherwise the exception will get rethrown and this method will crash out as it should
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

        // SyncCallbacks transformedCallbacks = TransformInputCallbacksToSyncCallbacks(env, syncCallbacks);

        CF_CALLBACK_REGISTRATION callbackTable[] = {
            {CF_CALLBACK_TYPE_NOTIFY_DELETE_COMPLETION, DeleteDataNotificationCallback},
            CF_CALLBACK_REGISTRATION_END};

        HRESULT hr = CfConnectSyncRoot(
            syncRootPath,
            callbackTable,
            nullptr, // Contexto (opcional)
            CF_CONNECT_FLAG_REQUIRE_PROCESS_INFO | CF_CONNECT_FLAG_REQUIRE_FULL_FILE_PATH,
            connectionKey);

        CallbackContext* context = GlobalContextContainer::GetContext();

        wprintf(L"Connect Finished\n\n");
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