#include "stdafx.h"
#include "DirectoryWatcher.h"
#include <iostream>
#include <filesystem>
#include <sstream>
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

CF_PLACEHOLDER_STANDARD_INFO getPlaceholderInfo(const std::wstring &directoryPath)
{
    HANDLE hFile = CreateFileW(
        directoryPath.c_str(),
        FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        int size = sizeof(CF_PLACEHOLDER_STANDARD_INFO) + 300;
        CF_PLACEHOLDER_STANDARD_INFO PlaceholderInfo;
        DWORD returnlength(0);
        HRESULT hr = CfGetPlaceholderInfo(hFile, CF_PLACEHOLDER_INFO_STANDARD, &PlaceholderInfo, size, &returnlength);
        if (SUCCEEDED(hr))
        {
            Logger::getInstance().log("getPlaceholderInfo: Success\n", LogLevel::INFO);
            return PlaceholderInfo;
            CloseHandle(hFile);
        }
        else
        {
            CloseHandle(hFile);
            Logger::getInstance().log("getPlaceholderInfo: Failed, CfGetPlaceholderInfo failed.\n", LogLevel::ERROR);
            return CF_PLACEHOLDER_STANDARD_INFO{};
        }
    }
}

bool isFileValid(const std::wstring &fullPath, std::list<FileChange> &result, FileChange fc)
{
    std::filesystem::path p(fullPath);

    Logger::getInstance().log("File Validation:  " + Logger::fromWStringToString(fullPath), LogLevel::INFO);
    if (std::filesystem::file_size(p) > FILE_SIZE_LIMIT)
    {
        Logger::getInstance().log("ERROR_FILE_SIZE_EXCEEDED", LogLevel::ERROR);
        fc.type = ERROR_FILE_SIZE_EXCEEDED;
        fc.item_added = false;
        result.push_back(fc);
        return false;
    }
    else if (std::filesystem::file_size(p) == 0)
    {
        Logger::getInstance().log("ERROR_FILE_ZERO_SIZE", LogLevel::ERROR);
        fc.type = ERROR_FILE_ZERO_SIZE;
        fc.item_added = false;
        result.push_back(fc);
        return false;
    }
    else if (p.extension().string().empty())
    {
        Logger::getInstance().log("ERROR_FILE_NON_EXTENSION", LogLevel::ERROR);
        fc.type = ERROR_FILE_NON_EXTENSION;
        fc.item_added = false;
        result.push_back(fc);
        return false;
    }
    else
    {
        // Logger::getInstance().log("Pass File Validation", LogLevel::INFO);
        return true;
    }
}

void ExploreDirectory(const std::wstring &directoryPath, std::list<FileChange> &result, FileChange fc)
{
    Logger::getInstance().log("New Folder: " + Logger::fromWStringToString(directoryPath), LogLevel::INFO);
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
                Logger::getInstance().log("New file processed recursively: " + Logger::fromWStringToString(data.cFileName), LogLevel::INFO);
                std::wstring fullPath2 = directoryPath + L"\\" + data.cFileName;
                FileChange fc;
                if (isFileValid(fullPath2, result, fc))
                {

                    fc.path = fullPath2;
                    fc.type = NEW_FILE;
                    fc.item_added = true;
                    result.push_back(fc);
                }
            }
        } while (FindNextFileW(hFind, &data));
        FindClose(hFind);
    }
}

winrt::Windows::Foundation::IAsyncAction DirectoryWatcher::ReadChangesInternalAsync()
{
    co_await winrt::resume_background();
    bool folderRenamedAtCreation = false;
    std::wstring folderRenamedAtCreationPath = L"";
    while (true)
    {
        Logger::getInstance().log("Watching...", LogLevel::INFO);
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

            if ((next->Action == FILE_ACTION_ADDED || (next->Action == FILE_ACTION_MODIFIED && !fileExists)) && !isTmpFile && !isDirectory && !isHidden)
            {
                Logger::getInstance().log("New File: " + Logger::fromWStringToString(fullPath), LogLevel::INFO);
                if (isFileValid(fullPath, result, fc))
                {
                    fc.type = NEW_FILE;
                    fc.item_added = true;
                    result.push_back(fc);
                }
            }
            else if (next->Action == FILE_ACTION_ADDED && isDirectory && !isHidden)
            {
                Logger::getInstance().log("New Folder: " + Logger::fromWStringToString(fullPath), LogLevel::INFO);
                Sleep(2000);
                if (std::filesystem::exists(fullPath)) {
                    Logger::getInstance().log("New Folder exist: " + Logger::fromWStringToString(fullPath) + " exists", LogLevel::INFO);
                    ExploreDirectory(fullPath, result, fc);
                } else {
                    Logger::getInstance().log("New Folder does not exist: " + Logger::fromWStringToString(fullPath) + " does not exist", LogLevel::INFO);
                    folderRenamedAtCreationPath = fullPath;
                }
            }
            else if (next->Action == FILE_ACTION_RENAMED_OLD_NAME && folderRenamedAtCreationPath == fullPath) {
                folderRenamedAtCreation = true;
            }
            else if (next->Action == FILE_ACTION_RENAMED_NEW_NAME && folderRenamedAtCreation) {
                Logger::getInstance().log("New Folder renamed at creation: " + Logger::fromWStringToString(fullPath), LogLevel::INFO);
                fc.type = NEW_FOLDER;
                fc.item_added = true;
                result.push_back(fc);
                folderRenamedAtCreation = false;
                folderRenamedAtCreationPath = L"";
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
        result.sort([](const FileChange &a, const FileChange &b)
                    { return std::tie(a.path, a.type) < std::tie(b.path, b.type); });

        // Use unique to remove duplicates
        result.unique([](const FileChange &a, const FileChange &b)
                      { return a.path == b.path && a.type == b.type; });

        // ordernar de primero las paths de folders y luego las de archivos
        result.sort([](const FileChange &a, const FileChange &b)
                    {
                        bool a_is_folder = a.path.back() == L'\\' || a.path.back() == L'/';
                        bool b_is_folder = b.path.back() == L'\\' || b.path.back() == L'/';
                        if (a_is_folder && !b_is_folder)
                            return true;
                        if (!a_is_folder && b_is_folder)
                            return false;
                        return a.path < b.path; });
        // TODO: delete this, but it is used to Debug the results from watcher
        // for (auto change : result)
        //{
        //    wprintf(L"[Log] change.path: %s\n", change.path.c_str());
        //    wprintf(L"[Log] change.type: %d\n", change.type);
        //}
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
