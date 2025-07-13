#pragma once

#include <memory>
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
        JAM_ASSERT(s_coreLogger != nullptr, "Log system not initialized!");
        s_coreLogger->trace(_format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Info(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        JAM_ASSERT(s_coreLogger != nullptr, "Log system not initialized!");
        s_coreLogger->info(_format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Warn(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        JAM_ASSERT(s_coreLogger != nullptr, "Log system not initialized!");
        s_coreLogger->warn(_format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Error(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        JAM_ASSERT(s_coreLogger != nullptr, "Log system not initialized!");
        s_coreLogger->error(_format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Critical(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        JAM_ASSERT(s_coreLogger != nullptr, "Log system not initialized!");
        s_coreLogger->critical(_format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Debug(spdlog::format_string_t<Args...> _format, Args&&... args)
    {
        JAM_ASSERT(s_coreLogger != nullptr, "Log system not initialized!");
        s_coreLogger->debug(_format, std::forward<Args>(args)...);
    }

    template<typename T>
    static void Trace(const T& _str)
    {
        JAM_ASSERT(s_coreLogger != nullptr, "Log system not initialized!");
        s_coreLogger->trace(_str);
    }

    template<typename T>
    static void Info(const T& _str)
    {
        JAM_ASSERT(s_coreLogger != nullptr, "Log system not initialized!");
        s_coreLogger->info(_str);
    }

    template<typename T>
    static void Warn(const T& _str)
    {
        JAM_ASSERT(s_coreLogger != nullptr, "Log system not initialized!");
        s_coreLogger->warn(_str);
    }

    template<typename T>
    static void Error(const T& _str)
    {
        JAM_ASSERT(s_coreLogger != nullptr, "Log system not initialized!");
        s_coreLogger->error(_str);
    }

    template<typename T>
    static void Critical(const T& _str)
    {
        JAM_ASSERT(s_coreLogger != nullptr, "Log system not initialized!");
        s_coreLogger->critical(_str);
    }

    template<typename T>
    static void Debug(const T& _str)
    {
        JAM_ASSERT(s_coreLogger != nullptr, "Log system not initialized!");
        s_coreLogger->debug(_str);
    }

private:
    constexpr static std::string_view k_logDirectory = "logs/";
    constexpr static std::string_view k_logPattern   = "[%Y.%m.%d %H:%M:%S.%e] [%^%l%$] %v";

    inline static std::shared_ptr<spdlog::logger> s_coreLogger = nullptr;
};

}   // namespace jam