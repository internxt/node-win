#include "Thumbnail.h"
#include <iostream>
#include <stdafx.h>

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <propkey.h>
#include <propvarutil.h>

bool FileExists(LPCWSTR fileName) {
    DWORD fileAttr = GetFileAttributesW(fileName);
    return (fileAttr != INVALID_FILE_ATTRIBUTES &&
            !(fileAttr & FILE_ATTRIBUTE_DIRECTORY));
}

void SetThumbnail(const std::wstring& filePath, const std::wstring& thumbnailPath) {
    // HRESULT hr;
    // IShellItem2* pShellItem = nullptr;
    // IPropertyStore* pPropertyStore = nullptr;

    // const std::wstring& filePath2 = L"C:\\Users\\gcarl\\Desktop\\a.png";

    // if ( !FileExists(filePath2.c_str()) ) {
    //     std::wcerr << L"El archivo no existe: " << filePath2 << std::endl;
    //     return;
    // }

    // if ( !FileExists(thumbnailPath.c_str()) ) {
    //     std::wcerr << L"El archivo no existe: " << thumbnailPath << std::endl;
    //     return;
    // }

    // //print filePath and thumbnailPath
    // std::wcout << L"filePath: " << filePath << std::endl;
    // std::wcout << L"thumbnailPath: " << thumbnailPath << std::endl;

    // // Crear un objeto IShellItem desde una ruta de archivo
    // hr = SHCreateItemFromParsingName(filePath.c_str(), nullptr, IID_PPV_ARGS(&pShellItem));
    // //print hr
    // std::wcout << L"hr: " << std::hex << hr << std::endl;
    // if (FAILED(hr)) {
    //     std::wcerr << L"Error al crear el objeto IShellItem: " << std::hex << hr << std::endl;
    //     return;
    // }

    // // Obtener el IPropertyStore para el archivo
    // hr = pShellItem->GetPropertyStore(GETPROPERTYSTOREFLAGS::GPS_READWRITE, IID_PPV_ARGS(&pPropertyStore));
    // if (FAILED(hr)) {
    //     LPVOID errorMsg;
    //     FormatMessageW(
    //         FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
    //         NULL,
    //         hr,
    //         0, // Default language
    //         (LPWSTR)&errorMsg,
    //         0,
    //         NULL
    //     );
    //     std::wcerr << L"Error al obtener el IPropertyStore: " << (LPWSTR)errorMsg << std::endl;
    //     LocalFree(errorMsg);
    //     pShellItem->Release();
    //     wprintf(L"Error al obtener el IPropertyStore: %x\n", hr);
    //     return;
    // }

    // PROPVARIANT propvar;
    // hr = InitPropVariantFromString(thumbnailPath.c_str(), &propvar);
    // if (SUCCEEDED(hr)) {
    //     // Establecer la propiedad PKEY_ThumbnailStream
    //     hr = pPropertyStore->SetValue(PKEY_ThumbnailStream, propvar);
    //     PropVariantClear(&propvar);
    //     if (SUCCEEDED(hr)) {
    //         hr = pPropertyStore->Commit();
    //         if (FAILED(hr)) {
    //             std::wcerr << L"Error al confirmar los cambios: " << std::hex << hr << std::endl;
    //         } else {
    //             std::wcout << L"Miniatura establecida con éxito." << std::endl;
    //         }
    //     } else {
    //         std::wcerr << L"Error al establecer el valor de la propiedad: " << std::hex << hr << std::endl;
    //     }
    // } else {
    //     std::wcerr << L"Error al inicializar PROPVARIANT: " << std::hex << hr << std::endl;
    // }

    // pPropertyStore->Release();
    // pShellItem->Release();
}