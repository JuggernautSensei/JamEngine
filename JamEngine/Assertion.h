// ReSharper disable CppClangTidyClangDiagnosticReservedIdentifier
// ReSharper disable CppClangTidyBugproneReservedIdentifier
#pragma once
#include <source_location>

namespace Jam::Details
{

void ReportCrash__(const std::source_location& _loc, const char* _msg);
void ReportHRESULTFailure_(const std::source_location& _loc, std::string_view _func, HRESULT _hr);

template<typename... TArgs>
void ReportCrash_(
    const std::source_location&        _loc,
    const fmt::format_string<TArgs...> _fmt,
    TArgs&&... _args)
{
    char buf[2048];
    const auto [end, _] = fmt::format_to_n(buf, sizeof(buf) - 1, _fmt, std::forward<TArgs>(_args)...);
    *end                = '\0';
    ReportCrash__(_loc, buf);
}

template<typename... TArgs>
void ReportAssertion_(
    const std::source_location&        _loc,
    const char*                        _condition,
    const fmt::format_string<TArgs...> _fmt,
    TArgs&&... _args)
{
    char buf[2048];
    const auto [e0, _]  = fmt::format_to_n(buf, sizeof(buf) - 1, "Assertion failed! Condition: {}\n", _condition);
    const auto [e1, __] = fmt::format_to_n(e0, sizeof(buf) - (e0 - buf) - 1, _fmt, std::forward<TArgs>(_args)...);
    *e1                 = '\0';
    ReportCrash__(_loc, buf);
}

}   // namespace Jam::Details

#if defined(JAM_DEBUG) && !defined(JAM_ENABLE_ASSERTS) && !defined(JAM_DISABLE_ASSERTS)
#    define JAM_ENABLE_ASSERTS
#endif

#ifdef JAM_ENABLE_ASSERTS

#    define JAM_ASSERT(_condition, ...)                                                                    \
        JAM_BEGIN_MACRO_BLOCK                                                                              \
        if (!std::is_constant_evaluated())                                                                 \
        {                                                                                                  \
            if (!(_condition))                                                                             \
            {                                                                                              \
                Jam::Details::ReportAssertion_(std::source_location::current(), #_condition, __VA_ARGS__); \
                std::terminate();                                                                          \
            }                                                                                              \
        }                                                                                                  \
        JAM_END_MACRO_BLOCK

#else
#    define JAM_ASSERT(...) JAM_NOOP
#endif

#define JAM_CRASH(_msg, ...)                                                        \
    JAM_BEGIN_MACRO_BLOCK                                                           \
    Jam::Details::ReportCrash_(std::source_location::current(), _msg, __VA_ARGS__); \
    std::terminate();                                                               \
    JAM_END_MACRO_BLOCK

#define JAM_UNREACHABLE JAM_CRASH("Unreachable code reached.")

#define JAM_CHECK_HRESULT(_func)                                                          \
    JAM_BEGIN_MACRO_BLOCK                                                                 \
    if (const HRESULT hr = (_func); FAILED(hr))                                           \
    {                                                                                     \
        Jam::Details::ReportHRESULTFailure_(std::source_location::current(), #_func, hr); \
        std::terminate();                                                                 \
    }                                                                                     \
    JAM_END_MACRO_BLOCK

#define JAM_CHECK_WIN32(_func)                                                                \
    JAM_BEGIN_MACRO_BLOCK                                                                     \
    if ((_func) == FALSE)                                                                     \
    {                                                                                         \
        Jam::Details::ReportHRESULTFailure_(                                                  \
            std::source_location::current(), #_func, ::HRESULT_FROM_WIN32(::GetLastError())); \
        std::terminate();                                                                     \
    }                                                                                         \
    JAM_END_MACRO_BLOCK