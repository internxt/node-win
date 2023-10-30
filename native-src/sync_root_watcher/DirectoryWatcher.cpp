#include "stdafx.h"
#include "DirectoryWatcher.h"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <algorithm> // Asegúrate de incluir esta cabecera para std::min

namespace fs = std::filesystem;
const size_t c_bufferSize = 32768; // sizeof(FILE_NOTIFY_INFORMATION) * 100;
struct NotificationInfo
{
    DWORD Action;          // Puedes usar un DWORD para almacenar la acción
    std::wstring FileName; // Nombre del archivo afectado (usando wstring para manejar nombres Unicode)
    std::chrono::system_clock::time_point timestamp;
};

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

void ExploreDirectory(const std::wstring &directoryPath, std::list<FileChange> &result, FileChange fc)
{
    // FileChange fc;
    wprintf(L"\nnew folder: %s\n", directoryPath.c_str());
    fc.path = directoryPath;
    fc.type = NEW_FOLDER;
    fc.item_added = true;
    result.push_back(fc);

    std::wstring searchPath = directoryPath + L"\\*";
    WIN32_FIND_DATAW data;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &data);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (wcscmp(data.cFileName, L".") != 0 && wcscmp(data.cFileName, L"..") != 0)
                {
                    // Es un directorio, llama a la función recursivamente
                    std::wstring fullPath2 = directoryPath + L"\\" + data.cFileName;
                    ExploreDirectory(fullPath2, result, fc);
                }
            }
            else
            {
                wprintf(L"new file recursivo: %s\n", data.cFileName);
                // Es un archivo
                std::wstring fullPath2 = directoryPath + L"\\" + data.cFileName;
                FileChange fc;
                fc.path = fullPath2;
                fc.type = NEW_FILE;
                fc.item_added = true;
                result.push_back(fc);
            }
        } while (FindNextFileW(hFind, &data));
        FindClose(hFind);
    }
}

winrt::Windows::Foundation::IAsyncAction DirectoryWatcher::ReadChangesInternalAsync()
{
    co_await winrt::resume_background();
    std::list<NotificationInfo> notificationInfoList;
    while (true)
    {
        wprintf(L"[Control] watcher running\n");
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

        // DWORD transferred;
        // wprintf(L"[Control] Expecting\n");
        // // std::chrono::system_clock::time_point vent = std::chrono::system_clock::now();
        // if (!GetOverlappedResult(_dir.get(), &_overlapped, &transferred, TRUE))
        // {
        //     DWORD error = GetLastError();
        //     if (error != ERROR_OPERATION_ABORTED)
        //     {
        //         throw winrt::hresult_error(HRESULT_FROM_WIN32(error));
        //     }
        //     break;
        // }
        // Events detector implementing -- start
        std::chrono::system_clock::time_point lastTimestamp;
        std::list<NotificationInfo> eventsCaptured;
        NotificationInfo lastNotification;
        std::wstring lastPath;
        bool isLastTimestampSet = false;

        DWORD transferred;
        bool isEventSet = false;
        const DWORD timeout = 1000; // tiempo de espera en milisegundos
        DWORD timeAccumulator = 0;
        while (!isEventSet)
        {
            if (GetOverlappedResult(_dir.get(), &_overlapped, &transferred, FALSE))
            {
                // se ha producido un evento, hacer algo aquí
                isEventSet = true;
            }
            else
            {
                DWORD error = GetLastError();
                if (error != ERROR_IO_INCOMPLETE)
                {
                    throw winrt::hresult_error(HRESULT_FROM_WIN32(error));
                }
                // wprintf(L"[Control] esperando evento...\n");
                Sleep(timeout);
                timeAccumulator += timeout;
                // si ya paso el acumulador 4 segundos evaluar los ultimos eventos
                if (timeAccumulator >= 4000)
                {
                    // wprintf(L"[Control] 4 seconds - unheard evetns\n");
                }
            }
        }

        std::list<FileChange> result;
        std::list<std::wstring> addedFiles;
        std::list<std::wstring> removedFiles;
        FILE_NOTIFY_INFORMATION *next = _notify.get();
        wprintf(L"[Control] Event emited1\n");
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

            // almacenar eventos
            NotificationInfo notificationInfo;
            notificationInfo.Action = next->Action;
            notificationInfo.FileName = fullPath;
            notificationInfo.timestamp = std::chrono::system_clock::now();
            notificationInfoList.push_back(notificationInfo);

            // if ((next->Action == FILE_ACTION_ADDED || (next->Action == FILE_ACTION_MODIFIED && !fileExists)) && !isTmpFile && !isDirectory && !isHidden)
            // {
            //     wprintf(L"\nnew file: %s\n", fullPath.c_str());
            //     fc.type = NEW_FILE;
            //     fc.item_added = true;
            //     result.push_back(fc);
            // }
            // else if (next->Action == FILE_ACTION_ADDED && isDirectory && !isHidden)
            // {
            //     ExploreDirectory(fullPath, result, fc);
            // }
            // else
            // {
            //     fc.type = OTHER;
            //     fc.item_added = false;
            //     result.push_back(fc);
            // }

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

        for (NotificationInfo notificationInfo : notificationInfoList)
        {
            wprintf(L"[Control] notificationInfo [Action]: %d\n", notificationInfo.Action);
            wprintf(L"[Control] notificationInfo [lastTimestamp]: %lld\n", lastTimestamp.time_since_epoch().count());
            wprintf(L"[Control] notificationInfo [lastPath]: %ls\n", lastPath.c_str());
            wprintf(L"[Control] notificationInfo [FileName]: %ls\n", notificationInfo.FileName.c_str());
            wprintf(L"[Control] notificationInfo [timestamp]: %lld\n", notificationInfo.timestamp.time_since_epoch().count());
            if (!isLastTimestampSet)
            {
                wprintf(L"[Control] No lastTimestamp set, maybe new event\n");
            }
            else
            {
                std::chrono::duration<double> elapsed_seconds = notificationInfo.timestamp - lastTimestamp;
                std::chrono::milliseconds elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_seconds);
                wprintf(L"[Control] time since last notifition (seconds): %f\n", elapsed_seconds.count());
                wprintf(L"[Control] time since epoch (milliseconds): %lld\n", elapsed_milliseconds.count());
                if (elapsed_milliseconds.count() < 5000)
                {
                    eventsCaptured.push_back(notificationInfo);
                }
                else
                {
                    wprintf(L"[Control] Likely new event! - called events removed \n");
                }
            }
            lastNotification = notificationInfo;
            lastTimestamp = notificationInfo.timestamp;
            lastPath = notificationInfo.FileName;
            isLastTimestampSet = true;
        }
        // Events detector implementing -- end
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
