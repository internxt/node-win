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
        FILE_NOTIFY_INFORMATION* next = _notify.get();
        while (next != nullptr)
        {

    //         if (next->Action == FILE_ACTION_REMOVED)
    //         {
    //             wprintf(L"File was removed\n");
    //             wprintf(L"_path: %s\n", _path.c_str());
    //             std::wstring fullPath(_path);
    //             fullPath.append(L"\\");
    //             fullPath.append(std::wstring_view(next->FileName, next->FileNameLength / sizeof(wchar_t)));

    //             wprintf(L"fullPath: %s\n", fullPath.c_str());
    // // Abrir el archivo para obtener el handle
    //             HANDLE fileHandle = CreateFileW(
    //                 fullPath.c_str(),
    //                 READ_ATTRIBUTES, // Sólo necesitas READ_ATTRIBUTES
    //                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    //                 nullptr,
    //                 OPEN_EXISTING,
    //                 FILE_ATTRIBUTE_NORMAL,
    //                 nullptr);

    //             wprintf(L"Handle del archivo: %p\n", fileHandle);


    //             if (fileHandle == INVALID_HANDLE_VALUE) {
    //                 DWORD dwError = GetLastError();
    //                 wprintf(L"Error al abrir el archivo: %u\n", dwError);
    //             }
    //             if (fileHandle != INVALID_HANDLE_VALUE) {
    //                 CF_PLACEHOLDER_BASIC_INFO placeholderInfo;
    //                 DWORD returnedLength;

    //                 HRESULT hr = CfGetPlaceholderInfo(
    //                     fileHandle,
    //                     CF_PLACEHOLDER_INFO_BASIC, // O CF_PLACEHOLDER_INFO_STANDARD, según necesites
    //                     &placeholderInfo,
    //                     sizeof(placeholderInfo),
    //                     &returnedLength);

    //                 if (SUCCEEDED(hr)) {
    //                     wprintf(L"ID del placeholder: %llu\n", placeholderInfo.FileId); // Imprimir el ID
    //                 } else {
    //                     // Manejar error, por ejemplo:
    //                     wprintf(L"Error al obtener la información del placeholder: 0x%x\n", hr);
    //                 }

    //                 CloseHandle(fileHandle); // No olvides cerrar el handle
    //             } else {
    //                 // Manejar error al abrir el archivo
    //                 // Continuar con tu lógica actual
    //                 std::wstring fullPath(_path);
    //                 fullPath.append(L"\\");
    //                 fullPath.append(std::wstring_view(next->FileName, next->FileNameLength / sizeof(wchar_t)));
    //                 result.push_back(fullPath);
    //             }

    //         }
            //==============================================================================

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

