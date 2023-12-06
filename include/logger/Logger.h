#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

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

    void log(const std::string &message, LogLevel level);

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string toString(LogLevel level);

private:
    explicit Logger();
    ~Logger();

    std::ofstream log_file;
    std::mutex log_mutex;
};
