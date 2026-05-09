#ifndef TKDE_LOGGER_HPP
#define TKDE_LOGGER_HPP

#include <cstdio>
#include <cstring>
#include <ctime>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace tkde {

enum class LogLevel { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3 };
static constexpr const char* LEVEL_NAMES[] = { "DEBUG", "INFO", "WARN", "ERROR" };

class Logger {
    static constexpr size_t MAX_LOG_LEN = 4096;

    struct LogEntry {
        LogLevel level;
        std::string file;
        int line;
        std::string func;
        std::string message;
        time_t ts;
    };

    std::mutex mutex_;
    std::vector<LogEntry> buffer_;
    size_t maxBuffer_ = 1000;
    FILE* fileHandle_ = nullptr;
    LogLevel minLevel_ = LogLevel::DEBUG;
    bool termuxColor_ = true;

    static std::string timestamp() {
        char buf[32];
        time_t now = time(nullptr);
        struct tm* t = localtime(&now);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
        return std::string(buf);
    }

    void flushEntry(const LogEntry& e) {
        const char* color = "";
        const char* reset = "";

        if (termuxColor_) {
            switch (e.level) {
                case LogLevel::DEBUG: color = "\033[90m"; break;  // grey
                case LogLevel::INFO:  color = "\033[36m"; break;  // cyan
                case LogLevel::WARN:  color = "\033[33m"; break;  // yellow
                case LogLevel::ERROR: color = "\033[31m"; break;  // red
            }
            reset = "\033[0m";
        }

        char entry[MAX_LOG_LEN];
        int n = snprintf(entry, sizeof(entry),
            "[%s] \033[1m%s\033[0m %s%-5s\033[0m %s:%d (%s) %s%s\033[0m\n",
            timestamp().c_str(),
            color, LEVEL_NAMES[static_cast<int>(e.level)], reset,
            e.file.c_str(), e.line, e.func.c_str(),
            e.message.c_str(), reset);

        std::fwrite(entry, 1, n, stdout);
        if (fileHandle_) {
            std::fwrite(entry, 1, n, fileHandle_);
            std::fflush(fileHandle_);
        }
    }

public:
    ~Logger() { close(); }

    void open(const std::string& path) {
        std::lock_guard<std::mutex> l(mutex_);
        if (fileHandle_) std::fclose(fileHandle_);
        fileHandle_ = std::fopen(path.c_str(), "a");
    }

    void close() {
        std::lock_guard<std::mutex> l(mutex_);
        if (fileHandle_) {
            std::fclose(fileHandle_);
            fileHandle_ = nullptr;
        }
    }

    void setMinLevel(LogLevel lvl) { minLevel_ = lvl; }
    void setTermuxColor(bool v) { termuxColor_ = v; }

    void log(LogLevel lvl, const char* file, int line, const char* func, const char* fmt, ...) {
        if (lvl < minLevel_) return;

        char msg[2048];
        va_list args;
        va_start(args, fmt);
        vsnprintf(msg, sizeof(msg), fmt, args);
        va_end(args);

        LogEntry e;
        e.level = lvl;
        e.file = file ? file : "";
        e.line = line;
        e.func = func ? func : "";
        e.message = msg;
        e.ts = time(nullptr);

        std::lock_guard<std::mutex> l(mutex_);
        if (buffer_.size() >= maxBuffer_) buffer_.erase(buffer_.begin());
        buffer_.push_back(e);
        flushEntry(e);
    }

    // Variadic helper
    template<typename... Args>
    void debug(const char* file, int line, const char* func, const char* fmt, Args... args);
    template<typename... Args>
    void info(const char* file, int line, const char* func, const char* fmt, Args... args);
    template<typename... Args>
    void warn(const char* file, int line, const char* func, const char* fmt, Args... args);
    template<typename... Args>
    void error(const char* file, int line, const char* func, const char* fmt, Args... args);
};

template<typename... Args>
void Logger::debug(const char* f, int l, const char* fn, const char* fmt, Args... a) {
    log(LogLevel::DEBUG, f, l, fn, fmt, a...);
}
template<typename... Args>
void Logger::info(const char* f, int l, const char* fn, const char* fmt, Args... a) {
    log(LogLevel::INFO, f, l, fn, fmt, a...);
}
template<typename... Args>
void Logger::warn(const char* f, int l, const char* fn, const char* fmt, Args... a) {
    log(LogLevel::WARN, f, l, fn, fmt, a...);
}
template<typename... Args>
void Logger::error(const char* f, int l, const char* fn, const char* fmt, Args... a) {
    log(LogLevel::ERROR, f, l, fn, fmt, a...);
}

extern Logger g_log;

#define TKDE_DEBUG(...)  tkde::g_log.debug(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define TKDE_INFO(...)  tkde::g_log.info(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define TKDE_WARN(...)  tkde::g_log.warn(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define TKDE_ERROR(...) tkde::g_log.error(__FILE__, __LINE__, __func__, __VA_ARGS__)

} // namespace tkde
#endif // TKDE_LOGGER_HPP