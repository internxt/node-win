#include "Thumbnail.h"
#include <iostream>
#include <stdafx.h>

bool FileExists(LPCWSTR fileName) {
    DWORD fileAttr = GetFileAttributesW(fileName);
    return (fileAttr != INVALID_FILE_ATTRIBUTES &&
            !(fileAttr & FILE_ATTRIBUTE_DIRECTORY));
}

winrt::Windows::Storage::Streams::IRandomAccessStream GetImageStream(const std::wstring& imagePath) {
    try {
        winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::StorageFile> getFileOperation = 
            winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(imagePath);

        winrt::Windows::Storage::StorageFile storageFile = getFileOperation.get();

        winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::Streams::IRandomAccessStream> getStreamOperation = 
            storageFile.OpenAsync(winrt::Windows::Storage::FileAccessMode::Read);

        winrt::Windows::Storage::Streams::IRandomAccessStream stream = getStreamOperation.get();
        return stream;

    } catch (winrt::hresult_error const& ex) {
        std::wcerr << L"Se produjo un error: " << ex.message().c_str() << std::endl;
        throw;
    }
}

void SetThumbnailBase(winrt::Windows::Storage::StorageFile const& file) {
    try {
        winrt::Windows::Storage::FileProperties::StorageItemContentProperties properties = file.Properties();
        winrt::Windows::Storage::Streams::IRandomAccessStream imageStream = GetImageStream();

        // Usa el stream de la imagen como thumbnail
        winrt::Windows::Storage::FileProperties::ThumbnailMode thumbnailMode = winrt::Windows::Storage::FileProperties::ThumbnailMode::SingleItem;
        winrt::Windows::Foundation::IAsyncAction thumbnailSetAction = properties.SetThumbnailAsync(thumbnailMode, imageStream);

        thumbnailSetAction.get();  // Espera a que la operación asíncrona se complete
    } catch (...) {
        // Manejo de errores
        std::wcout << L"Se produjo un error al establecer el thumbnail." << std::endl;
    }
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