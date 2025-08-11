#pragma once

#define FMT_UNICODE 0

#include "MemorySink.h"
#include <spdlog/spdlog.h>

namespace jam
{

class Log
{
public:
    static void Initialize();
    static void Shutdown();

    template<typename... Args>
    static void Trace(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        spdlog::trace(_format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Info(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        spdlog::info(_format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Warn(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        spdlog::warn(_format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Error(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        spdlog::error(_format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Critical(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        spdlog::critical(_format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Debug(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        spdlog::debug(_format, std::forward<Args>(args)...);
    }

    template<typename T>
    static void Trace(const T& _str)
    {
        spdlog::trace(_str);
    }

    template<typename T>
    static void Info(const T& _str)
    {
        spdlog::info(_str);
    }

    template<typename T>
    static void Warn(const T& _str)
    {
        spdlog::warn(_str);
    }

    template<typename T>
    static void Error(const T& _str)
    {
        spdlog::error(_str);
    }

    template<typename T>
    static void Critical(const T& _str)
    {
        spdlog::critical(_str);
    }

    template<typename T>
    static void Debug(const T& _str)
    {
        spdlog::debug(_str);
    }

    static const Ref<MemorySinkMT>& GetMemorySink()
    {
        JAM_ASSERT(s_memorySink != nullptr, "Memory sink is not initialized.");
        return s_memorySink;
    }

    constexpr static UInt32 k_memorySinkLogStoreCapacity = 4096;   // number of items in memory sink

private:
    inline static Ref<MemorySinkMT> s_memorySink = nullptr;

    constexpr static std::string_view k_logDirectory      = "logs/";
    constexpr static std::string_view k_defaultLogPattern = "[%Y.%m.%d %H:%M:%S] [%l] %n: %v";
    constexpr static std::string_view k_memoryLogPattern  = "[%Y.%m.%d %H:%M:%S] %n: %v";
};

}   // namespace jam