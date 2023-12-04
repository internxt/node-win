#include "Logger.h"

std::string loggerPath = "";

Logger::Logger() : log_file(loggerPath, std::ios::app) {
    printf("Logger path: %s\n", loggerPath.c_str());
    if (!log_file.is_open() && !loggerPath.empty()) {
        throw std::runtime_error("No se pudo abrir el archivo de log.");
    }
}

Logger::~Logger() {
    if (log_file.is_open()) {
        log_file.close();
    }
}

void Logger::log(const std::string &message, LogLevel level) {
    log_file << "[" << toString(level) << "] " << message << std::endl;
}

std::string Logger::toString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}