#include "ICallback.h"
#include "FolderEvent.h"
#include "IEventDetector.h"

class FolderWatcher {
    const size_t max_size_buffer = 10;
    std::vector<ICallback*> callbacks;
    std::map<EventType, std::vector<ICallback*>> callback_map;
    std::map<EventType, std::vector<IEventDetector*>> event_detector_map;
    std::deque<FolderEvent> event_buffer;
    std::unique_ptr<FILE_NOTIFY_INFORMATION> _notify;
    napi_env _env;
    winrt::handle _dir;
    std::wstring _path;

    public: 
        void pathGuard(PCWSTR path);
        void envGuard(napi_env env);
        void addCallback(ICallback *callback, EventType type);
        void addEventDetector(IEventDetector *eventDetector, const EventType type);
        void initialize(PCWSTR path, napi_env env);
        void watch(std::wstring path, napi_env env);
};