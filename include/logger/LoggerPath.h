#pragma once

#include <mutex>
#include <string>
#include <node_api.h>

struct LoggerInfo {
    std::string path_;
    napi_threadsafe_function threadsafe_callback_;
};
class LoggerPath {
public:
    static void set(const std::string& path, napi_threadsafe_function callback);
    static LoggerInfo get();

private:
    static std::string path_;
    static napi_threadsafe_function threadsafe_callback_;
    static std::mutex mutex_;
};
