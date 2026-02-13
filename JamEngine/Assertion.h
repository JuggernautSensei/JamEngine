#pragma once
#include <source_location>

namespace Jam::Details
{

void ReportCrash(const std::source_location& _loc, std::string_view _msg);
void ReportAssertion(const std::source_location& _loc, std::string_view _msg);
void ReportHRESULTFailure(const std::source_location& _loc, std::string_view _msg, HRESULT _hr);

template<typename... TArgs>
void ReportCrash(const std::source_location& _loc, fmt::format_string<TArgs...> _fmt, TArgs&&... _args)
{
    char buf[2048];
    const auto [end, _] = fmt::format_to_n(buf, sizeof(buf) - 1, _fmt, std::forward<TArgs>(_args)...);
    *end                = '\0';
    const size_t len    = end - buf;
    ReportCrash(_loc, { buf, len });
}

template<typename... TArgs>
void ReportAssertion(const std::source_location& _loc, fmt::format_string<TArgs...> _fmt, TArgs&&... _args)
{
    char buf[2048];
    const auto [end, _] = fmt::format_to_n(buf, sizeof(buf) - 1, _fmt, std::forward<TArgs>(_args)...);
    *end                = '\0';
    const size_t len    = end - buf;
    ReportAssertion(_loc, { buf, len });
}

}   // namespace Jam::Details

#if defined(JAM_DEBUG) && !defined(JAM_ENABLE_ASSERTS) && !defined(JAM_DISABLE_ASSERTS)
#    define JAM_ENABLE_ASSERTS
#endif

#ifdef JAM_ENABLE_ASSERTS

#    define JAM_ASSERT(_condition, ...)                                                                                       \
        JAM_BEGIN_MACRO_BLOCK                                                                                                 \
        if (!std::is_constant_evaluated())                                                                                    \
        {                                                                                                                     \
            if (!(_condition))                                                                                                \
            {                                                                                                                 \
                Jam::Details::ReportAssertion(std::source_location::current(), "Assertion failed! " #_condition __VA_ARGS__); \
                JAM_DEBUG_BREAK;                                                                                              \
            }                                                                                                                 \
        }                                                                                                                     \
        JAM_END_MACRO_BLOCK

#else
#    define JAM_ASSERT(...) JAM_NOOP
#endif

#define JAM_CRASH(_msg, ...)                                                       \
    JAM_BEGIN_MACRO_BLOCK                                                          \
    Jam::Details::ReportCrash(std::source_location::current(), _msg, __VA_ARGS__); \
    std::abort();                                                                  \
    JAM_END_MACRO_BLOCK

#define JAM_UNREACHABLE JAM_CRASH("Unreachable code reached.")

#define JAM_CHECK_HRESULT(_func)                                                                               \
    JAM_BEGIN_MACRO_BLOCK                                                                                      \
    if (const HRESULT hr = (_func); FAILED(hr))                                                                \
    {                                                                                                          \
        Jam::Details::ReportHRESULTFailure(std::source_location::current(), "HRESULT failure in " #_func, hr); \
        std::abort();                                                                                          \
    }                                                                                                          \
    JAM_END_MACRO_BLOCK

#define JAM_CHECK_WIN32(_func)                                                                                \
    JAM_BEGIN_MACRO_BLOCK                                                                                     \
    if ((_func) == FALSE)                                                                                     \
    {                                                                                                         \
        Jam::Details::ReportHRESULTFailure(                                                                   \
            std::source_location::current(), "Win32 failure in " #_func, HRESULT_FROM_WIN32(GetLastError())); \
        std::abort();                                                                                         \
    }                                                                                                         \
    JAM_END_MACRO_BLOCK