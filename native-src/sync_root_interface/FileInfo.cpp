// FileIcon.cpp
#include "stdafx.h"
#include "FileIcon.h"
#include <gdiplus.h>
#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <vector>
#include <wincrypt.h>
#include <atlbase.h>

#pragma comment (lib, "gdiplus.lib")
#pragma comment (lib, "Crypt32.lib")

bool GetFileIconAsBase64(const std::wstring& filePath, std::string& base64Icon)
{
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::Status gdiplusStatus = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    if (gdiplusStatus != Gdiplus::Ok) {
        wprintf(L"GdiplusStartup failed: %d\n", gdiplusStatus);
        return false;
    }

    SHFILEINFOW sfi = { 0 };
    wprintf(L"File path_: %s\n", filePath.c_str());
    if (!SHGetFileInfoW(filePath.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_ADDOVERLAYS | SHGFI_SMALLICON))
    {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        wprintf(L"SHGetFileInfo failed: %d\n", GetLastError());
        return false;
    }

    HICON hIcon = sfi.hIcon;
    if (!hIcon)
    {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        wprintf(L"hicon failed");
        return false;
    }

    Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromHICON(hIcon);
    if (!bitmap)
    {
        DestroyIcon(hIcon);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        wprintf(L"bitmap failed");
        return false;
    }

    IStream* stream = NULL;
    HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    if (FAILED(hr))
    {
        delete bitmap;
        DestroyIcon(hIcon);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    CLSID pngClsid;
    UINT numEncoders = 0;
    UINT size = 0;
    Gdiplus::GetImageEncodersSize(&numEncoders, &size);
    if (size == 0)
    {
        stream->Release();
        delete bitmap;
        DestroyIcon(hIcon);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    std::vector<BYTE> bufferEncoders(size);
    Gdiplus::ImageCodecInfo* pImageCodecInfo = reinterpret_cast<Gdiplus::ImageCodecInfo*>(bufferEncoders.data());
    Gdiplus::GetImageEncoders(numEncoders, size, pImageCodecInfo);

    bool found = false;
    for (UINT i = 0; i < numEncoders; ++i)
    {
        if (wcscmp(pImageCodecInfo[i].MimeType, L"image/png") == 0)
        {
            pngClsid = pImageCodecInfo[i].Clsid;
            found = true;
            break;
        }
    }

    if (!found)
    {
        stream->Release();
        delete bitmap;
        DestroyIcon(hIcon);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    if (bitmap->Save(stream, &pngClsid, NULL) != Gdiplus::Ok)
    {
        stream->Release();
        delete bitmap;
        DestroyIcon(hIcon);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    STATSTG stat;
    if (stream->Stat(&stat, STATFLAG_DEFAULT) != S_OK)
    {
        stream->Release();
        delete bitmap;
        DestroyIcon(hIcon);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    std::vector<BYTE> imageData(stat.cbSize.LowPart);
    ULONG bytesRead = 0;
    if (stream->Read(imageData.data(), imageData.size(), &bytesRead) != S_OK)
    {
        stream->Release();
        delete bitmap;
        DestroyIcon(hIcon);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    DWORD base64Length = 0;
    if (!CryptBinaryToStringA(imageData.data(), bytesRead, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &base64Length))
    {
        stream->Release();
        delete bitmap;
        DestroyIcon(hIcon);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    std::vector<char> base64Buffer(base64Length);
    if (!CryptBinaryToStringA(imageData.data(), bytesRead, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, base64Buffer.data(), &base64Length))
    {
        stream->Release();
        delete bitmap;
        DestroyIcon(hIcon);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }

    base64Icon.assign(base64Buffer.data());

    stream->Release();
    delete bitmap;
    DestroyIcon(hIcon);
    Gdiplus::GdiplusShutdown(gdiplusToken);

    return true;
}
