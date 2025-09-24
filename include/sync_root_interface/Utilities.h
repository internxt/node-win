#pragma once
#include <Windows.h>
class Utilities
{
public:
    static void ApplyTransferStateToFile(_In_ LPCWSTR fullPath, _In_ CF_CALLBACK_INFO &callbackInfo, UINT64 total, UINT64 completed);
    static std::wstring GetErrorMessageCloudFiles(HRESULT hr);

    static winrt::com_array<wchar_t>
    ConvertSidToStringSid(_In_ PSID sid)
    {
        winrt::com_array<wchar_t> string;
        if (::ConvertSidToStringSidW(sid, winrt::put_abi(string)))
        {
            return string;
        }
        else
        {
            throw std::bad_alloc();
        }
    };

    inline static LARGE_INTEGER FileTimeToLargeInteger(_In_ const FILETIME fileTime)
    {
        LARGE_INTEGER largeInteger;

        largeInteger.LowPart = fileTime.dwLowDateTime;
        largeInteger.HighPart = fileTime.dwHighDateTime;

        return largeInteger;
    }

    inline static LARGE_INTEGER LongLongToLargeInteger(_In_ const LONGLONG longlong)
    {
        LARGE_INTEGER largeInteger;
        largeInteger.QuadPart = longlong;
        return largeInteger;
    }

    static DWORD convertSizeToDWORD(size_t &convertVar)
    {
        if (convertVar > UINT_MAX)
        {
            convertVar = UINT_MAX;
        }
        return static_cast<DWORD>(convertVar);
    }

    static DWORD sizeToDWORD(size_t size)
    {
        return convertSizeToDWORD(size);
    }
};
