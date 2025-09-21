#include "Callbacks.h"
#include "SyncRoot.h"
#include "stdafx.h"
#include <filesystem>
#include "Logger.h"
#include <iostream>
#include <vector>

namespace fs = std::filesystem;
// variable to disconect
CF_CONNECTION_KEY gloablConnectionKey;
std::map<std::wstring, CF_CONNECTION_KEY> connectionMap;

void SyncRoot::HydrateFile(const wchar_t *filePath)
{
    wprintf(L"Hydration file started %ls\n", filePath);
    DWORD attrib = GetFileAttributesW(filePath);
    if (!(attrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        winrt::handle placeholder(CreateFileW(filePath, 0, FILE_READ_DATA, nullptr, OPEN_EXISTING, 0, nullptr));

        LARGE_INTEGER offset;
        offset.QuadPart = 0;
        LARGE_INTEGER length;
        GetFileSizeEx(placeholder.get(), &length);

        if (attrib & FILE_ATTRIBUTE_PINNED)
        {
            // if (!(attrib & FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS))
            // {
            Logger::getInstance().log("Hydration file init", LogLevel::INFO);

            auto start = std::chrono::steady_clock::now();

            HRESULT hr = CfHydratePlaceholder(placeholder.get(), offset, length, CF_HYDRATE_FLAG_NONE, NULL);

            if (FAILED(hr))
            {
                Logger::getInstance().log("Error hydrating file " + Logger::fromWStringToString(filePath), LogLevel::ERROR);
            }
            else
            {
                auto end = std::chrono::steady_clock::now();
                auto elapsedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

                if (elapsedMilliseconds < 200)
                {
                    wprintf(L"Already Hydrated: %d ms\n", elapsedMilliseconds);
                }
                else
                {
                    wprintf(L"Hydration finished %ls\n", filePath);
                }
            }
        }
    }
}

HRESULT SyncRoot::ConnectSyncRoot(const wchar_t *syncRootPath, InputSyncCallbacks syncCallbacks, napi_env env, CF_CONNECTION_KEY *connectionKey)
{
    Utilities::AddFolderToSearchIndexer(syncRootPath);
    register_threadsafe_callbacks(env, syncCallbacks);

    CF_CALLBACK_REGISTRATION callbackTable[] = {
        {CF_CALLBACK_TYPE_FETCH_DATA, fetch_data_callback_wrapper},
        {CF_CALLBACK_TYPE_CANCEL_FETCH_DATA, cancel_fetch_data_callback_wrapper},
        CF_CALLBACK_REGISTRATION_END
    };

    HRESULT hr = CfConnectSyncRoot(
        syncRootPath,
        callbackTable,
        nullptr,
        CF_CONNECT_FLAG_REQUIRE_PROCESS_INFO | CF_CONNECT_FLAG_REQUIRE_FULL_FILE_PATH,
        connectionKey
    );

    wprintf(L"Connection key: %llu\n", connectionKey->Internal);
    
    if (SUCCEEDED(hr)) {
        connectionMap[syncRootPath] = *connectionKey;
    }
    
    return hr;
}

// disconection sync root
HRESULT SyncRoot::DisconnectSyncRoot(const wchar_t *syncRootPath)
{
    auto it = connectionMap.find(syncRootPath);
    if (it != connectionMap.end())
    {
        HRESULT hr = CfDisconnectSyncRoot(it->second);
        if (SUCCEEDED(hr))
        {
            connectionMap.erase(it);
        }
        return hr;
    }
    return E_FAIL;
}
