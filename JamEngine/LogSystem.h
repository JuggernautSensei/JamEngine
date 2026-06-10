#pragma once
#include <spdlog/spdlog.h>
#include "StringKeyedContainers.h"

namespace Jam
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

using SinkRef   = std::shared_ptr<spdlog::sinks::sink>;
using LoggerRef = std::shared_ptr<spdlog::logger>;

class LogChannel : public RefCounted
{
public:
    explicit LogChannel(const LoggerRef& _pLogger);

    void AddSink(StringView _name, const SinkRef& _pSink);
    void RemoveSink(StringView _name);
    void SetPattern(StringView _pattern) const;
    void SetLevel(eLogLevel _level) const;

    [[nodiscard]] const SinkRef&   GetSink(StringView _name) const;
    [[nodiscard]] const LoggerRef& GetLogger() const;

    template<typename... Args>
    void Log(const eLogLevel _level, spdlog::format_string_t<Args...> _format, Args&&... args) const
    {
        const spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(_level);
        m_pLogger->log(level, _format, std::forward<Args>(args)...);
    }

    void Log(const eLogLevel _level, const StringView _msg) const
    {
        const spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(_level);
        m_pLogger->log(level, { _msg.GetPtr(), static_cast<size_t>(_msg.GetLength()) });
    }

    template<typename... Args>
    void Log(const eLogLevel _level, spdlog::wformat_string_t<Args...> _format, Args&&... args) const
    {
        const spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(_level);
        m_pLogger->log(level, _format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Log(const eLogLevel _level, const WStringView _msg) const
    {
        const spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(_level);
        m_pLogger->log(level, { _msg.GetPtr(), static_cast<size_t>(_msg.GetLength()) });
    }

    void Flush() const
    {
        m_pLogger->flush();
    }

private:
    LoggerRef              m_pLogger = nullptr;
    StringHashMap<SinkRef> m_sinkMap = {};
};

class LogSystem
{
public:
    LogSystem(const LogSystem&)            = delete;
    LogSystem& operator=(const LogSystem&) = delete;
    LogSystem(LogSystem&&)                 = delete;
    LogSystem& operator=(LogSystem&&)      = delete;

    [[nodiscard]] static LogSystem& GetInstance();

    [[nodiscard]] const Ref<LogChannel>& GetCoreChannel() const;
    void                                 SetCoreChannel(const Ref<LogChannel>& _pChannel);

    template<typename... Args>
    void Log(const eLogLevel _level, spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        GetCoreChannel()->Log(_level, _format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Log(const eLogLevel _level, spdlog::wformat_string_t<Args...> _format, Args&&... args)
    {
        GetCoreChannel()->Log(_level, _format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Log(const Ref<LogChannel>& _pChannel, const eLogLevel _level, spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        _pChannel->Log(_level, _format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Log(const Ref<LogChannel>& _pChannel, const eLogLevel _level, spdlog::wformat_string_t<Args...> _format, Args&&... args)
    {
        _pChannel->Log(_level, _format, std::forward<Args>(args)...);
    }

    void Log(const eLogLevel _level, const StringView _msg) const
    {
        GetCoreChannel()->Log(_level, _msg);
    }

    void Log(const eLogLevel _level, const WStringView _msg) const
    {
        GetCoreChannel()->Log(_level, _msg);
    }

    void Log(const Ref<LogChannel>& _pChannel, const eLogLevel _level, const StringView _msg) const
    {
        _pChannel->Log(_level, _msg);
    }

    void Log(const Ref<LogChannel>& _pChannel, const eLogLevel _level, const WStringView _msg) const
    {
        _pChannel->Log(_level, _msg);
    }

    void Flush() const;
    void Flush(const Ref<LogChannel>& _pChannel) const;

private:
    LogSystem();
    ~LogSystem();

    static void CreateInstance();
    static void DestroyInstance();

    Ref<LogChannel> m_pCoreLogger = nullptr;

    inline static LogSystem* s_pInstance = nullptr;
    friend class Application;
};

[[nodiscard]] inline LogSystem& GetLogSystem()
{
    return LogSystem::GetInstance();
}

}   // namespace Jam

#define JAM_LOG_TRACE(...)    GetLogSystem().Log(Jam::eLogLevel::Trace, __VA_ARGS__)
#define JAM_LOG_INFO(...)     GetLogSystem().Log(Jam::eLogLevel::Info, __VA_ARGS__)
#define JAM_LOG_WARN(...)     GetLogSystem().Log(Jam::eLogLevel::Warn, __VA_ARGS__)
#define JAM_LOG_ERROR(...)    GetLogSystem().Log(Jam::eLogLevel::Error, __VA_ARGS__)
#define JAM_LOG_CRITICAL(...) GetLogSystem().Log(Jam::eLogLevel::Critical, __VA_ARGS__)

#define JAM_LOG_CHANNEL_TRACE(_channel, ...)    GetLogSystem().Log(_channel, Jam::eLogLevel::Trace, __VA_ARGS__)
#define JAM_LOG_CHANNEL_INFO(_channel, ...)     GetLogSystem().Log(_channel, Jam::eLogLevel::Info, __VA_ARGS__)
#define JAM_LOG_CHANNEL_WARN(_channel, ...)     GetLogSystem().Log(_channel, Jam::eLogLevel::Warn, __VA_ARGS__)
#define JAM_LOG_CHANNEL_ERROR(_channel, ...)    GetLogSystem().Log(_channel, Jam::eLogLevel::Error, __VA_ARGS__)
#define JAM_LOG_CHANNEL_CRITICAL(_channel, ...) GetLogSystem().Log(_channel, Jam::eLogLevel::Critical, __VA_ARGS__)

#ifdef JAM_DEBUG
#    define JAM_LOG_DEBUG(...)                   GetLogSystem().Log(Jam::eLogLevel::Debug, __VA_ARGS__)
#    define JAM_LOG_CHANNEL_DEBUG(_channel, ...) GetLogSystem().Log(_channel, Jam::eLogLevel::Debug, __VA_ARGS__)
#else
#    define JAM_LOG_DEBUG(...)                   JAM_NOOP
#    define JAM_LOG_CHANNEL_DEBUG(_channel, ...) JAM_NOOP
#endif