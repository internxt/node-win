#include "stdafx.h"

#include "ClassFactory.h"
#include "ThumbnailProvider.h"
#include "ShellService.h"
#include <initguid.h>
#include <atlstr.h>

const CLSID CLSID_ThumbnailProvider = {0x3d781652, 0x78c5, 0x4038, {0x87, 0xa4, 0xec, 0x59, 0x40, 0xab, 0x56, 0x0a}};

namespace
{
    template<typename T>
    DWORD make_and_register_class_object()
    {
        DWORD cookie;
        auto factory = winrt::make<ClassFactory<T>>();
        winrt::check_hresult(CoRegisterClassObject(__uuidof(T), factory.get(), CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &cookie));
        return cookie;
    }
}

bool RegisterThumbnailProvider(const GUID& clsid, const TCHAR* extension)
{
    HKEY hKey;
    wchar_t szCLSID[64];
    TCHAR szModule[MAX_PATH];

    StringFromGUID2(clsid, szCLSID, ARRAYSIZE(szCLSID));
    GetModuleFileName(NULL, szModule, ARRAYSIZE(szModule));

    // Clase COM: Registrar la DLL
    if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Classes\\CLSID\\") + CString(szCLSID), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueEx(hKey, _T("InprocServer32"), 0, REG_SZ, (BYTE*)szModule, (DWORD)(_tcslen(szModule) + 1) * sizeof(TCHAR));
        RegSetValueEx(hKey, _T("ThreadingModel"), 0, REG_SZ, (BYTE*)_T("Both"), (DWORD)(_tcslen(_T("Both")) + 1) * sizeof(TCHAR));
        RegCloseKey(hKey);
    }
    else
    {
        return false;
    }

    // Asociar con extensión
    if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Classes\\") + CString(extension) + _T("\\shellex\\{e357fccd-a995-4576-b01f-234630154e96}"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueEx(hKey, NULL, 0, REG_SZ, (BYTE*)szCLSID, (DWORD)(wcslen(szCLSID) + 1) * sizeof(wchar_t));
        RegCloseKey(hKey);
        return true;
    }

    return false;
}


void ShellServices::InitAndStartServiceTask()
{
    auto task = std::thread([]()
    {
        winrt::init_apartment(winrt::apartment_type::single_threaded);

        make_and_register_class_object<ThumbnailProvider>();

        const CLSID CLSID_ThumbnailProvider = __uuidof(ThumbnailProvider); // Obtener CLSID de la clase ThumbnailProvider
        if (RegisterThumbnailProvider(CLSID_ThumbnailProvider, _T(".jpg")))
        {
            _tprintf(_T("Proveedor registrado correctamente.\n"));
        }
        else
        {
            _tprintf(_T("Fallo al registrar proveedor.\n"));
        }

        winrt::handle dummyEvent(CreateEvent(nullptr, FALSE, FALSE, nullptr));
        if (!dummyEvent)
        {
            winrt::throw_last_error();
        }
        DWORD index;
        HANDLE temp = dummyEvent.get();
        CoWaitForMultipleHandles(COWAIT_DISPATCH_CALLS, INFINITE, 1, &temp, &index);
    });
    task.detach();
}
