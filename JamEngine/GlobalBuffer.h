#pragma once
#include "FixedString.h"

namespace jam
{

// global error buffer
class GlobalBuffer
{
public:
    // for last error messages
    static void SetLastError(const StringView _msg)
    {
        s_errorBuffer.Set(_msg.GetPtr(), _msg.GetLength());
    }

    template<typename... Args>
    static void SetLastError(const std::format_string<Args...> _fmt, Args&&... _args)
    {
        s_errorBuffer.Format(_fmt, _args...);
    }

    [[nodiscard]] static StringView GetLastError()
    {
        return s_errorBuffer.ToStringView();
    }

    // temporary string formatting (no heap allocation)
    template<typename... Args>
    [[nodiscard]] static StringView FormatTmp(const std::format_string<Args...> _fmt, Args&&... _args)
    {
        s_tmpBuffer.Format(_fmt, std::forward<Args>(_args)...);
        return s_tmpBuffer.ToStringView();
    }

    [[nodiscard]] static StringView PrintfTmp(const char* _fmt, ...)
    {
        va_list args;
        va_start(args, _fmt);
        s_tmpBuffer.VPrintf(_fmt, args);
        va_end(args);
        return s_tmpBuffer.ToStringView();
    }

private:
    static constexpr Int32 k_bufferCap = 2048;

    static thread_local FixedString<k_bufferCap> s_errorBuffer;
    static thread_local FixedString<k_bufferCap> s_tmpBuffer;
};

}   // namespace jam