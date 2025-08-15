#include "stdafx.h"
#include <SearchAPI.h>
#include <propkey.h>
#include <propvarutil.h>
#include "Utilities.h"
#include <Logger.h>

#define MSSEARCH_INDEX L"SystemIndex"
DEFINE_PROPERTYKEY(PKEY_StorageProviderTransferProgress, 0xE77E90DF, 0x6271, 0x4F5B, 0x83, 0x4F, 0x2D, 0xD1, 0xF2, 0x45, 0xDD, 0xA4, 4);

void Utilities::AddFolderToSearchIndexer(_In_ PCWSTR folder)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        wprintf(L"Failed to initialize COM library. Error code = %08x\n", hr);
        return;
    }

    std::wstring url(L"file:///");
    url.append(folder);

    try
    {
        winrt::com_ptr<ISearchManager> searchManager;
        winrt::check_hresult(CoCreateInstance(__uuidof(CSearchManager), NULL, CLSCTX_SERVER, __uuidof(&searchManager), searchManager.put_void()));

        winrt::com_ptr<ISearchCatalogManager> searchCatalogManager;
        winrt::check_hresult(searchManager->GetCatalog(MSSEARCH_INDEX, searchCatalogManager.put()));

        winrt::com_ptr<ISearchCrawlScopeManager> searchCrawlScopeManager;
        winrt::check_hresult(searchCatalogManager->GetCrawlScopeManager(searchCrawlScopeManager.put()));

        winrt::check_hresult(searchCrawlScopeManager->AddDefaultScopeRule(url.data(), TRUE, FOLLOW_FLAGS::FF_INDEXCOMPLEXURLS));
        winrt::check_hresult(searchCrawlScopeManager->SaveAll());

        // wprintf(L"Succesfully called AddFolderToSearchIndexer on \"%s\"\n", url.data());
    }
    catch (...)
    {
        wprintf(L"Failed on call to AddFolderToSearchIndexer for \"%s\" with %08x\n", url.data(), static_cast<HRESULT>(winrt::to_hresult()));
    }
}

void Utilities::ApplyTransferStateToFile(_In_ PCWSTR fullPath, _In_ CF_CALLBACK_INFO &callbackInfo, UINT64 total, UINT64 completed)
{
    Logger::getInstance().log("ApplyTransferStateToFile", LogLevel::INFO);
    // Tell the Cloud File API about progress so that toasts can be displayed

    HRESULT hr1 = CfReportProviderProgress(
        callbackInfo.ConnectionKey,
        callbackInfo.TransferKey,
        LongLongToLargeInteger(total),
        LongLongToLargeInteger(completed));

    if (FAILED(hr1))
    {
        wprintf(L"Failed to call CfReportProviderProgress with %08x\n", hr1);
        return;
    }
    else
    {
        wprintf(L"Succesfully called CfReportProviderProgress \"%s\" with %llu/%llu\n", fullPath, completed, total);
    }
    // wprintf(L"Succesfully called CfReportProviderProgress \"%s\" with %llu/%llu\n", fullPath, completed, total);

    // Tell the Shell so File Explorer can display the progress bar in its view
    try
    {
        // First, get the Volatile property store for the file. That's where the properties are maintained.
        winrt::com_ptr<IShellItem2> shellItem;
        winrt::com_ptr<IPropertyStore>propStoreVolatile;

        winrt::check_hresult(SHCreateItemFromParsingName(fullPath, nullptr, __uuidof(shellItem), shellItem.put_void()));

        // wprintf(L"transfer-> propStoreVolatile \"%s\"\n", propStoreVolatile);
        winrt::check_hresult(
            shellItem->GetPropertyStore(
                GETPROPERTYSTOREFLAGS::GPS_READWRITE | GETPROPERTYSTOREFLAGS::GPS_VOLATILEPROPERTIESONLY,
                __uuidof(propStoreVolatile),
                propStoreVolatile.put_void()));
        // wprintf(L"transfer-> shellItem \"%s\"\n", shellItem);
        // The PKEY_StorageProviderTransferProgress property works with a UINT64 array that is two elements, with
        // element 0 being the amount of data transferred, and element 1 being the total amount
        // that will be transferred.
        if (completed < total)
        {
            PROPVARIANT pvProgress, pvStatus;
            UINT64 values[] { completed, total };
            InitPropVariantFromUInt64Vector(values, ARRAYSIZE(values), &pvProgress);
            InitPropVariantFromUInt32(SYNC_TRANSFER_STATUS::STS_TRANSFERRING, &pvStatus);

            propStoreVolatile->SetValue(PKEY_StorageProviderTransferProgress, pvProgress);
            propStoreVolatile->SetValue(PKEY_SyncTransferStatus, pvStatus);
            propStoreVolatile->Commit();

            PropVariantClear(&pvProgress);
        }
        else
        {
            PROPVARIANT empty; PropVariantInit(&empty);
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
            if (h != INVALID_HANDLE_VALUE) {
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

std::wstring Utilities::GetErrorMessageCloudFiles(HRESULT hr) {
    LPWSTR errorMsg = nullptr;
    DWORD size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        hr,
        0,
        (LPWSTR)&errorMsg,
        0,
        nullptr
    );

    std::wstring message;
    if (size > 0 && errorMsg != nullptr) {
        message.assign(errorMsg, size);
    }
    LocalFree(errorMsg);
    return message;
}
