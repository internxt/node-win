#include "Logger.h"
#include "LoggerPath.h"

Logger::Logger() : log_file(LoggerPath::get(), std::ios::app) {
    std::string path = LoggerPath::get();
    if (!log_file.is_open() && !path.empty()) {
        throw std::runtime_error("No se pudo abrir el archivo de log.");
    }
}

Logger::~Logger() {
    if (log_file.is_open()) {
        log_file.close();
    }
}

void Logger::log(const std::string &message, LogLevel level) {
    std::lock_guard<std::mutex> guard(log_mutex); // Bloquear el mutex
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