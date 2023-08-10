#include "stdafx.h"
#include "Placeholders.h"


extern "C" {
    void CALLBACK FetchDataCallback_C(
        _In_ CF_CALLBACK_INFO* callbackInfo,
        _In_ CF_CALLBACK_PARAMETERS* callbackParameters
    );

    void CALLBACK FetchPlaceholdersCallback_C(
        _In_ CF_CALLBACK_INFO* callbackInfo,
        _In_ CF_CALLBACK_PARAMETERS* callbackParameters
    );
}

HRESULT RegisterSyncRoot(const wchar_t* syncRootPath) {
    CF_SYNC_REGISTRATION registration = {};
    CF_SYNC_POLICIES policies = {};

    // Configurar el registro (esto es un esbozo, deberás adaptarlo a tus necesidades)
    registration.StructSize = sizeof(registration);
    registration.ProviderName = L"MiProveedor"; // El nombre de tu proveedor
    registration.ProviderVersion = L"1.0"; // Versión de tu proveedor
    registration.ProviderId = {}; // Puedes generar un GUID único para tu proveedor

    // Configurar las políticas
    policies.StructSize = sizeof(policies);
    policies.Hydration.Primary = CF_HYDRATION_POLICY_FULL;
    policies.Population.Primary = CF_POPULATION_POLICY_FULL;

    // Llamar a CfRegisterSyncRoot
    HRESULT hr = CfRegisterSyncRoot(
        syncRootPath,
        &registration,
        &policies,
        CF_REGISTER_FLAG_UPDATE // Por ejemplo, para re-registrar previamente sync roots registrados
    );

    return hr;
}

HRESULT UnregisterSyncRoot(const wchar_t* syncRootPath) {
    HRESULT hr = CfUnregisterSyncRoot(syncRootPath);
    return hr;
}

void Placeholders::CreateOne(
    _In_ PCWSTR fileName,
    _In_ PCWSTR fileIdentity,
    uint32_t fileSize,
    DWORD fileIdentityLength,
    uint32_t fileAttributes,
    FILETIME creationTime,
    FILETIME lastWriteTime,
    FILETIME lastAccessTime,
    _In_ PCWSTR destPath)
{
    try
    {
        /* Doc: https://learn.microsoft.com/en-us/windows/win32/api/cfapi/ns-cfapi-cf_placeholder_create_info */
        CF_PLACEHOLDER_CREATE_INFO cloudEntry = {};

        std::wstring fullDestPath = std::wstring(destPath) + L'\\'; // fileName;

        cloudEntry.FileIdentity = fileIdentity;
        cloudEntry.FileIdentityLength = fileIdentityLength;

        cloudEntry.RelativeFileName = fileName;
        cloudEntry.Flags = CF_PLACEHOLDER_CREATE_FLAG_MARK_IN_SYNC;

        cloudEntry.FsMetadata.FileSize.QuadPart = fileSize;  // Set the appropriate file size
        cloudEntry.FsMetadata.BasicInfo.FileAttributes = fileAttributes;
        cloudEntry.FsMetadata.BasicInfo.CreationTime = Utilities::FileTimeToLargeInteger(creationTime);
        cloudEntry.FsMetadata.BasicInfo.LastWriteTime = Utilities::FileTimeToLargeInteger(lastWriteTime);
        cloudEntry.FsMetadata.BasicInfo.LastAccessTime = Utilities::FileTimeToLargeInteger(lastAccessTime);
        cloudEntry.FsMetadata.BasicInfo.ChangeTime = Utilities::FileTimeToLargeInteger(lastWriteTime);

        wprintf(L"Creating placeholder for %s\n", fileName);

        RegisterSyncRoot(destPath);
    
        CF_CALLBACK_REGISTRATION callbackTable[] = {
            { CF_CALLBACK_TYPE_FETCH_DATA, FetchDataCallback },
            { CF_CALLBACK_TYPE_FETCH_PLACEHOLDERS, FetchPlaceholdersCallback },
            CF_CALLBACK_REGISTRATION_END
        };

        CF_CONNECTION_KEY connectionKey;

        CfConnectSyncRoot(
          destPath, callbackTable, 
          nullptr,           // Contexto (opcional)
          CF_CONNECT_FLAG_NONE, // Flags
          &connectionKey
        );
        /* Checks the result of placeholder creation, on succesful creation the value should be STATUS_OK */
        //winrt::check_hresult(CfCreatePlaceholders(fullDestPath.c_str(), &cloudEntry, 1, CF_CREATE_FLAG_NONE, NULL));
        try {
            winrt::check_hresult(CfCreatePlaceholders(fullDestPath.c_str(), &cloudEntry, 1, CF_CREATE_FLAG_NONE, NULL));
            // Resto del código que se ejecuta después de una llamada exitosa...
        }
        catch (const winrt::hresult_error& error) {
            // Manejo del error aquí
            wprintf(L"Error al crear placeholder: %s", error.message().c_str());
        }

        // Apply custom state if needed
        winrt::StorageProviderItemProperty prop;
        prop.Id(1);
        prop.Value(L"Value1");
        prop.IconResource(L"shell32.dll,-44");

        // print destPath
        wprintf(L"\ndestPath: %s\n", destPath);
        // print fileName
        wprintf(L"fileName: %s\n", fileName);

        wprintf(L"Applying custom state for %s\n", fileName);
        Utilities::ApplyCustomStateToPlaceholderFile(destPath, fileName, prop);
    }
    catch (...)
    {
        wprintf(L"Failed to create or customize placeholder with %08x\n", static_cast<HRESULT>(winrt::to_hresult()));
    }
}