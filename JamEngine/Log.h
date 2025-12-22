#pragma once

#define FMT_UNICODE 0
#include <spdlog/spdlog.h>

namespace jam
{

enum class eLogLevel
{
    Trace    = spdlog::level::trace,
    Debug    = spdlog::level::debug,
    Info     = spdlog::level::info,
    Warn     = spdlog::level::warn,
    Error    = spdlog::level::err,
    Critical = spdlog::level::critical,
    Off      = spdlog::level::off
};

using SinkPtr = spdlog::sink_ptr;

class Log
{
public:
    static void Init();
    static void Shutdown();

    static void AddSink(StringView _name, const spdlog::sink_ptr& _pSink);
    static void RemoveSink(StringView _name);

    [[nodiscard]] static const SinkPtr& GetSink(StringView _name);
    [[nodiscard]] static const SinkPtr& GetConsoleSink();
    [[nodiscard]] static const SinkPtr& GetFileSink();

    template<typename... Args>
    static void Logging(const eLogLevel _level, spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(_level);
        spdlog::default_logger_raw()->log(level, _format, args...);
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
        Logging(eLogLevel::Trace, _format, args...);
    }

    template<typename... Args>
    static void Debug(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        Logging(eLogLevel::Debug, _format, args...);
    }

    template<typename... Args>
    static void Info(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        Logging(eLogLevel::Info, _format, args...);
    }

    template<typename... Args>
    static void Warn(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        Logging(eLogLevel::Warn, _format, args...);
    }

    template<typename... Args>
    static void Error(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        Logging(eLogLevel::Error, _format, args...);
    }

    template<typename... Args>
    static void Critical(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        Logging(eLogLevel::Critical, _format, args...);
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
    static void SetLogLevel(StringView _name, eLogLevel _level);

    static void SetLogPattern(StringView _pattern);
    static void SetLogPattern(StringView _name, StringView _pattern);
};

}   // namespace jam

// 디버그에서만 동작하는 로그
#ifdef JAM_DEBUG
#    define JAM_DEBUG_LOG(...) jam::Log::Debug(__VA_ARGS__)
#else
#    define JAM_DEBUG_LOG(...) JAM_NOOP
#endif