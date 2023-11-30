#include "Logger.h"

// Inicializa el puntero de la instancia singleton a nullptr.
Logger* Logger::instance = nullptr;

Logger& Logger::getInstance() {
    if (instance == nullptr) {
        throw std::logic_error("El singleton Logger debe ser inicializado antes de su uso.");
    }
    return *instance;
}

void Logger::initialize(const std::string &file_name) {
    if (instance == nullptr) {
        instance = new Logger(file_name);
    }
}

Logger::Logger(const std::string &file_name) : log_file(file_name, std::ios::app) {
    if (!log_file.is_open()) {
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
        case LogLevel::ERROR: return "ERROR";  // Corrige la ortografía aquí también
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}