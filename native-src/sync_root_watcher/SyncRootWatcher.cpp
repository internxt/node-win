#include "stdafx.h"
#include "SyncRootWatcher.h"
#include "DownloadMutexManager.h"
#include "DirectoryWatcher.h"
#include "Callbacks.h"
#include <windows.h>
#include <Logger.h>
#include <PlaceHolders.h>
#include <filesystem>

namespace fs = std::filesystem; 
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
    std::thread watcherThread([this, syncRootPath, env, input]
                              { WatcherTask(syncRootPath, env, input); });
    watcherThread.detach();
}

void SyncRootWatcher::WatcherTask(const wchar_t *syncRootPath, napi_env env, InputSyncCallbacksThreadsafe input)
{
    SetConsoleCtrlHandler(Stop, TRUE);
    InitDirectoryWatcher(syncRootPath, env, input);

    if (syncRootPath == nullptr)
    {
        Logger::getInstance().log("syncRootPath null.", LogLevel::ERROR);
        throw std::invalid_argument("syncRootPath must be a valid path.");
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
            Logger::getInstance().log("CloudProviderSyncRootWatcher watcher failed. Unknown error.", LogLevel::ERROR);
            throw;
        }
    }
}

void SyncRootWatcher::OnSyncRootFileChanges(_In_ std::list<FileChange> &changes, napi_env env, InputSyncCallbacksThreadsafe input)
{
    auto start = GetTickCount64();
    s_state = winrt::StorageProviderState::Syncing;
    s_statusChanged(nullptr, nullptr);

    for (auto change : changes)
    {
        // TODO: it should be just one callback for all the errors -> need refactor
        if (
            change.type == ERROR_FILE_SIZE_EXCEEDED ||
            change.type == ERROR_FOLDER_SIZE_EXCEEDED ||
            change.type == ERROR_FILE_ZERO_SIZE ||
            change.type == ERROR_FILE_NON_EXTENSION)
        {
            change.message = change.path.c_str();
            register_threadsafe_message_callback(change, "message", env, input);
        }
        else
        {
            // FileState fileState = DirectoryWatcher::getPlaceholderInfo(change.path);
            DWORD attrib = GetFileAttributesW(change.path.c_str());
            if (!(attrib & FILE_ATTRIBUTE_DIRECTORY) && !change.item_added)
            {
                winrt::handle placeholder(CreateFileW(change.path.c_str(), 0, FILE_READ_DATA, nullptr, OPEN_EXISTING, 0, nullptr));

                LARGE_INTEGER offset;
                offset.QuadPart = 0;
                LARGE_INTEGER length;
                GetFileSizeEx(placeholder.get(), &length);
                // length.QuadPart = MAXLONGLONG;
                // bool isHydrated = fileState.pinstate == PinState::AlwaysLocal && fileState.syncstate == SyncState::InSync;
                if (attrib & FILE_ATTRIBUTE_PINNED) // && !(isHydrated)
                {
                    Logger::getInstance().log("Hydration file ", LogLevel::INFO);

                    auto start = std::chrono::steady_clock::now();

                    HRESULT hr = CfHydratePlaceholder(placeholder.get(), offset, length, CF_HYDRATE_FLAG_NONE, NULL);

                    if (FAILED(hr))
                    {
                        Logger::getInstance().log("Error hydrating file " + Logger::fromWStringToString(change.path), LogLevel::ERROR);
                    }

                    auto end = std::chrono::steady_clock::now();
                    auto elapsedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

                    if (elapsedMilliseconds < 200)
                    {
                        Logger::getInstance().log("Already Hydrated: " + std::to_string(elapsedMilliseconds) + " ms", LogLevel::WARN);
                    }
                    else
                    {
                        Logger::getInstance().log("Hydration finished " + Logger::fromWStringToString(change.path), LogLevel::INFO);
                        Logger::getInstance()
                            .log("Mutex waiting for " + Logger::fromWStringToString(change.path), LogLevel::INFO);
                        DownloadMutexManager &mutexManager = DownloadMutexManager::getInstance();
                        mutexManager.waitReady();
                        Logger::getInstance().log("Mutex ready for " + Logger::fromWStringToString(change.path), LogLevel::INFO);
                        mutexManager.resetReady();
                        Logger::getInstance().log("resetReady for " + Logger::fromWStringToString(change.path), LogLevel::INFO);
                    }
                    // Sleep(250);
                    // std::wstring folder = change.path.substr(0, change.path.find_last_of(L"\\"));
                    // Logger::getInstance().log("Marking folder as in sync" + Logger::fromWStringToString(folder), LogLevel::INFO);
                    // Placeholders::UpdateSyncStatus(folder, true, true);
                }
                else if (attrib & FILE_ATTRIBUTE_UNPINNED)
                {
                    Logger::getInstance().log("Dehydrating file" + Logger::fromWStringToString(change.path), LogLevel::INFO);
                    HRESULT hr = CfDehydratePlaceholder(placeholder.get(), offset, length, CF_DEHYDRATE_FLAG_NONE, NULL);
                    
                    if (FAILED(hr))
                    {
                        Logger::getInstance().log("Error dehydrating file " + Logger::fromWStringToString(change.path), LogLevel::ERROR);
                    }
                    else
                    {
                        Logger::getInstance().log("Dehydration finished " + Logger::fromWStringToString(change.path), LogLevel::INFO);
                        Placeholders::UpdateSyncStatus(change.path, true, fs::is_directory(change.path));
                        Placeholders::UpdatePinState(change.path, PinState::OnlineOnly);
                    }
                }
            }

            if (change.type == NEW_FILE || change.type == NEW_FOLDER)
            {
                Sleep(250);
                register_threadsafe_notify_file_added_callback(change, "file_added", env, input);
            }
            // else if ( change.type == MODIFIED_FILE) {
            //     wprintf(L"MODIFIED_FILE\n");
            //     wprintf(L"change.path: %s\n", change.path.c_str());
            //     MarkFileAsInSync(change.path);
            // }
        }
    }

    try
    {

        auto elapsed = GetTickCount64() - start;
        if (elapsed < 3000)
        {
            Sleep(static_cast<DWORD>(3000 - elapsed));
        }
    }
    catch (...)
    {
        Logger::getInstance().log("Error sleeping the thread.", LogLevel::ERROR);
        throw;
    }

    s_state = winrt::StorageProviderState::InSync;
    s_statusChanged(nullptr, nullptr);
}

void SyncRootWatcher::InitDirectoryWatcher(const wchar_t *syncRootPath, napi_env env, InputSyncCallbacksThreadsafe input)
{
    try
    {
        s_directoryWatcher.Initialize(syncRootPath, OnSyncRootFileChanges, env, input);
    }
    catch (std::exception &e)
    {
        Logger::getInstance().log("Error initializing directory watcher: " + std::string(e.what()), LogLevel::ERROR);
        throw;
    }
    catch (...)
    {
        Logger::getInstance().log("Could not init directory watcher.", LogLevel::ERROR);
        throw;
    }
}

BOOL WINAPI
SyncRootWatcher::Stop(DWORD /*dwReason*/)
{
    s_shutdownWatcher = TRUE;
    return TRUE;
}
