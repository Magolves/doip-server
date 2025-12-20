#pragma once

#include "AnsiColors.h"
#include <cstdlib>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h> // For fmt::streamed() support
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/syslog_sink.h>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>
#include <stdexcept>

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

// namespace std {
// template <typename T>
// std::ostream& operator<<(std::ostream& os, const std::optional<T>& opt) {
//     if (opt) {
//         os << *opt;
//     } else {
//         os << "<nullopt>";
//     }
//     return os;
// }
//}
#endif

namespace doip {

constexpr const char *DEFAULT_PATTERN = "[%H:%M:%S.%e] [%n] [%^%l%$] %v";

/**
 * @brief Pattern for short output without timestamp
 *
 */
constexpr const char *SHORT_PATTERN = "[%n] [%^%l%$] %v";

constexpr const char *SYSLOG_PATTERN = "[%n] %v";  // Syslog adds its own timestamp

/**
 * @brief Centralized logger for the DoIP library
 */
class Logger {
  public:
    static std::shared_ptr<spdlog::logger> get(const std::string &name = "doip", spdlog::level::level_enum level = spdlog::level::info) {
        static std::mutex mutex;
        std::lock_guard<std::mutex> lock(mutex);

        auto &loggers = m_loggers;
        if (auto it = loggers.find(name); it != loggers.end()) {
            return it->second;
        }

        std::shared_ptr<spdlog::logger> new_logger;
        if (use_syslog) {
            auto syslog_sink = std::make_shared<spdlog::sinks::syslog_sink_mt>(
                name,
                LOG_PID,    // Include PID in log messages
                LOG_DAEMON, // Facility
                true        // Enable syslog
            );
            new_logger = std::make_shared<spdlog::logger>(name, syslog_sink);
            new_logger->set_level(level);
            new_logger->set_pattern(SYSLOG_PATTERN);
        } else {
            // Avoid spdlog global registry: build logger manually with console sink
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_color_mode(spdlog::color_mode::automatic);
            new_logger = std::make_shared<spdlog::logger>(name, console_sink);
            new_logger->set_level(level);
            new_logger->set_pattern(DEFAULT_PATTERN);
        }

        loggers.emplace(name, new_logger);
        return new_logger;
    }

    static std::shared_ptr<spdlog::logger> getUdp() {
        return get("udp ");
    }

    static std::shared_ptr<spdlog::logger> getTcp() {
        return get("tcp ");
    }

    static void setLevel(spdlog::level::level_enum level) {
        // Apply to all existing loggers
        for (auto &pair : m_loggers) {
            if (pair.second) pair.second->set_level(level);
        }
    }

    static void setPattern(const std::string &pattern) {
        for (auto &pair : m_loggers) {
            if (pair.second) pair.second->set_pattern(pattern);
        }
    }

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

    static bool useSyslog() {
        return use_syslog;
    }

    static void setUseSyslog(bool use) {
        if (!m_loggers.empty()) {
            throw std::runtime_error("Cannot change syslog setting after loggers have been created");
        }
        use_syslog = use;
    }

    // Explicit shutdown to avoid sanitizer leak reports and ensure safe teardown
    static void shutdown() {
        for (auto &pair : m_loggers) {
            if (pair.second) {
                pair.second->flush();
            }
        }
        m_loggers.clear();
        // Also shutdown spdlog registry resources (safe even if unused)
        spdlog::shutdown();
    }

  private:
    static bool use_syslog;
    static std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> m_loggers;
};

} // namespace doip

// Logging macros
#define LOG_DOIP_TRACE(...) doip::Logger::get()->trace(__VA_ARGS__)
#define LOG_DOIP_DEBUG(...) doip::Logger::get()->debug(__VA_ARGS__)
#define LOG_DOIP_INFO(...) doip::Logger::get()->info(__VA_ARGS__)
#define LOG_DOIP_WARN(...) doip::Logger::get()->warn(__VA_ARGS__)
#define LOG_DOIP_ERROR(...) doip::Logger::get()->error(__VA_ARGS__)
#define LOG_DOIP_CRITICAL(...) doip::Logger::get()->critical(__VA_ARGS__)

