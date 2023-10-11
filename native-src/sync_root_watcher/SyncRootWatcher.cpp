#include "stdafx.h"
#include "SyncRootWatcher.h"
#include "DirectoryWatcher.h"
#include "Callbacks.h"

namespace winrt
{
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Windows::Storage::Provider;
}

#include <Windows.h>
#include <cfapi.h>

DirectoryWatcher SyncRootWatcher::s_directoryWatcher;
bool SyncRootWatcher::s_shutdownWatcher;
winrt::StorageProviderState SyncRootWatcher::s_state;
winrt::event<winrt::EventHandler<winrt::IInspectable>> SyncRootWatcher::s_statusChanged;

void SyncRootWatcher::WatchAndWait(const wchar_t *syncRootPath, napi_env env, InputSyncCallbacksThreadsafe input)
{
    std::thread watcherThread([this, syncRootPath, env, input] { WatcherTask(syncRootPath, env , input); });
    watcherThread.detach();
}

void SyncRootWatcher::WatcherTask(const wchar_t *syncRootPath, napi_env env, InputSyncCallbacksThreadsafe input)
{
    SetConsoleCtrlHandler(Stop, TRUE);
    InitDirectoryWatcher(syncRootPath, env, input);;

    if (syncRootPath == nullptr) {
        wprintf(L"syncRootPath es nulo.\n");
        throw std::invalid_argument("syncRootPath no puede ser nulo");
    }
    while (true)
    {
        try
        {
            auto task = s_directoryWatcher.ReadChangesAsync();

            while (task.Status() == winrt::AsyncStatus::Started)
            {
                Sleep(1000);

                if (s_shutdownWatcher)
                {
                    s_directoryWatcher.Cancel();
                }
            }

            if (s_shutdownWatcher)
            {
                break;
            }
        }
        catch (...)
        {
            wprintf(L"CloudProviderSyncRootWatcher watcher failed. Unknown error.\n");
            throw;
        }
    }
}

void SyncRootWatcher::OnSyncRootFileChanges(_In_ std::list<FileChange>& changes, napi_env env, InputSyncCallbacksThreadsafe input)
{
    auto start = GetTickCount64();
    s_state = winrt::StorageProviderState::Syncing;
    s_statusChanged(nullptr, nullptr);

    for (auto change : changes)
    {
        wprintf(L"Processing change for %s\n", change.path.c_str());

        DWORD attrib = GetFileAttributesW(change.path.c_str());
        if (!(attrib & FILE_ATTRIBUTE_DIRECTORY) && !change.item_added)
        {
            winrt::handle placeholder(CreateFileW(change.path.c_str(), 0, FILE_READ_DATA, nullptr, OPEN_EXISTING, 0, nullptr));

            LARGE_INTEGER offset;
            offset.QuadPart = 0;
            LARGE_INTEGER length;
            GetFileSizeEx(placeholder.get(), &length);
            //length.QuadPart = MAXLONGLONG;

            if (attrib & FILE_ATTRIBUTE_PINNED)
            {
                wprintf(L"Hydrating file %s\n", change.path.c_str());
                CfHydratePlaceholder(placeholder.get(), offset, length, CF_HYDRATE_FLAG_NONE, NULL);
            }
            else if (attrib & FILE_ATTRIBUTE_UNPINNED)
            {
                wprintf(L"Dehydrating file %s\n", change.path.c_str());
                CfDehydratePlaceholder(placeholder.get(), offset, length, CF_DEHYDRATE_FLAG_NONE, NULL);
            }
        }

        if (change.type == NEW_FILE || change.type == NEW_FOLDER )
        {
           register_threadsafe_notify_file_added_callback(change, "file_added", env, input);
        } 
        // else if ( change.type == MODIFIED_FILE) {
        //     wprintf(L"MODIFIED_FILE\n");
        //     wprintf(L"change.path: %s\n", change.path.c_str());
        //     MarkFileAsInSync(change.path);
        // }
     }

    try {

        auto elapsed = GetTickCount64() - start;
        if (elapsed < 3000)
        {
            Sleep(static_cast<DWORD>(3000 - elapsed));
        }

    } catch (...) {
        wprintf(L"Error al dormir el hilo.\n");
        throw;
    }

    s_state = winrt::StorageProviderState::InSync;
    s_statusChanged(nullptr, nullptr);
}

void SyncRootWatcher::InitDirectoryWatcher(const wchar_t *syncRootPath, napi_env env, InputSyncCallbacksThreadsafe input)
{
    try
    {
        s_directoryWatcher.Initialize(syncRootPath, OnSyncRootFileChanges, env , input);
    }
    catch(std::exception& e) {
        wprintf(L"Error initializing directory watcher: %S\n", e.what());
        throw;
    }
    catch (...)
    {
        wprintf(L"Could not init directory watcher.\n");
        throw;
    }
}

BOOL WINAPI
SyncRootWatcher::Stop(DWORD /*dwReason*/)
{
    s_shutdownWatcher = TRUE;
    return TRUE;
}

