// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserve
// #include "index.h"
#include "stdafx.h"
#include <SearchAPI.h>    // needed for AddFolderToSearchIndexer
#include <propkey.h>      // needed for ApplyTransferStateToFile
#include <propvarutil.h>  // needed for ApplyTransferStateToFile
#include "Utilities.h"

#define MSSEARCH_INDEX L"SystemIndex"
DEFINE_PROPERTYKEY(PKEY_StorageProviderTransferProgress, 0xE77E90DF, 0x6271, 0x4F5B, 0x83, 0x4F, 0x2D, 0xD1, 0xF2, 0x45, 0xDD, 0xA4, 4);

void Utilities::ApplyCustomStateToPlaceholderFile(LPCWSTR path, LPCWSTR filename, winrt::StorageProviderItemProperty& prop)
{
    try
    {
        std::wstring fullPath(path);
        fullPath.append(L"\\");
        fullPath.append(filename);

        wprintf(L"Full path: %s\n", fullPath.c_str());
        winrt::IStorageItem item = winrt::StorageFile::GetFileFromPathAsync(fullPath).get();
        winrt::StorageProviderItemProperties::SetAsync(item, { prop }).get();
    }
    catch (const winrt::hresult_error& error)
    {
        wprintf(L"Failed to set custom state. Error: %s (Code: %08x)\n", error.message().c_str(), error.code());
    }
    catch (...)
    {
        wprintf(L"Failed to set custom state with unknown error %08x\n", static_cast<HRESULT>(winrt::to_hresult()));
    }
}