// Logging macros for UDP socket
#define LOG_UDP_TRACE(...) doip::Logger::getUdp()->trace(__VA_ARGS__)
#define LOG_UDP_DEBUG(...) doip::Logger::getUdp()->debug(__VA_ARGS__)
#define LOG_UDP_INFO(...) doip::Logger::getUdp()->info(__VA_ARGS__)
#define LOG_UDP_WARN(...) doip::Logger::getUdp()->warn(__VA_ARGS__)
#define LOG_UDP_ERROR(...) doip::Logger::getUdp()->error(__VA_ARGS__)
#define LOG_UDP_CRITICAL(...) doip::Logger::getUdp()->critical(__VA_ARGS__)

// Logging macros for TCP socket
#define LOG_TCP_TRACE(...) doip::Logger::getTcp()->trace(__VA_ARGS__)
#define LOG_TCP_DEBUG(...) doip::Logger::getTcp()->debug(__VA_ARGS__)
#define LOG_TCP_INFO(...) doip::Logger::getTcp()->info(__VA_ARGS__)
#define LOG_TCP_WARN(...) doip::Logger::getTcp()->warn(__VA_ARGS__)
#define LOG_TCP_ERROR(...) doip::Logger::getTcp()->error(__VA_ARGS__)
#define LOG_TCP_CRITICAL(...) doip::Logger::getTcp()->critical(__VA_ARGS__)

// Colored logging macros
#define LOG_DOIP_SUCCESS(...) \
    doip::Logger::get()->info(std::string(doip::ansi::bold_green) + fmt::format(__VA_ARGS__) + doip::ansi::reset)

#define LOG_DOIP_ERROR_COLORED(...) \
    doip::Logger::get()->error(std::string(doip::ansi::bold_red) + fmt::format(__VA_ARGS__) + doip::ansi::reset)

#define LOG_DOIP_PROTOCOL(...) \
    doip::Logger::get()->info(std::string(doip::ansi::bold_blue) + fmt::format(__VA_ARGS__) + doip::ansi::reset)

#define LOG_DOIP_CONNECTION(...) \
    doip::Logger::get()->info(std::string(doip::ansi::bold_magenta) + fmt::format(__VA_ARGS__) + doip::ansi::reset)

#define LOG_DOIP_HIGHLIGHT(...) \
    doip::Logger::get()->info(std::string(doip::ansi::bold_cyan) + fmt::format(__VA_ARGS__) + doip::ansi::reset)

// Convenience macros for types with stream operators (using fmt::streamed)
// These automatically wrap arguments with fmt::streamed() for seamless logging of DoIP types
#define LOG_DOIP_STREAM_INFO(obj, ...) LOG_DOIP_INFO(fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)))
#define LOG_DOIP_STREAM_DEBUG(obj, ...) LOG_DOIP_DEBUG(fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)))
#define LOG_DOIP_STREAM_WARN(obj, ...) LOG_DOIP_WARN(fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)))
#define LOG_DOIP_STREAM_ERROR(obj, ...) LOG_DOIP_ERROR(fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)))

// Colored stream logging macros for DoIP types
#define LOG_DOIP_STREAM_SUCCESS(obj, ...) \
    doip::Logger::get()->info(std::string(doip::ansi::bold_green) + fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)) + doip::ansi::reset)

#define LOG_DOIP_STREAM_PROTOCOL(obj, ...) \
    doip::Logger::get()->info(std::string(doip::ansi::bold_blue) + fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)) + doip::ansi::reset)

#define LOG_DOIP_STREAM_CONNECTION(obj, ...) \
    doip::Logger::get()->info(std::string(doip::ansi::bold_magenta) + fmt::format("{} " __VA_ARGS__, fmt::streamed(obj)) + doip::ansi::reset)
