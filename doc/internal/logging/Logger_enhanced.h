#pragma once

#include "AnsiColors.h"
#include <cstdlib>
#include <memory>
#include <mutex>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/syslog_sink.h>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>

#if !defined(FMT_VERSION) || FMT_VERSION < 90000
#include <optional>
#include <sstream>
namespace fmt {
template <typename T>
struct streamed_t {
    const T &value;
    explicit streamed_t(const T &v) : value(v) {}
};

template <typename T>
streamed_t<T> streamed(const T &v) { return streamed_t<T>(v); }

template <typename OStream, typename T>
OStream &operator<<(OStream &os, const streamed_t<T> &s) {
    os << s.value;
    return os;
}

// Fallback for std::optional<T>
template <typename OStream, typename T>
OStream &operator<<(OStream &os, const streamed_t<std::optional<T>> &s) {
    if (s.value.has_value()) {
        os << *s.value;
    } else {
        os << "<nullopt>";
    }
    return os;
}
} // namespace fmt
#endif

namespace doip {

constexpr const char *DEFAULT_PATTERN = "[%H:%M:%S.%e] [%n] [%^%l%$] %v";
constexpr const char *SHORT_PATTERN = "[%n] [%^%l%$] %v";
constexpr const char *SYSLOG_PATTERN = "[%n] %v";  // Syslog adds its own timestamp

/**
 * @brief Logger output mode
 */
enum class LoggerMode {
    Console,    // stdout with colors
    Syslog,     // syslog daemon
    File        // file output (future extension)
};

/**
 * @brief Logger configuration
 */
struct LoggerConfig {
    LoggerMode mode = LoggerMode::Console;
    spdlog::level::level_enum level = spdlog::level::info;
    std::string pattern = DEFAULT_PATTERN;
    std::string syslog_ident = "doipd";  // Identifier for syslog
    int syslog_facility = LOG_DAEMON;     // LOG_DAEMON, LOG_USER, etc.
    bool enable_colors = true;            // Only used in console mode
};

/**
 * @brief Centralized logger factory for the DoIP library
 * 
 * This class manages logger instances and allows switching between
 * console and syslog output modes. It should be initialized early
 * in main() before any logging occurs.
 * 
 * Usage:
 *   // In main(), before daemonization:
 *   LoggerFactory::initialize(LoggerMode::Console);
 *   
 *   // After daemonization:
 *   LoggerFactory::switchToSyslog("doipd");
 *   
 *   // In your code:
 *   LOG_DOIP_INFO("Server started");
 */
class LoggerFactory {
public:
    /**
     * @brief Initialize the logger factory with a specific mode
     * 
     * @param config Logger configuration
     */
    static void initialize(const LoggerConfig &config = LoggerConfig{}) {
        std::lock_guard<std::mutex> lock(getMutex());
        
        getConfig() = config;
        
        // Clear existing loggers to force recreation
        m_loggers.clear();
        spdlog::drop_all();
        
        // Create default loggers
        createLogger("doip");
        createLogger("udp ");
        createLogger("tcp ");
    }
    
    /**
     * @brief Switch all loggers to syslog mode
     * 
     * Call this after daemonization to redirect all log output to syslog
     * 
     * @param ident Syslog identifier (program name)
     * @param facility Syslog facility (LOG_DAEMON, LOG_USER, etc.)
     */
    static void switchToSyslog(const std::string &ident = "doipd", 
                               int facility = LOG_DAEMON) {
        std::lock_guard<std::mutex> lock(getMutex());
        
        auto &cfg = getConfig();
        cfg.mode = LoggerMode::Syslog;
        cfg.syslog_ident = ident;
        cfg.syslog_facility = facility;
        cfg.pattern = SYSLOG_PATTERN;
        
        // Recreate all loggers with syslog sink
        m_loggers.clear();
        spdlog::drop_all();
        
        createLogger("doip");
        createLogger("udp ");
        createLogger("tcp ");
    }
    
    /**
     * @brief Switch all loggers to console mode
     * 
     * @param enable_colors Enable colored output
     */
    static void switchToConsole(bool enable_colors = true) {
        std::lock_guard<std::mutex> lock(getMutex());
        
        auto &cfg = getConfig();
        cfg.mode = LoggerMode::Console;
        cfg.enable_colors = enable_colors;
        cfg.pattern = DEFAULT_PATTERN;
        
        // Recreate all loggers with console sink
        m_loggers.clear();
        spdlog::drop_all();
        
        createLogger("doip");
        createLogger("udp ");
        createLogger("tcp ");
    }
    
