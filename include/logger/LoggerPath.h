#pragma once

#include <mutex>
#include <string>

class LoggerPath {
public:
    static void set(const std::string& path);
    static std::string get();

private:
    static std::string path_;
    static std::mutex mutex_;
};
