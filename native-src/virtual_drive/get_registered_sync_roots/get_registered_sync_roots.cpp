#include "stdafx.h"
#include <filesystem>
#include <iostream>
#include <vector>
#include "get_registered_sync_roots.h"

std::vector<SyncRoots> get_registered_sync_roots() {
    std::vector<SyncRoots> syncRootList;

    auto syncRoots = winrt::StorageProviderSyncRootManager::GetCurrentSyncRoots();

    printf("Sync roots count: %d\n", syncRoots.Size());

    for (auto const &info : syncRoots) {
        auto contextBuffer = info.Context();
        std::wstring contextString;

        if (contextBuffer) {
            contextString = winrt::CryptographicBuffer::ConvertBinaryToString(winrt::BinaryStringEncoding::Utf8, contextBuffer).c_str();
        }

        /**
         * v2.5.1 Jonathan Arce
         * Sync root register are now filtered using the characters '->' and '#inxt#' to identify our register.
         * Currently, we only use '#inxt#' in the register, but to support previous versions, we are still
         * including '->' in the filter. In future versions, the filtering by '->' should be removed.
         */
        if (contextString.find(L"#inxt#") != std::wstring::npos || contextString.find(L"->") != std::wstring::npos) {
            SyncRoots sr;
            sr.id = info.Id();
            sr.path = info.Path().Path();
            sr.displayName = info.DisplayNameResource();
            sr.version = info.Version();
            syncRootList.push_back(sr);
        }
    }

  return syncRootList;
}