    /**
     * @brief Get or create a logger instance
     * 
     * @param name Logger name
     * @param level Log level (uses default from config if not specified)
     * @return std::shared_ptr<spdlog::logger> Logger instance
     */
    static std::shared_ptr<spdlog::logger> get(
        const std::string &name = "doip", 
        std::optional<spdlog::level::level_enum> level = std::nullopt) {
        
        std::lock_guard<std::mutex> lock(getMutex());
        
        if (auto it = m_loggers.find(name); it != m_loggers.end()) {
            return it->second;
        }
        
        auto logger = createLogger(name);
        if (level.has_value()) {
            logger->set_level(level.value());
        }
        return logger;
    }
    
    /**
     * @brief Get UDP logger
     */
    static std::shared_ptr<spdlog::logger> getUdp() {
        return get("udp ");
    }
    
    /**
     * @brief Get TCP logger
     */
    static std::shared_ptr<spdlog::logger> getTcp() {
        return get("tcp ");
    }
    
    /**
     * @brief Set global log level for all loggers
     * 
     * @param level Log level
     */
    static void setLevel(spdlog::level::level_enum level) {
        std::lock_guard<std::mutex> lock(getMutex());
        getConfig().level = level;
        
        for (auto &[name, logger] : m_loggers) {
            logger->set_level(level);
        }
    }
    
    /**
     * @brief Set pattern for all loggers
     * 
     * @param pattern spdlog pattern string
     */
    static void setPattern(const std::string &pattern) {
        std::lock_guard<std::mutex> lock(getMutex());
        getConfig().pattern = pattern;
        
        for (auto &[name, logger] : m_loggers) {
            logger->set_pattern(pattern);
        }
    }
    
    /**
     * @brief Check if color output is supported by the terminal
     * 
     * @return true if colors are supported
     */
    static bool colorsSupported() {
        const char *term = std::getenv("TERM");
        const char *colorterm = std::getenv("COLORTERM");
        
        if (term == nullptr)
            return false;
        
        std::string termStr(term);
        return termStr.find("color") != std::string::npos ||
               termStr.find("xterm") != std::string::npos ||
               termStr.find("screen") != std::string::npos ||
               colorterm != nullptr;
    }
    
    /**
     * @brief Get current logger mode
     */
    static LoggerMode getMode() {
        std::lock_guard<std::mutex> lock(getMutex());
        return getConfig().mode;
    }
    
private:
    /**
     * @brief Create a logger based on current configuration
     * 
     * @param name Logger name
     * @return std::shared_ptr<spdlog::logger> Created logger
     */
    static std::shared_ptr<spdlog::logger> createLogger(const std::string &name) {
        auto &cfg = getConfig();
        std::shared_ptr<spdlog::logger> logger;
        
        switch (cfg.mode) {
            case LoggerMode::Syslog: {
                // Create syslog sink
                auto syslog_sink = std::make_shared<spdlog::sinks::syslog_sink_mt>(
                    cfg.syslog_ident, 
                    LOG_PID,           // Include PID in log messages
                    cfg.syslog_facility,
                    true               // Enable syslog
                );
                
                logger = std::make_shared<spdlog::logger>(name, syslog_sink);
                logger->set_pattern(SYSLOG_PATTERN);
                break;
            }
            
            case LoggerMode::Console:
            default: {
                // Create colored console sink
                if (cfg.enable_colors && colorsSupported()) {
                    logger = spdlog::stdout_color_mt(name);
                } else {
                    logger = spdlog::stdout_logger_mt(name);
                }
                logger->set_pattern(cfg.pattern);
                break;
            }
        }
        
        logger->set_level(cfg.level);
        m_loggers[name] = logger;
        
        return logger;
    }
    
    /**
     * @brief Get global mutex (avoids static initialization order fiasco)
     */
    static std::mutex &getMutex() {
        static std::mutex mutex;
        return mutex;
    }
    
    /**
     * @brief Get global config (avoids static initialization order fiasco)
     */
    static LoggerConfig &getConfig() {
        static LoggerConfig config;
        return config;
    }
    
    static inline std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> m_loggers;
};

// Maintain backward compatibility with old Logger class name
using Logger = LoggerFactory;

} // namespace doip

