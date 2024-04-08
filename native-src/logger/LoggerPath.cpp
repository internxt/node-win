#include "LoggerPath.h"

std::string LoggerPath::path_ = "";
napi_threadsafe_function LoggerPath::threadsafe_callback_ = nullptr;
std::mutex LoggerPath::mutex_;

void LoggerPath::set(const std::string& path, napi_threadsafe_function callback) {
    std::lock_guard<std::mutex> guard(mutex_);
    path_ = path;
    threadsafe_callback_ = callback;
}

LoggerInfo LoggerPath::get() {
    std::lock_guard<std::mutex> guard(mutex_);
    LoggerInfo response = {
        path_,
        threadsafe_callback_ 
    };
    return response;
}