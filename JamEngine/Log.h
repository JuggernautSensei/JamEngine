#pragma once

#define FMT_UNICODE 0
#include <spdlog/spdlog.h>

/// @brief 쓰레드 안전한 로깅 시스템
/// 별도의 init 없이 사용할 수 있음
/// init 호출시 콘솔 싱크, 파일 싱크, 메모리 싱크가 추가됨
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

    /// @brief 파일 싱크 및 콘솔 싱크를 추가하고 비동기 로깅을 초기화합니다.
    static void Init();

    /// @brief 로거를 종료하고 모든 싱크를 제거합니다.
    static void Shutdown();

    /// @brief 싱크를 추가/제거합니다. (싱크 이름은 고유해야 합니다)
    static void AddSink(std::string_view _sinkName, const spdlog::sink_ptr& _sink);

    /// @brief 싱크를 제거합니다.
    static void RemoveSink(std::string_view _sinkName);

    /// @brief 싱크를 가져옵니다.
    [[nodiscard]] static const spdlog::sink_ptr& GetSink(std::string_view _sinkName);
    [[nodiscard]] static const spdlog::sink_ptr& GetConsoleSink();
    [[nodiscard]] static const spdlog::sink_ptr& GetFileSink();

    /// @brief 기본 로거에 로그를 남깁니다.
    template<typename... Args>
    static void Logging(const eLogLevel _level, spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(_level);
        spdlog::default_logger_raw()->log(level, _format, std::forward<Args>(args)...);
    }

    /// @brief 기본 로거에 로그를 남깁니다.
    template<typename T>
    static void Logging(const eLogLevel _level, const T& _msg)
    {
        spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(_level);
        spdlog::default_logger_raw()->log(level, _msg);
    }

    /// @brief Trace 레벨 로그
    template<typename... Args>
    static void Trace(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        Logging(eLogLevel::Trace, _format, std::forward<Args>(args)...);
    }

    /// @brief Debug 레벨 로그
    template<typename... Args>
    static void Debug(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        Logging(eLogLevel::Debug, _format, std::forward<Args>(args)...);
    }

    /// @brief Info 레벨 로그
    template<typename... Args>
    static void Info(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        Logging(eLogLevel::Info, _format, std::forward<Args>(args)...);
    }

    /// @brief Warn 레벨 로그
    template<typename... Args>
    static void Warn(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        Logging(eLogLevel::Warn, _format, std::forward<Args>(args)...);
    }

    /// @brief Error 레벨 로그
    template<typename... Args>
    static void Error(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        Logging(eLogLevel::Error, _format, std::forward<Args>(args)...);
    }

    /// @brief Critical 레벨 로그
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

    /// @brief 기본 로거의 로그 레벨을 설정합니다.
    static void SetLogLevel(eLogLevel _level);

    /// @brief 특정 싱크의 로그 레벨을 설정합니다.
    static void SetLogLevel(std::string_view _sinkName, eLogLevel _level);

    /// @brief 기본 로거의 로그 패턴을 설정합니다.
    static void SetLogPattern(std::string_view _pattern);

    /// @brief 특정 싱크의 로그 패턴을 설정합니다.
    static void SetLogPattern(std::string_view _sinkName, std::string_view _pattern);
  
};

}   // namespace jam

// 디버그에서만 동작하는 로그
#ifdef JAM_DEBUG
#    define JAM_DEBUG_LOG(...) jam::Log::Debug(__VA_ARGS__)
#else
#    define JAM_DEBUG_LOG(...) JAM_NOOP
#endif