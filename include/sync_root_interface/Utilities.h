#pragma once
#include <Windows.h>
class Utilities
{
public:
    static void ApplyTransferStateToFile(const std::wstring &path, _In_ CF_CALLBACK_INFO &callbackInfo, UINT64 total, UINT64 completed);

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
