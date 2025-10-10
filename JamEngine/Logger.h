#pragma once

#define FMT_UNICODE 0
#include "MemorySink.h"

/// @brief 쓰레드 안전한 로깅 시스템
/// 별도의 init 없이 사용할 수 있음
/// init 호출시 콘솔 싱크, 파일 싱크, 메모리 싱크가 추가됨
namespace jam::logger
{

void Init();
void Shutdown();

template<typename... Args>
void Trace(spdlog::format_string_t<Args...> _format, Args&&... args)
{
    spdlog::trace(_format, std::forward<Args>(args)...);
}

template<typename... Args>
void Info(spdlog::format_string_t<Args...> _format, Args&&... args)
{
    spdlog::info(_format, std::forward<Args>(args)...);
}

template<typename... Args>
void Warn(spdlog::format_string_t<Args...> _format, Args&&... args)
{
    spdlog::warn(_format, std::forward<Args>(args)...);
}

template<typename... Args>
void Error(spdlog::format_string_t<Args...> _format, Args&&... args)
{
    spdlog::error(_format, std::forward<Args>(args)...);
}

template<typename... Args>
void Critical(spdlog::format_string_t<Args...> _format, Args&&... args)
{
    spdlog::critical(_format, std::forward<Args>(args)...);
}

template<typename T>
void Trace(const T& _str)
{
    spdlog::trace(_str);
}

template<typename T>
void Info(const T& _str)
{
    spdlog::info(_str);
}

template<typename T>
void Warn(const T& _str)
{
    spdlog::warn(_str);
}

template<typename T>
void Error(const T& _str)
{
    spdlog::error(_str);
}

template<typename T>
void Critical(const T& _str)
{
    spdlog::critical(_str);
}

void SetLogLevel(eLogLevel _level);

void Flush();

/// memory sink access
JAM_NODISCARD MemorySinkMT::ContainerT::const_iterator GetMemoryLogBeginIter();
JAM_NODISCARD MemorySinkMT::ContainerT::const_iterator GetMemoryLogEndIter();
JAM_NODISCARD size_t                                   GetMemoryLogCount();
JAM_NODISCARD void                                     ClearMemoryLog();

}   // namespace jam::log

// 디버그에서만 동작하는 로그
#ifdef JAM_DEBUG
#    define JAM_LOG_DEBUG(...) jam::logger::Trace(__VA_ARGS__)
#else
#    define JAM_LOG_DEBUG(...) JAM_NOOP
#endif