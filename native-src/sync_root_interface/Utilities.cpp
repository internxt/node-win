#include "stdafx.h"
#include <SearchAPI.h>
#include <propkey.h>
#include <propvarutil.h>
#include "Utilities.h"
#include <Logger.h>

DEFINE_PROPERTYKEY(PKEY_StorageProviderTransferProgress, 0xE77E90DF, 0x6271, 0x4F5B, 0x83, 0x4F, 0x2D, 0xD1, 0xF2, 0x45, 0xDD, 0xA4, 4);

void Utilities::ApplyTransferStateToFile(_In_ PCWSTR fullPath, _In_ CF_CALLBACK_INFO &callbackInfo, UINT64 total, UINT64 completed)
{
    try
    {
        winrt::check_hresult(CfReportProviderProgress(
            callbackInfo.ConnectionKey,
            callbackInfo.TransferKey,
            LongLongToLargeInteger(total),
            LongLongToLargeInteger(completed)));

        winrt::com_ptr<IShellItem2> shellItem;
        winrt::com_ptr<IPropertyStore> propStoreVolatile;

        winrt::check_hresult(SHCreateItemFromParsingName(fullPath, nullptr, __uuidof(shellItem), shellItem.put_void()));

        winrt::check_hresult(
            shellItem->GetPropertyStore(
                GETPROPERTYSTOREFLAGS::GPS_READWRITE | GETPROPERTYSTOREFLAGS::GPS_VOLATILEPROPERTIESONLY,
                __uuidof(propStoreVolatile),
                propStoreVolatile.put_void()));

        if (completed < total)
        {
            PROPVARIANT pvProgress, pvStatus;
            UINT64 values[]{completed, total};
            InitPropVariantFromUInt64Vector(values, ARRAYSIZE(values), &pvProgress);
            InitPropVariantFromUInt32(SYNC_TRANSFER_STATUS::STS_TRANSFERRING, &pvStatus);

            propStoreVolatile->SetValue(PKEY_StorageProviderTransferProgress, pvProgress);
            propStoreVolatile->SetValue(PKEY_SyncTransferStatus, pvStatus);
            propStoreVolatile->Commit();

            PropVariantClear(&pvProgress);
        }
        else
        {
            PROPVARIANT empty;
            PropVariantInit(&empty);
            propStoreVolatile->SetValue(PKEY_StorageProviderTransferProgress, empty);
            propStoreVolatile->SetValue(PKEY_SyncTransferStatus, empty);
            propStoreVolatile->Commit();

            HANDLE h = CreateFileW(fullPath,
                                   FILE_WRITE_ATTRIBUTES,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   nullptr,
                                   OPEN_EXISTING,
                                   FILE_FLAG_OPEN_REPARSE_POINT,
                                   nullptr);

            if (h != INVALID_HANDLE_VALUE)
            {
                CfSetInSyncState(h, CF_IN_SYNC_STATE_IN_SYNC,
                                 CF_SET_IN_SYNC_FLAG_NONE, nullptr);
                CloseHandle(h);
            }
        }
    }
    catch (...)
    {
        // winrt::to_hresult() will eat the exception if it is a result of winrt::check_hresult,
        // otherwise the exception will get rethrown and this method will crash out as it should
        wprintf(L"Failed to Set Transfer Progress on \"%s\" with %08x\n", fullPath, static_cast<HRESULT>(winrt::to_hresult()));
    }
}
