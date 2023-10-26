#include <chrono>
#include <iostream>

enum EventType {
    CREATE_FILE,
    CREATE_FOLDER,
    MODIFY_FILE,
};

class FolderEvent {
    EventType type;
    std::chrono::system_clock::time_point timestamp;
};