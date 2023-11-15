#pragma once

#include <node_api.h>

// 20GB limit
#define FILE_SIZE_LIMIT 21474836480

enum ChangeType
{
    NEW_FILE,
    NEW_FOLDER,
    MODIFIED_FILE,
    ERROR_FILE_SIZE_EXCEEDED,
    ERROR_FOLDER_SIZE_EXCEEDED,
    OTHER
};
struct FileChange
{
    std::wstring path;
    bool item_added;
    ChangeType type;
    std::wstring message;
};

struct InputCallbacks
{
    napi_ref notify_file_added_callback_ref;
    napi_ref notify_message_callback_ref;
};

struct InputSyncCallbacksThreadsafe
{
    napi_threadsafe_function notify_file_added_threadsafe_callback;
    napi_threadsafe_function notify_message_threadsafe_callback;
};
class DirectoryWatcher
{
public:
    std::atomic<bool> _shouldRun;
    void Initialize(_In_ PCWSTR path, _In_ std::function<void(std::list<FileChange> &, napi_env env, InputSyncCallbacksThreadsafe input)> callback, napi_env env, InputSyncCallbacksThreadsafe input);
    winrt::Windows::Foundation::IAsyncAction ReadChangesAsync();
    void Cancel();

private:
    winrt::Windows::Foundation::IAsyncAction ReadChangesInternalAsync();
    std::map<std::wstring, DWORD> _file_sizes;
    winrt::handle _dir;
    std::wstring _path;
    napi_env _env;
    InputSyncCallbacksThreadsafe _input;
    std::unique_ptr<FILE_NOTIFY_INFORMATION> _notify;
    OVERLAPPED _overlapped{};
    winrt::Windows::Foundation::IAsyncAction _readTask;
    std::function<void(std::list<FileChange> &, napi_env env, InputSyncCallbacksThreadsafe input)> _callback;
};
