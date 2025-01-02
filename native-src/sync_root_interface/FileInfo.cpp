#include "stdafx.h"
#include "FileIcon.h"
#include <gdiplus.h>
#include <windows.h>
#include <shlobj.h>
#include <commctrl.h>  // Para ImageList_GetIcon
#include <gdiplus.h>
#include <wincrypt.h>
#include <vector>
#include <atlbase.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "Crypt32.lib")

bool GetFileIconAsBase64(const std::wstring &filePath, std::string &base64Icon)
{
    // Iniciar GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) != Gdiplus::Ok)
        return false;

    // SHFILEINFO para obtener indices de icono y overlay
    SHFILEINFOW shfi = {};
    // Se pueden combinar las banderas que necesites (por ej. SHGFI_USEFILEATTRIBUTES si es necesario)
    HIMAGELIST hImageList = (HIMAGELIST) SHGetFileInfoW(
        filePath.c_str(),
        FILE_ATTRIBUTE_NORMAL,
        &shfi,
        sizeof(shfi),
        SHGFI_SYSICONINDEX      // Queremos el índice en la System Image List
        | SHGFI_OVERLAYINDEX    // Que incluya el overlayIndex en los bits altos de iIcon
        | SHGFI_SMALLICON       // O SHGFI_LARGEICON, como prefieras
        | SHGFI_ADDOVERLAYS     // Que intente añadir sobreimpresiones
        | SHGFI_ICON            // Devolver icono
    );

    if (!hImageList)
    {
        // Falló la obtención del HIMAGELIST
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    // iIcon = (overlayIndex << 24) | (iconIndex & 0x00FFFFFF)
    int iconIndex    = LOWORD(shfi.iIcon);        // bits bajos
    int overlayIndex = HIBYTE(HIWORD(shfi.iIcon)); // bits altos
    
    // Obtener HICON combinando base + overlay
    // ILD_TRANSPARENT (o ILD_IMAGE) + la macro para overlay
    HICON hIcon = ImageList_GetIcon(hImageList,
                                    iconIndex,
                                    ILD_TRANSPARENT | INDEXTOOVERLAYMASK(overlayIndex));
    if (!hIcon)
    {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    // Convertir HICON a Gdiplus::Bitmap
    Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromHICON(hIcon);
    if (!pBitmap)
    {
        DestroyIcon(hIcon);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    // Guardar como PNG en un IStream en memoria
    IStream *pStream = nullptr;
    if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &pStream)))
    {
        delete pBitmap;
        DestroyIcon(hIcon);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    // Buscar CLSID de PNG
    CLSID pngClsid;
    {
        UINT numEncoders = 0, size = 0;
        Gdiplus::GetImageEncodersSize(&numEncoders, &size);
        if (!numEncoders || !size)
        {
            pStream->Release();
            delete pBitmap;
            DestroyIcon(hIcon);
            Gdiplus::GdiplusShutdown(gdiplusToken);
            return false;
        }

        std::vector<BYTE> buf(size);
        Gdiplus::ImageCodecInfo* pInfo = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buf.data());
        Gdiplus::GetImageEncoders(numEncoders, size, pInfo);
        
        bool found = false;
        for (UINT i = 0; i < numEncoders; i++)
        {
            if (wcscmp(pInfo[i].MimeType, L"image/png") == 0)
            {
                pngClsid = pInfo[i].Clsid;
                found = true;
                break;
            }
        }
        if (!found)
        {
            pStream->Release();
            delete pBitmap;
            DestroyIcon(hIcon);
            Gdiplus::GdiplusShutdown(gdiplusToken);
            return false;
        }
    }

    // Guardar en el stream
    if (pBitmap->Save(pStream, &pngClsid, nullptr) != Gdiplus::Ok)
    {
        pStream->Release();
        delete pBitmap;
        DestroyIcon(hIcon);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    // Rebobinar para leer desde el principio
    LARGE_INTEGER zero = {};
    ULARGE_INTEGER newPos = {};
    if (FAILED(pStream->Seek(zero, STREAM_SEEK_SET, &newPos)))
    {
        pStream->Release();
        delete pBitmap;
        DestroyIcon(hIcon);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    // Obtener tamaño del stream
    STATSTG stat;
    if (pStream->Stat(&stat, STATFLAG_DEFAULT) != S_OK)
    {
        pStream->Release();
        delete pBitmap;
        DestroyIcon(hIcon);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    // Leer los datos en un buffer
    std::vector<BYTE> pngData(stat.cbSize.LowPart);
    ULONG bytesRead = 0;
    if (pStream->Read(pngData.data(), pngData.size(), &bytesRead) != S_OK)
    {
        pStream->Release();
        delete pBitmap;
        DestroyIcon(hIcon);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    // Cerrar stream
    pStream->Release();

    // Convertir a Base64
    DWORD base64Length = 0;
    if (!CryptBinaryToStringA(pngData.data(), bytesRead, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &base64Length))
    {
        delete pBitmap;
        DestroyIcon(hIcon);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    std::vector<char> base64Buffer(base64Length);
    if (!CryptBinaryToStringA(pngData.data(), bytesRead, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, base64Buffer.data(), &base64Length))
    {
        delete pBitmap;
        DestroyIcon(hIcon);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    base64Icon.assign(base64Buffer.data());

    // Liberar recursos
    delete pBitmap;
    DestroyIcon(hIcon);
    Gdiplus::GdiplusShutdown(gdiplusToken);

    return true;
}
