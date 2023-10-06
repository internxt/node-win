#include "stdafx.h"
#include "DirectoryWatcher.h"

const size_t c_bufferSize = 32768; //sizeof(FILE_NOTIFY_INFORMATION) * 100;

bool IsTemporaryFile(const std::wstring& fullPath)
{
    // Comprueba el prefijo ~$ usado por Word, Excel y PowerPoint
    size_t fileNameStart = fullPath.find_last_of(L'\\') + 1;
    if (fullPath.size() >= fileNameStart + 2 && fullPath.compare(fileNameStart, 2, L"~$") == 0)
    {
        return true;
    }

    // Comprueba las extensiones de archivo temporales y las extensiones específicas mencionadas
    std::array<std::wstring, 7> tempExtensions = {
        L".tmp",      // Extension genérica de archivo temporal
        L".laccdb",   // Access
        L".ldb",      // Access versión más antigua
        L".bak",      // AutoCAD backup
        L".sv$",      // AutoCAD autosave
        L".psdtmp",   // Photoshop (en algunos casos)
        L".~tmp"      // Algunos programas generan este tipo de archivos temporales
    };

    for (const auto& ext : tempExtensions)
    {
        if (fullPath.size() >= ext.size() && 
            fullPath.compare(fullPath.size() - ext.size(), ext.size(), ext) == 0)
        {
            return true;
        }
    }
    
    return false;
}

void DirectoryWatcher::Initialize(
    _In_ PCWSTR path,
    _In_ std::function<void(std::list<FileChange>&, napi_env env, InputSyncCallbacksThreadsafe input)> callback,
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
            FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE,
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

        std::list<FileChange> result;
        std::list<std::wstring> addedFiles;
        std::list<std::wstring> removedFiles;
        FILE_NOTIFY_INFORMATION* next = _notify.get();
        while (next != nullptr)
        {
            std::wstring fullPath(_path);
            fullPath.append(L"\\");
            fullPath.append(std::wstring_view(next->FileName, next->FileNameLength / sizeof(wchar_t)));
            
            FileChange fc;
            fc.path = fullPath;
            bool isTmpFile = IsTemporaryFile(fullPath);

            DWORD fileAttributes = GetFileAttributesW(fullPath.c_str());
            
            bool isDirectory = (fileAttributes != INVALID_FILE_ATTRIBUTES) && (fileAttributes & FILE_ATTRIBUTE_DIRECTORY);
            bool fileExists = (fileAttributes != INVALID_FILE_ATTRIBUTES);
            bool isHidden = (fileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
            
            if ( ( next->Action == FILE_ACTION_ADDED || (next->Action == FILE_ACTION_MODIFIED && !fileExists)) && !isTmpFile && !isDirectory && !isHidden ) {
                wprintf(L"new file: %s\n", fullPath.c_str());
                fc.type = NEW_FILE;
                fc.item_added = true;
                result.push_back(fc);
            } else if ( next->Action == FILE_ACTION_ADDED && isDirectory && !isHidden ) {
                wprintf(L"new folder: %s\n", fullPath.c_str());
                fc.type = NEW_FOLDER;
                fc.item_added = true;
                result.push_back(fc);
            } else {
                fc.type = OTHER;
                fc.item_added = false;
                result.push_back(fc);
            }
            
            // else if (next->Action == FILE_ACTION_MODIFIED && fileExists  && !isTmpFile && !isDirectory && !isHidden) {
            //     wprintf(L"modified file1: %s\n", fullPath.c_str());
            //     fc.type = MODIFIED_FILE;
            //     fc.item_added = true;
            //     result.push_back(fc);
            // }
            
            // fc.file_added =( next->Action == FILE_ACTION_ADDED || (next->Action == FILE_ACTION_MODIFIED && !fileExists)) && !isTmpFile && !isDirectory;
            
            wprintf(L"next->FileName: %ls\n", next->FileName);
            wprintf(L"next->Action: %d\n", next->Action);
            wprintf(L"fileExists: %d\n", fileExists);
            wprintf(L"isTmpFile: %d\n", isTmpFile);
            wprintf(L"isDirectory: %d\n", isDirectory);

            if (next->NextEntryOffset)
            {
                next = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<char*>(next) + next->NextEntryOffset);
            }
            else
            {
                next = nullptr;
            }
        }

        // for (auto path : addedFiles) {
            
        //     std::wstring *dataToSend = new std::wstring(path);
        //     napi_status status = napi_call_threadsafe_function(_input.notify_file_added_threadsafe_callback, dataToSend, napi_tsfn_blocking);

        //     CfConvertToPlaceholder(path.c_str());

        //     if (status != napi_ok) {
        //         napi_throw_error(_env, NULL, "Unable to call notify_file_added_threadsafe_callback");
        //     }
        // }
        _callback(result, _env, _input);
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


