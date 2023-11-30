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
    ERROR,  // Corrige la ortografía aquí
    TRACE,
    FATAL
};

class Logger {
public:
    // Método para obtener la instancia singleton.
    static Logger& getInstance();

    // Método para inicializar la instancia singleton con un path específico.
    static void initialize(const std::string &file_name);

    // Método para registrar mensajes.
    void log(const std::string &message, LogLevel level);

private:
    // Constructor privado.
    explicit Logger(const std::string &file_name);
    ~Logger();

    // Copia y asignación privadas para evitar copias.
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Stream de archivo para el log.
    std::ofstream log_file;

    // Método privado para convertir LogLevel a string.
    std::string toString(LogLevel level);

    // Instancia singleton.
    static Logger* instance;
};