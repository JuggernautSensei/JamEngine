#pragma once

#include <source_location>

namespace jam::details
{

void AssertFail(const std::source_location& _loc, std::string_view _msg);

template<typename... Args>
void AssertFail(const std::source_location& _loc, const std::format_string<Args...> _fmt, Args&&... _args)
{
    AssertFail(_loc, std::format(_fmt, std::forward<Args>(_args)...));
}

void Crash(const std::source_location& _loc, std::string_view _msg);

template<typename... Args>
void Crash(const std::source_location& _loc, const std::format_string<Args...> _fmt, Args&&... _args)
{
    Crash(_loc, std::format(_fmt, std::forward<Args>(_args)...));
}

void CrashWithComHint(const std::source_location& _loc, const char* _func, HRESULT _hr);

}   // namespace jam::details

#if defined(JAM_DEBUG) && !defined(JAM_ENABLE_ASSERTS) && !defined(JAM_DISABLE_ASSERTS)
#    define JAM_ENABLE_ASSERTS
#endif

#ifdef JAM_ENABLE_ASSERTS
#    define JAM_ASSERT(_cond, ...)                                                  \
        JAM_BEGIN_MACRO_BLOCK                                                       \
        if (!(_cond))                                                               \
        {                                                                           \
            jam::details::AssertFail(std::source_location::current(), __VA_ARGS__); \
            JAM_DEBUG_BREAK;                                                        \
        }                                                                           \
        JAM_END_MACRO_BLOCK

#else
#    define JAM_ASSERT(...) JAM_NOOP
#endif

#define JAM_CRASH(...)                                                 \
    JAM_BEGIN_MACRO_BLOCK                                              \
    jam::details::Crash(std::source_location::current(), __VA_ARGS__); \
    std::abort();                                                      \
    JAM_END_MACRO_BLOCK

#define JAM_CHECK_COM(_func)                                                         \
    JAM_BEGIN_MACRO_BLOCK                                                            \
    if (HRESULT hr = (_func); FAILED(hr))                                            \
        jam::details::CrashWithComHint(std::source_location::current(), #_func, hr); \
    JAM_END_MACRO_BLOCK

namespace jam
{

// com error message helper
[[nodiscard]] std::string ErrorMessageFrom(HRESULT _hr);

}   // namespace jam