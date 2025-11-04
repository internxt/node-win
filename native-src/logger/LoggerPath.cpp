#include "LoggerPath.h"

std::string LoggerPath::path_ = "";
std::mutex LoggerPath::mutex_;

void LoggerPath::set(const std::string& path) {
    std::lock_guard<std::mutex> guard(mutex_);
    path_ = path;
}

std::string LoggerPath::get() {
    std::lock_guard<std::mutex> guard(mutex_);
    return path_;
}