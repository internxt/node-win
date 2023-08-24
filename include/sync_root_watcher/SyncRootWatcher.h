#pragma once

#include "DirectoryWatcher.h"

class SyncRootWatcher
{
public:
    static void WatchAndWait(const wchar_t *syncRootPath);
    static BOOL WINAPI Stop(DWORD reason);

    // These methods allow the StatusUISource to report on sync status.
    static auto StatusChanged(winrt::Windows::Foundation::EventHandler<winrt::Windows::Foundation::IInspectable> const& handler)
    {
        return s_statusChanged.add(handler);
    }

    static auto StatusChanged(winrt::event_token const& token) noexcept
    {
        return s_statusChanged.remove(token);
    }

    static auto State() { return s_state; }

private:
    static void InitDirectoryWatcher(const wchar_t *syncRootPath);
    static void OnSyncRootFileChanges(_In_ std::list<std::wstring>& changes);

    static DirectoryWatcher s_directoryWatcher;
    static bool s_shutdownWatcher;
    static winrt::Windows::Storage::Provider::StorageProviderState s_state;
    static winrt::event<winrt::Windows::Foundation::EventHandler<winrt::Windows::Foundation::IInspectable>> s_statusChanged;
};

