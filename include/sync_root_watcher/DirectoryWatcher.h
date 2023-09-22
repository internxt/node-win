#pragma once

#include <node_api.h>
struct FileChange {
    std::wstring path;
    bool file_added;
};

struct InputCallbacks {
    napi_ref notify_file_added_callback_ref;
};

struct InputSyncCallbacksThreadsafe {
    napi_threadsafe_function notify_file_added_threadsafe_callback;
};
class DirectoryWatcher
{
public:
    std::atomic<bool> _shouldRun;
    void Initialize(_In_ PCWSTR path, _In_ std::function<void(std::list<FileChange>&, napi_env env, InputSyncCallbacksThreadsafe input)> callback, napi_env env, InputSyncCallbacksThreadsafe input);
    winrt::Windows::Foundation::IAsyncAction ReadChangesAsync();
    void Cancel();

private:
    winrt::Windows::Foundation::IAsyncAction ReadChangesInternalAsync();

    winrt::handle _dir;
    std::wstring _path;
    napi_env _env;
    InputSyncCallbacksThreadsafe _input;
    std::unique_ptr<FILE_NOTIFY_INFORMATION> _notify;
    OVERLAPPED _overlapped{};
    winrt::Windows::Foundation::IAsyncAction _readTask;
    std::function<void(std::list<FileChange>&, napi_env env, InputSyncCallbacksThreadsafe input)> _callback;
};

