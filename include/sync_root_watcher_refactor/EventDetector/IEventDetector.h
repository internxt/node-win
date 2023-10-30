#include <deque>
#include <FolderEvent.h>

class IEventDetector {
    public:
        virtual boolean detect(std::deque<FolderEvent>* EventBuffer) = 0;
};