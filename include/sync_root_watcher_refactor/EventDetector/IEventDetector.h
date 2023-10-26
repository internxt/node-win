#include <deque>
#include <FolderEvent.h>

class IEventDetector {
    public:
        virtual void detect(std::deque<FolderEvent> EventBuffer) = 0;
};