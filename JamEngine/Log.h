#pragma once

#define FMT_UNICODE 0
#include <spdlog/spdlog.h>

namespace jam
{

enum class eLogLevel : Int8
{
    Trace    = spdlog::level::trace,
    Debug    = spdlog::level::debug,
    Info     = spdlog::level::info,
    Warn     = spdlog::level::warn,
    Error    = spdlog::level::err,
    Critical = spdlog::level::critical,
    Off      = spdlog::level::off
};

class Log
{
public:
    static void Init();
    static void Shutdown();

    static void AddSink(std::string_view _sinkName, const spdlog::sink_ptr& _sink);
    static void RemoveSink(std::string_view _sinkName);

    [[nodiscard]] static const spdlog::sink_ptr& GetSink(std::string_view _sinkName);
    [[nodiscard]] static const spdlog::sink_ptr& GetConsoleSink();
    [[nodiscard]] static const spdlog::sink_ptr& GetFileSink();

    template<typename... Args>
    static void Logging(const eLogLevel _level, spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(_level);
        spdlog::default_logger_raw()->log(level, _format, std::forward<Args>(args)...);
    }

    template<typename T>
    static void Logging(const eLogLevel _level, const T& _msg)
    {
        spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(_level);
        spdlog::default_logger_raw()->log(level, _msg);
    }

    template<typename... Args>
    static void Trace(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        Logging(eLogLevel::Trace, _format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Debug(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        Logging(eLogLevel::Debug, _format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Info(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        Logging(eLogLevel::Info, _format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Warn(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        Logging(eLogLevel::Warn, _format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Error(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        Logging(eLogLevel::Error, _format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Critical(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        Logging(eLogLevel::Critical, _format, std::forward<Args>(args)...);
    }

    template<typename T>
    static void Trace(const T& _msg)
    {
        Logging(eLogLevel::Trace, _msg);
    }

    template<typename T>
    static void Debug(const T& _msg)
    {
        Logging(eLogLevel::Debug, _msg);
    }

    template<typename T>
    static void Info(const T& _msg)
    {
        Logging(eLogLevel::Info, _msg);
    }

    template<typename T>
    static void Warn(const T& _msg)
    {
        Logging(eLogLevel::Warn, _msg);
    }

    template<typename T>
    static void Error(const T& _msg)
    {
        Logging(eLogLevel::Error, _msg);
    }

    template<typename T>
    static void Critical(const T& _msg)
    {
        Logging(eLogLevel::Critical, _msg);
    }

    static void SetLogLevel(eLogLevel _level);
    static void SetLogLevel(std::string_view _sinkName, eLogLevel _level);

    static void SetLogPattern(std::string_view _pattern);
    static void SetLogPattern(std::string_view _sinkName, std::string_view _pattern);
};

}   // namespace jam

// 디버그에서만 동작하는 로그
#ifdef JAM_DEBUG
#    define JAM_DEBUG_LOG(...) jam::Log::Debug(__VA_ARGS__)
#else
#    define JAM_DEBUG_LOG(...) JAM_NOOP
#endif