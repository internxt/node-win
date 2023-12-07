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
    std::lock_guard<std::mutex> guard(log_mutex);

    auto now = std::chrono::system_clock::now();
    auto now_as_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::tm now_tm = *std::localtime(&now_as_time_t);

    std::ostringstream time_stream;
    time_stream << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
    time_stream << '.' << std::setfill('0') << std::setw(3) << now_ms.count();

    std::string level_str = toString(level);
    std::transform(level_str.begin(), level_str.end(), level_str.begin(), ::tolower);

    log_file << "[" << time_stream.str() << "] [" << level_str << "] " << message << std::endl;
    printf("[%s] [%s] %s\n", time_stream.str().c_str(), level_str.c_str(), message.c_str());
}

std::string Logger::toString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}