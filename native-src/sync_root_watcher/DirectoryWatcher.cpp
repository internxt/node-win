#include "stdafx.h"
#include "DirectoryWatcher.h"

const size_t c_bufferSize = sizeof(FILE_NOTIFY_INFORMATION) * 100;

void DirectoryWatcher::Initialize(
    _In_ PCWSTR path,
    _In_ std::function<void(std::list<std::wstring>&)> callback,
    napi_env env,
    InputSyncCallbacksThreadsafe input)
{
    _path = path;
    _notify.reset(reinterpret_cast<FILE_NOTIFY_INFORMATION*>(new char[c_bufferSize]));

    _callback = callback;

    _env = env;

    _input = input;

    _dir.attach(
        CreateFileW(path,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr)
    );
    if (_dir.get() == INVALID_HANDLE_VALUE)
    {
        throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()));
    }
}



winrt::Windows::Foundation::IAsyncAction DirectoryWatcher::ReadChangesAsync()
{
    _readTask = ReadChangesInternalAsync();
    return _readTask;
}

winrt::Windows::Foundation::IAsyncAction DirectoryWatcher::ReadChangesInternalAsync()
{
    co_await winrt::resume_background();

    while (true)
    {
        DWORD returned;
        winrt::check_bool(ReadDirectoryChangesW(
            _dir.get(),
            _notify.get(),
            c_bufferSize,
            TRUE,
            FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_FILE_NAME,
            &returned,
            &_overlapped,
            nullptr));

        DWORD transferred;
        if (!GetOverlappedResult(_dir.get(), &_overlapped, &transferred, TRUE))
        {
            DWORD error = GetLastError();
            if (error != ERROR_OPERATION_ABORTED)
            {
                throw winrt::hresult_error(HRESULT_FROM_WIN32(error));
            }
            break;
        }

        std::list<std::wstring> result;
        std::list<std::wstring> addedFiles;
        std::list<std::wstring> removedFiles;
        FILE_NOTIFY_INFORMATION* next = _notify.get();
        while (next != nullptr)
        {
            std::wstring fullPath(_path);
            fullPath.append(L"\\");
            fullPath.append(std::wstring_view(next->FileName, next->FileNameLength / sizeof(wchar_t)));
            result.push_back(fullPath);

            if (next->Action == FILE_ACTION_ADDED)
            {
                addedFiles.push_back(fullPath);
            }
            else if (next->Action == FILE_ACTION_REMOVED)
            {
                removedFiles.push_back(fullPath);
            }

            if (next->NextEntryOffset)
            {
                next = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<char*>(next) + next->NextEntryOffset);
            }
            else
            {
                next = nullptr;
            }
        }
        // print removed files
        for (auto path : removedFiles)
        {
            wprintf(L"Removed: %s\n", path.c_str());
        }
        // print added files
        for (auto path : addedFiles) {
            wprintf(L"Added: %s\n", path.c_str());
            
            // Convertir el wstring a un puntero y pasarlo como argumento al threadsafe function
            std::wstring *dataToSend = new std::wstring(path);
            napi_status status = napi_call_threadsafe_function(_input.notify_file_added_threadsafe_callback, dataToSend, napi_tsfn_blocking);

            if (status != napi_ok) {
                napi_throw_error(_env, NULL, "Unable to call notify_file_added_threadsafe_callback");
            }
        }
        _callback(result);
    }

    wprintf(L"watcher exiting\n");
}

void DirectoryWatcher::Cancel()
{
    wprintf(L"Canceling watcher\n");
    while (_readTask && (_readTask.Status() == winrt::AsyncStatus::Started) &&
        !CancelIoEx(_dir.get(), &_overlapped))
    {
        // Raced against the thread loop. Try again.
        Sleep(10);
    }
}

