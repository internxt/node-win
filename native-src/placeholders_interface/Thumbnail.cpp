#include "Thumbnail.h"
#include <iostream>

bool FileExists(LPCWSTR fileName) {
    DWORD fileAttr = GetFileAttributesW(fileName);
    return (fileAttr != INVALID_FILE_ATTRIBUTES &&
            !(fileAttr & FILE_ATTRIBUTE_DIRECTORY));
}

void SetThumbnail(LPCWSTR filePath, LPCWSTR thumbnailPath) {
    wprintf(L"Setting thumbnail for %s\n", filePath);
    LPCWSTR filePath_ = L"C:\\Users\\gcarl\\Desktop\\carpeta\\file1.txt";
    if (!FileExists(filePath_)) {
        std::wcout << L"El archivo " << filePath_ << L" no existe." << std::endl;
        return;
    }

    Microsoft::WRL::ComPtr<IPropertyStore> propertyStore;
    
    HRESULT hr = SHGetPropertyStoreFromParsingName(filePath_, nullptr, GPS_READWRITE, IID_PPV_ARGS(&propertyStore));
    if (SUCCEEDED(hr)) {
        PROPVARIANT propVariant;
        hr = InitPropVariantFromBoolean(TRUE, &propVariant);
        if (SUCCEEDED(hr)) {
            hr = propertyStore->SetValue(PKEY_ThumbnailStream, propVariant);
            PropVariantClear(&propVariant);
        } else {
            wprintf(L"Failed to initialize PROPVARIANT: %d\n", hr);
        }
    } else {
        wprintf(L"Failed to get property store from parsing name: %d\n", hr);
    }
}