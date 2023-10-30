#include <FolderWatcher.h>

void FolderWatcher::initialize(PCWSTR path, napi_env env) {
    _path = path;
    _notify.reset(reinterpret_cast<FILE_NOTIFY_INFORMATION *>(new char[32768]));

    _env = env;

    _dir.attach(
        CreateFileW(path,
                    FILE_LIST_DIRECTORY,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr,
                    OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                    nullptr));
}

void FolderWatcher::envGuard(napi_env env) {
    if (env == nullptr) {
        throw std::invalid_argument("env cannot be null");
    }
}

void FolderWatcher::pathGuard(PCWSTR path) {
    if (path == nullptr) {
        throw std::invalid_argument("path cannot be null");
    }
}

void FolderWatcher::addCallback(ICallback *callback, EventType type) {
    if (callback == nullptr) {
        throw std::invalid_argument("callback cannot be null");
    }

    callbacks.push_back(callback);
    callback_map[type].push_back(callback);
}

void FolderWatcher::addEventDetector(IEventDetector *eventDetector, const EventType type) {
    if (eventDetector == nullptr) {
        throw std::invalid_argument("eventDetector cannot be null");
    }

    event_detector_map[type].push_back(eventDetector);
}

void FolderWatcher::watch(std::wstring path, napi_env env) {
    initialize(path.c_str(), env);

    pathGuard(path.c_str());
    envGuard(env);

    while (true) {
        try {

        } catch (...) {
            wprintf(L"CloudProviderSyncRootWatcher watcher failed. Unknown error.\n");
            throw;
        }
    }
}