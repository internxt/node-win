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

    // Obtener la hora y fecha actual
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_c);

    // Formatear la fecha y hora
    std::ostringstream time_stream;
    time_stream << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");

    // Escribir en el archivo de log
    log_file << "[" << toString(level) << "] " 
             << time_stream.str() << " " 
             << message << std::endl;
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