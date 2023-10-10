#include "stdafx.h"
#include <SearchAPI.h>
#include <propkey.h>
#include <propvarutil.h>
#include "Utilities.h"

#define MSSEARCH_INDEX L"SystemIndex"
DEFINE_PROPERTYKEY(PKEY_StorageProviderTransferProgress, 0xE77E90DF, 0x6271, 0x4F5B, 0x83, 0x4F, 0x2D, 0xD1, 0xF2, 0x45, 0xDD, 0xA4, 4);

void Utilities::ApplyCustomStateToPlaceholderFile(LPCWSTR path, LPCWSTR filename, winrt::StorageProviderItemProperty &prop)
{
    try
    {
        std::wstring fullPath(path);
        fullPath.append(L"\\");
        fullPath.append(filename);

        wprintf(L"Full path: %s\n", fullPath.c_str());
        winrt::IStorageItem item = winrt::StorageFile::GetFileFromPathAsync(fullPath).get();
        winrt::StorageProviderItemProperties::SetAsync(item, {prop}).get();
    }
    catch (const winrt::hresult_error &error)
    {
        wprintf(L"Failed to set custom state. Error: %s (Code: %08x)\n", error.message().c_str(), error.code());
    }
    catch (...)
    {
        wprintf(L"Failed to set custom state with unknown error %08x\n", static_cast<HRESULT>(winrt::to_hresult()));
    }
}

void Utilities::ApplyCustomOverwriteStateToPlaceholderFile(LPCWSTR path, LPCWSTR filename, winrt::StorageProviderItemProperty &prop)
{
    try
    {
        std::wstring fullPath(path);
        fullPath.append(L"\\");
        fullPath.append(filename);

        wprintf(L"Full path: %s\n", fullPath.c_str());
        winrt::IStorageItem item = winrt::StorageFile::GetFileFromPathAsync(fullPath).get();
        winrt::StorageProviderItemProperties::SetAsync(item, {}).get();
        winrt::StorageProviderItemProperties::SetAsync(item, {prop}).get();
    }
    catch (const winrt::hresult_error &error)
    {
        wprintf(L"Failed to set custom state. Error: %s (Code: %08x)\n", error.message().c_str(), error.code());
    }
    catch (...)
    {
        wprintf(L"Failed to set custom state with unknown error %08x\n", static_cast<HRESULT>(winrt::to_hresult()));
    }
}


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
    printf("ApplyTransferStateToFile\n");
    // Tell the Cloud File API about progress so that toasts can be displayed
 
    HRESULT hr1 = CfReportProviderProgress(
            callbackInfo.ConnectionKey,
            callbackInfo.TransferKey,
            LongLongToLargeInteger(total),
            LongLongToLargeInteger(completed)
        );

    if (FAILED(hr1))
    {
        wprintf(L"Failed to call CfReportProviderProgress with %08x\n", hr1);
        return;
    }
    else
    {
        wprintf(L"Succesfully called CfReportProviderProgress \"%s\" with %llu/%llu\n", fullPath, completed, total);
    }
    wprintf(L"Succesfully called CfReportProviderProgress \"%s\" with %llu/%llu\n", fullPath, completed, total);

    // Tell the Shell so File Explorer can display the progress bar in its view
    try
    {
        // First, get the Volatile property store for the file. That's where the properties are maintained.
        winrt::com_ptr<IShellItem2> shellItem;
        winrt::check_hresult(SHCreateItemFromParsingName(fullPath, nullptr, __uuidof(shellItem), shellItem.put_void()));
        // wprintf(L"transfer-> fullPath \"%s\"\n", fullPath);
        // wprintf(L"transfer-> shellItem \"%s\"\n", shellItem);
        winrt::com_ptr<IPropertyStore> propStoreVolatile;
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
        PROPVARIANT transferProgress;
        UINT64 values[]{completed, total};
        winrt::check_hresult(InitPropVariantFromUInt64Vector(values, ARRAYSIZE(values), &transferProgress)); // TODO: should work, but doesn't the library doesn't have this function implemented
        winrt::check_hresult(propStoreVolatile->SetValue(PKEY_StorageProviderTransferProgress, transferProgress));
        // wprintf(L"transfer-> transferProgress \"%s\"\n", transferProgress);
        // Set the sync transfer status accordingly

        PROPVARIANT transferStatus;
        winrt::check_hresult(
            InitPropVariantFromUInt32(
                (completed < total) ? SYNC_TRANSFER_STATUS::STS_TRANSFERRING : SYNC_TRANSFER_STATUS::STS_NONE,
                &transferStatus));
        winrt::check_hresult(propStoreVolatile->SetValue(PKEY_SyncTransferStatus, transferStatus));
        // wprintf(L"transfer-> transferStatus \"%s\"\n", transferStatus);
        // Without this, all your hard work is wasted.
        winrt::check_hresult(propStoreVolatile->Commit());
        // wprintf(L"transfer-> propStoreVolatile \"%s\"\n", propStoreVolatile);
        // Broadcast a notification that something about the file has changed, so that apps
        // who subscribe (such as File Explorer) can update their UI to reflect the new progress
        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, static_cast<LPCVOID>(fullPath), nullptr);
        wprintf(L"Succesfully Set Transfer Progress on \"%s\" to %llu/%llu\n", fullPath, completed, total);
    }
    catch (...)
    {
        // winrt::to_hresult() will eat the exception if it is a result of winrt::check_hresult,
        // otherwise the exception will get rethrown and this method will crash out as it should
        wprintf(L"Failed to Set Transfer Progress on \"%s\" with %08x\n", fullPath, static_cast<HRESULT>(winrt::to_hresult()));
    }
}