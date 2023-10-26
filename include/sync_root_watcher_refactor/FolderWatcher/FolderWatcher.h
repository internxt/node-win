#include "ICallback.h"
#include "FolderEvent.h"
#include "IEventDetector.h"

class FolderWatcher {
    const size_t max_size_buffer = 10;
    std::vector<ICallback*> callbacks;
    std::map<EventType, std::vector<ICallback*>> callback_map;
    std::map<EventType, std::vector<IEventDetector*>> event_detector_map;

    public: 
        void addCallback(ICallback *callback, EventType type);
        void addEventDetector(IEventDetector *eventDetector, const EventType type);
        void watch();
};