// Logging macros (unchanged - they now use LoggerFactory)
#define LOG_DOIP_TRACE(...) doip::LoggerFactory::get()->trace(__VA_ARGS__)
#define LOG_DOIP_DEBUG(...) doip::LoggerFactory::get()->debug(__VA_ARGS__)
#define LOG_DOIP_INFO(...) doip::LoggerFactory::get()->info(__VA_ARGS__)
#define LOG_DOIP_WARN(...) doip::LoggerFactory::get()->warn(__VA_ARGS__)
#define LOG_DOIP_ERROR(...) doip::LoggerFactory::get()->error(__VA_ARGS__)
#define LOG_DOIP_CRITICAL(...) doip::LoggerFactory::get()->critical(__VA_ARGS__)

// UDP logging macros
#define LOG_UDP_TRACE(...) doip::LoggerFactory::getUdp()->trace(__VA_ARGS__)
#define LOG_UDP_DEBUG(...) doip::LoggerFactory::getUdp()->debug(__VA_ARGS__)
#define LOG_UDP_INFO(...) doip::LoggerFactory::getUdp()->info(__VA_ARGS__)
#define LOG_UDP_WARN(...) doip::LoggerFactory::getUdp()->warn(__VA_ARGS__)
#define LOG_UDP_ERROR(...) doip::LoggerFactory::getUdp()->error(__VA_ARGS__)
#define LOG_UDP_CRITICAL(...) doip::LoggerFactory::getUdp()->critical(__VA_ARGS__)

// TCP logging macros
#define LOG_TCP_TRACE(...) doip::LoggerFactory::getTcp()->trace(__VA_ARGS__)
#define LOG_TCP_DEBUG(...) doip::LoggerFactory::getTcp()->debug(__VA_ARGS__)
#define LOG_TCP_INFO(...) doip::LoggerFactory::getTcp()->info(__VA_ARGS__)
#define LOG_TCP_WARN(...) doip::LoggerFactory::getTcp()->warn(__VA_ARGS__)
#define LOG_TCP_ERROR(...) doip::LoggerFactory::getTcp()->error(__VA_ARGS__)
#define LOG_TCP_CRITICAL(...) doip::LoggerFactory::getTcp()->critical(__VA_ARGS__)

// Colored logging macros (only effective in console mode)
#define LOG_DOIP_SUCCESS(...) \
    doip::LoggerFactory::get()->info(std::string(doip::ansi::bold_green) + fmt::format(__VA_ARGS__) + doip::ansi::reset)

#define LOG_DOIP_ERROR_COLORED(...) \
    doip::LoggerFactory::get()->error(std::string(doip::ansi::bold_red) + fmt::format(__VA_ARGS__) + doip::ansi::reset)

#define LOG_DOIP_PROTOCOL(...) \
    doip::LoggerFactory::get()->info(std::string(doip::ansi::bold_blue) + fmt::format(__VA_ARGS__) + doip::ansi::reset)

#define LOG_DOIP_CONNECTION(...) \
    doip::LoggerFactory::get()->info(std::string(doip::ansi::bold_magenta) + fmt::format(__VA_ARGS__) + doip::ansi::reset)

#define LOG_DOIP_HIGHLIGHT(...) \
    doip::LoggerFactory::get()->info(std::string(doip::ansi::bold_cyan) + fmt::format(__VA_ARGS__) + doip::ansi::reset)

// Stream logging macros
#define LOG_DOIP_STREAM_INFO(obj, ...) LOG_DOIP_INFO(fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)))
#define LOG_DOIP_STREAM_DEBUG(obj, ...) LOG_DOIP_DEBUG(fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)))
#define LOG_DOIP_STREAM_WARN(obj, ...) LOG_DOIP_WARN(fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)))
#define LOG_DOIP_STREAM_ERROR(obj, ...) LOG_DOIP_ERROR(fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)))

// Colored stream logging macros
#define LOG_DOIP_STREAM_SUCCESS(obj, ...) \
    doip::LoggerFactory::get()->info(std::string(doip::ansi::bold_green) + fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)) + doip::ansi::reset)

#define LOG_DOIP_STREAM_PROTOCOL(obj, ...) \
    doip::LoggerFactory::get()->info(std::string(doip::ansi::bold_blue) + fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)) + doip::ansi::reset)

#define LOG_DOIP_STREAM_CONNECTION(obj, ...) \
    doip::LoggerFactory::get()->info(std::string(doip::ansi::bold_magenta) + fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)) + doip::ansi::reset)
