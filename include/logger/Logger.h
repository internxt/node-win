#pragma once

#ifdef ERROR
#undef ERROR
#endif

#include <iostream>
#include <fstream>
#include <string>

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR, 
    TRACE,
    FATAL
};

class Logger {
public:
    static Logger& getInstance();

    static void initialize(const std::string &file_name);

    void log(const std::string &message, LogLevel level);

private:
    explicit Logger(const std::string &file_name);
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream log_file;

    std::string toString(LogLevel level);

    static Logger* instance;
};