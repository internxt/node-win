#include "stdafx.h"
#include <wincodec.h>
#include "ThumbnailProvider.h"
#include <iostream>
#include <Shlwapi.h>
#include <Shobjidl.h>
#include <windows.h>

IStream* GetImageStream(const std::wstring& imagePath) {
    IStream *pStream = NULL;
    SHCreateStreamOnFileEx(
        imagePath.c_str(),
        STGM_READ | STGM_SHARE_DENY_WRITE,
        0,
        FALSE,
        NULL,
        &pStream
    );
    return pStream;
}

// IInitializeWithItem
IFACEMETHODIMP ThumbnailProvider::Initialize(_In_ IShellItem* item, _In_ DWORD mode)
{
    std::wcout << L"Inicializando ThumbnailProvider..." << std::endl;
    try
    {
        winrt::check_hresult(item->QueryInterface(__uuidof(_itemDest), _itemDest.put_void()));

        // We want to identify the original item in the source folder that we're mirroring,
        // based on the placeholder item that we get initialized with.  There's probably a way
        // to do this based on the file identity blob but this just uses path manipulation.
        winrt::com_array<wchar_t> destPathItem;
        winrt::check_hresult(_itemDest->GetDisplayName(SIGDN_FILESYSPATH, winrt::put_abi(destPathItem)));

        wprintf(L"Thumbnail requested for %s\n", destPathItem.data());

        // Verify the item is underneath the root as we expect.
        if (!PathIsPrefixW(L"C:\\Users\\gcarl\\Desktop\\carpeta", destPathItem.data()))
        {
            wprintf(L"Thumbnail requested for %s, which is not under the root\n", destPathItem.data());
            return E_UNEXPECTED;
        }

        wchar_t relativePath[MAX_PATH];
        winrt::check_bool(PathRelativePathToW(relativePath, L"C:\\Users\\gcarl\\Desktop\\carpeta", FILE_ATTRIBUTE_DIRECTORY, destPathItem.data(), FILE_ATTRIBUTE_NORMAL));

        winrt::com_array<wchar_t> sourcePathItem;
        winrt::check_hresult(PathAllocCombine(L"C:\\Users\\gcarl\\Desktop\\carpeta", relativePath, PATHCCH_ALLOW_LONG_PATHS, winrt::put_abi(sourcePathItem)));

        winrt::check_hresult(SHCreateItemFromParsingName(sourcePathItem.data(), nullptr, __uuidof(_itemSrc), _itemSrc.put_void()));
    }
    catch (...)
    {
        return winrt::to_hresult();
    }

    return S_OK;
}

IFACEMETHODIMP ThumbnailProvider::GetThumbnail(_In_ UINT width, _Out_ HBITMAP* bitmap, _Out_ WTS_ALPHATYPE* alphaType)
{
    *bitmap = nullptr;
    *alphaType = WTSAT_UNKNOWN;

    try
    {
        std::wstring thumbnailPath = L"C:\\Users\\gcarl\\Documents\\maxresdefault.jpg"; // Cambia esto a la ruta de tu thumbnail
        wprintf(L"Thumbnail path ====================================: %s\n", thumbnailPath.c_str());
        MessageBoxW(NULL, L"GetThumbnail llamado!", L"Debug", MB_OK);
        // Obtiene el IRandomAccessStream usando tu función GetImageStream
        auto imageStream = GetImageStream(thumbnailPath);
        if (imageStream == nullptr) {
            return E_FAIL;
        }
        
        // Crea un decodificador de imágenes a partir del stream
        winrt::com_ptr<IWICBitmapDecoder> decoder;
        winrt::com_ptr<IWICImagingFactory> imagingFactory;
        winrt::check_hresult(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(imagingFactory.put())));
        winrt::check_hresult(imagingFactory->CreateDecoderFromStream(imageStream, nullptr, WICDecodeMetadataCacheOnDemand, decoder.put()));

        // Obtiene el primer frame de la imagen
        winrt::com_ptr<IWICBitmapFrameDecode> frame;
        winrt::check_hresult(decoder->GetFrame(0, frame.put()));

        // Convierte el frame en un HBITMAP
        winrt::com_ptr<IWICFormatConverter> converter;
        winrt::check_hresult(imagingFactory->CreateFormatConverter(converter.put()));
        winrt::check_hresult(converter->Initialize(frame.get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom));

        UINT w, h;
        winrt::check_hresult(converter->GetSize(&w, &h));

        BITMAPINFO bminfo = {0};
        bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bminfo.bmiHeader.biWidth = w;
        bminfo.bmiHeader.biHeight = -static_cast<LONG>(h); // Negativo porque es top-down
        bminfo.bmiHeader.biPlanes = 1;
        bminfo.bmiHeader.biBitCount = 32;
        bminfo.bmiHeader.biCompression = BI_RGB;

        void* bits = nullptr;
        *bitmap = CreateDIBSection(nullptr, &bminfo, DIB_RGB_COLORS, &bits, nullptr, 0);

        if (*bitmap && bits) {
            winrt::check_hresult(converter->CopyPixels(nullptr, w * 4, w * h * 4, static_cast<BYTE*>(bits)));
            *alphaType = WTSAT_ARGB;
        } else {
            return E_OUTOFMEMORY;
        }
    }
    catch (...)
    {
        if (*bitmap) {
            DeleteObject(*bitmap);
        }
        return winrt::to_hresult();
    }

    return S_OK;
}