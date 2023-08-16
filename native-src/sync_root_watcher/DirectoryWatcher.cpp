#include "stdafx.h"
#include "DirectoryWatcher.h"

const size_t c_bufferSize = sizeof(FILE_NOTIFY_INFORMATION) * 100;

void DirectoryWatcher::Initialize(
    _In_ PCWSTR path,
    _In_ std::function<void(std::list<std::wstring>&)> callback)
{
    _path = path;
    _notify.reset(reinterpret_cast<FILE_NOTIFY_INFORMATION*>(new char[c_bufferSize]));

    _callback = callback;

    _dir.attach(CreateFileW(path,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr));
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
            FILE_NOTIFY_CHANGE_ATTRIBUTES,
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
        FILE_NOTIFY_INFORMATION* next = _notify.get();
        while (next != nullptr)
        {
            std::wstring fullPath(_path);
            fullPath.append(L"\\");
            fullPath.append(std::wstring_view(next->FileName, next->FileNameLength / sizeof(wchar_t)));
            result.push_back(fullPath);

            if (next->NextEntryOffset)
            {
                next = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<char*>(next) + next->NextEntryOffset);
            }
            else
            {
                next = nullptr;
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

