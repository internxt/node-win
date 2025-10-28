#pragma once
#include <Windows.h>
class Utilities
{
public:
    static void ApplyTransferStateToFile(_In_ LPCWSTR fullPath, _In_ CF_CALLBACK_INFO &callbackInfo, UINT64 total, UINT64 completed);

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

    inline static LARGE_INTEGER JsTimestampToLargeInteger(int64_t jsTimestamp)
    {
        const int64_t EPOCH_DIFFERENCE = 11644473600000LL;
        const int64_t MS_TO_100NS = 10000LL;

        int64_t windowsTime = (jsTimestamp + EPOCH_DIFFERENCE) * MS_TO_100NS;

        LARGE_INTEGER largeInteger;
        largeInteger.LowPart = static_cast<DWORD>(windowsTime & 0xFFFFFFFF);
        largeInteger.HighPart = static_cast<DWORD>((windowsTime >> 32) & 0xFFFFFFFF);

        return largeInteger;
    }

    inline static LARGE_INTEGER LongLongToLargeInteger(_In_ const LONGLONG longlong)
    {
        LARGE_INTEGER largeInteger;
        largeInteger.QuadPart = longlong;
        return largeInteger;
    }
};
