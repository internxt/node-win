#include "Logger.h"
#include "LoggerPath.h"

Logger::Logger() : log_file(LoggerPath::get(), std::ios::app) {
    std::wstring widePath = fromUtf8ToWide(LoggerPath::get());
    wprintf(L"Logger path: %ls\n", widePath.c_str());

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

std::wstring Logger::fromUtf8ToWide(const std::string& utf8Str) {
    if (utf8Str.empty()) return std::wstring();

    int wideSize = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
    if (wideSize <= 0) return std::wstring();

    std::unique_ptr<wchar_t[]> wideStr(new wchar_t[wideSize]);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, wideStr.get(), wideSize);

    return std::wstring(wideStr.get());
}

void Logger::log(const std::string &message, LogLevel level, WORD color) {
    std::lock_guard<std::mutex> guard(log_mutex);

    // Obtener la fecha y hora actual formateada
    auto now = std::chrono::system_clock::now();
    auto now_as_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::tm now_tm = *std::localtime(&now_as_time_t);

    std::ostringstream time_stream;
    time_stream << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
    time_stream << '.' << std::setfill('0') << std::setw(3) << now_ms.count();

    std::string level_str = toString(level);
    std::transform(level_str.begin(), level_str.end(), level_str.begin(), ::tolower);

    // Escribir en el archivo de log (sin colores)
    log_file << "[" << time_stream.str() << "] [" << level_str << "] " << message << std::endl;

    // Preparar la salida en consola
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    WORD saved_attributes = consoleInfo.wAttributes;

    // Si se ha especificado un color (distinto de 0), cambiar el atributo
    if (color != 0) {
        SetConsoleTextAttribute(hConsole, color);
    }

    // Imprimir en consola
    printf("[%s] [%s] %s\n", time_stream.str().c_str(), level_str.c_str(), message.c_str());

    // Restaurar el atributo original
    if (color != 0) {
        SetConsoleTextAttribute(hConsole, saved_attributes);
    }
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

std::string Logger::fromWStringToString(const std::wstring wstr) {
    if (wstr.empty()) return std::string();

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}
