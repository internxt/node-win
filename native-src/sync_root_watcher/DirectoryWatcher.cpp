#include "stdafx.h"
#include "DirectoryWatcher.h"
#include <iostream>
#include <filesystem>
#include "Logger.h"

const size_t c_bufferSize = 32768; // sizeof(FILE_NOTIFY_INFORMATION) * 100;

namespace fs = std::filesystem;

bool IsTemporaryFile(const std::wstring &fullPath)
{
    // Comprueba el prefijo ~$ usado por Word, Excel y PowerPoint
    size_t fileNameStart = fullPath.find_last_of(L'\\') + 1;
    if (fullPath.size() >= fileNameStart + 2 && fullPath.compare(fileNameStart, 2, L"~$") == 0)
    {
        return true;
    }

    // Comprueba las extensiones de archivo temporales y las extensiones específicas mencionadas
    std::array<std::wstring, 7> tempExtensions = {
        L".tmp",    // Extension genérica de archivo temporal
        L".laccdb", // Access
        L".ldb",    // Access versión más antigua
        L".bak",    // AutoCAD backup
        L".sv$",    // AutoCAD autosave
        L".psdtmp", // Photoshop (en algunos casos)
        L".~tmp"    // Algunos programas generan este tipo de archivos temporales
    };

    for (const auto &ext : tempExtensions)
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
    _In_ std::function<void(std::list<FileChange> &, napi_env env, InputSyncCallbacksThreadsafe input)> callback,
    napi_env env,
    InputSyncCallbacksThreadsafe input)
{
    _path = path;
    _notify.reset(reinterpret_cast<FILE_NOTIFY_INFORMATION *>(new char[c_bufferSize]));

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

std::uintmax_t getDirectorySize(const fs::path &directoryPath)
{
    std::uintmax_t size = 0;

    for (const auto &entry : fs::recursive_directory_iterator(directoryPath))
    {
        if (fs::is_regular_file(entry))
        {
            size += fs::file_size(entry.path());
        }
    }

    return size;
}

void ExploreDirectory(const std::wstring &directoryPath, std::list<FileChange> &result, FileChange fc)
{
    // FileChange fc;
    Logger::Write(L"[Log] New Folder: %s\n" + directoryPath.c_str(), LogLevel::INFO);
    fc.path = directoryPath;
    fc.type = NEW_FOLDER;
    fc.item_added = true;
    result.push_back(fc);

    // std::wstring searchPath = directoryPath + L"\\*";
    // WIN32_FIND_DATAW data;
    // HANDLE hFind = FindFirstFileW(searchPath.c_str(), &data);

    // if (hFind != INVALID_HANDLE_VALUE)
    // {
    //     do
    //     {
    //         if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    //         {
    //             if (wcscmp(data.cFileName, L".") != 0 && wcscmp(data.cFileName, L"..") != 0)
    //             {
    //                 // Es un directorio, llama a la función recursivamente
    //                 std::wstring fullPath2 = directoryPath + L"\\" + data.cFileName;
    //                 ExploreDirectory(fullPath2, result, fc);
    //             }
    //         }
    //         else
    //         {
    //             wprintf(L"[Debug] New file recursivo: %s\n", data.cFileName);
    //             std::wstring fullPath2 = directoryPath + L"\\" + data.cFileName;
    //             FileChange fc;
    //             fc.path = fullPath2;
    //             DWORD fileAttributes = GetFileAttributesW(fullPath2.c_str());
    //             bool isUnPinned = (fileAttributes != INVALID_FILE_ATTRIBUTES) && (fileAttributes & FILE_ATTRIBUTE_UNPINNED);
    //             bool isPinned = (fileAttributes != INVALID_FILE_ATTRIBUTES) && (fileAttributes & FILE_ATTRIBUTE_PINNED);
    //             if (isUnPinned)
    //             {
    //                 wprintf(L"[Debug] UNPINNED new file recursivo:\n");
    //                 fc.type = NEW_FILE;
    //                 fc.item_added = true;
    //                 result.push_back(fc);
    //             }
    //             if (isPinned)
    //             {
    //                 wprintf(L"[Debug] PINNED new file recursivo:\n");
    //                 fc.type = NEW_FILE;
    //                 fc.item_added = true;
    //                 result.push_back(fc);
    //             }
    //             // fc.type = NEW_FILE;
    //             // fc.item_added = true;
    //             // result.push_back(fc);
    //         }
    //     } while (FindNextFileW(hFind, &data));
    //     FindClose(hFind);
    // }
}

bool isFileValid(const std::wstring &fullPath, std::list<FileChange> &result, FileChange fc)
{
    std::filesystem::path p(fullPath);

    Logger::Write(L"[Log] File Validation\n", LogLevel::INFO);
    if (std::filesystem::file_size(p) > FILE_SIZE_LIMIT)
    {
        Logger::Write(L"[Error] ERROR_FILE_SIZE_EXCEEDED\n", LogLevel::ERROR);
        fc.type = ERROR_FILE_SIZE_EXCEEDED;
        fc.item_added = false;
        result.push_back(fc);
        return false;
    }
    else if (std::filesystem::file_size(p) == 0)
    {
        Logger::Write(L"[Error] ERROR_FILE_ZERO_SIZE\n", LogLevel::ERROR);
        fc.type = ERROR_FILE_ZERO_SIZE;
        fc.item_added = false;
        result.push_back(fc);
        return false;
    }
    else if (p.extension().string().empty())
    {
        Logger::Write(L"[Error] ERROR_FILE_NON_EXTENSION\n", LogLevel::ERROR);
        fc.type = ERROR_FILE_NON_EXTENSION;
        fc.item_added = false;
        result.push_back(fc);
        return false;
    }
    else
    {
        Logger::Write(L"[Log] Pass File Validation\n", LogLevel::INFO);
        return true;
    }
}

winrt::Windows::Foundation::IAsyncAction DirectoryWatcher::ReadChangesInternalAsync()
{
    co_await winrt::resume_background();

    while (true)
    {
        Logger::Write(L"[Log] Watching \n", LogLevel::INFO);
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
        FILE_NOTIFY_INFORMATION *next = _notify.get();
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

            // imprimir eventos que estan sucediendo

            if ((next->Action == FILE_ACTION_ADDED || (next->Action == FILE_ACTION_MODIFIED && !fileExists)) && !isTmpFile && !isDirectory && !isHidden)
            {
                Logger::Write(L"[Log] New File: %s\n" + fullPath.c_str() , LogLevel::INFO);
                if (isFileValid(fullPath, result, fc))
                {
                    fc.type = NEW_FILE;
                    fc.item_added = true;
                    result.push_back(fc);
                }
            }
            else if (next->Action == FILE_ACTION_ADDED && isDirectory && !isHidden)
            {
                ExploreDirectory(fullPath, result, fc);
            }
            else
            {
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

            // wprintf(L"next->FileName: %ls\n", next->FileName);
            // wprintf(L"next->Action: %d\n", next->Action);
            // wprintf(L"fileExists: %d\n", fileExists);
            // wprintf(L"isTmpFile: %d\n", isTmpFile);
            // wprintf(L"isDirectory: %d\n", isDirectory);

            if (next->NextEntryOffset)
            {
                next = reinterpret_cast<FILE_NOTIFY_INFORMATION *>(reinterpret_cast<char *>(next) + next->NextEntryOffset);
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
}

void DirectoryWatcher::Cancel()
{
    while (_readTask && (_readTask.Status() == winrt::AsyncStatus::Started) &&
           !CancelIoEx(_dir.get(), &_overlapped))
    {
        // Raced against the thread loop. Try again.
        Sleep(10);
    }
}
