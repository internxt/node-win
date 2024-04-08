#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <windows.h>
#include<node_api.h>

#ifdef ERROR
#undef ERROR
#endif

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    TRACE,
    ERROR,
    FATAL
};

extern std::string loggerPath;
class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void setCallback(napi_env env, napi_threadsafe_function callback);

    void log(const std::string &message, LogLevel level);

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string toString(LogLevel level);

    static std::string fromWStringToString(const std::wstring wstr);
private:
    explicit Logger();
    ~Logger();

    napi_env env;
    napi_threadsafe_function threadsafe_callback;

    std::ofstream log_file;
    std::mutex log_mutex;
};
