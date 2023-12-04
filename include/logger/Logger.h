#pragma once

#include <iostream>
#include <fstream>
#include <string>

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    TRACE,
    FATAL
};

extern std::string loggerPath;
class Logger {
public:
    void log(const std::string &message, LogLevel level);

    explicit Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream log_file;

    std::string toString(LogLevel level);

    static Logger* instance;
};