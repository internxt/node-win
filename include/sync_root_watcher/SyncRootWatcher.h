#pragma once

#include "DirectoryWatcher.h"
#include <node_api.h>

class SyncRootWatcher
{
public:
    void WatchAndWait(const wchar_t *syncRootPath, napi_env env, InputSyncCallbacksThreadsafe input);
    static BOOL WINAPI Stop(DWORD reason);

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
    static void WatcherTask(const wchar_t *syncRootPath, napi_env env, InputSyncCallbacksThreadsafe input);
    static void InitDirectoryWatcher(const wchar_t *syncRootPath, napi_env env, InputSyncCallbacksThreadsafe input);
    static void OnSyncRootFileChanges(_In_ std::list<FileChange>& changes, napi_env env, InputSyncCallbacksThreadsafe input);

    static DirectoryWatcher s_directoryWatcher;
    static bool s_shutdownWatcher;
    static winrt::Windows::Storage::Provider::StorageProviderState s_state;
    static winrt::event<winrt::Windows::Foundation::EventHandler<winrt::Windows::Foundation::IInspectable>> s_statusChanged;
};

