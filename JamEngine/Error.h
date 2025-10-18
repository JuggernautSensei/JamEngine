#pragma once
#include "Macros.h"
#include <source_location>

namespace jam
{

namespace details
{
    void ReportAssertFailed(const std::source_location& _loc, std::string_view _msg);
    void ReportCrash(const std::source_location& _loc, std::string_view _msg);

    template<typename... Args>
    void ReportAssertFailed(const std::source_location& _loc, const std::string_view _fmt, Args&&... _args)
    {
        ReportAssertFailed(_loc, std::vformat(_fmt, std::make_format_args(_args...)));
    }

    template<typename... Args>
    void ReportCrash(const std::source_location& _loc, const std::string_view _fmt, Args&&... _args)
    {
        ReportCrash(_loc, std::vformat(_fmt, std::make_format_args(_args...)));
    }

    JAM_NODISCARD std::string GetComErrorMessage(HRESULT _hr);

}   // namespace details

JAM_NODISCARD std::string_view GetErrorMessage();
void                           SetErrorMessage(std::string_view _msg);

template<typename... Args>
void SetErrorMessage(const std::format_string<Args...>& _fmt, Args&&... _args)
{
    SetErrorMessage(std::format(_fmt, std::forward<Args>(_args)...));
}

}   // namespace jam

#if defined(JAM_DEBUG) && !defined(JAM_ENABLE_ASSERTS) && !defined(JAM_DISABLE_ASSERTS)
#    define JAM_ENABLE_ASSERTS
#endif

#ifdef JAM_ENABLE_ASSERTS
#    define JAM_ASSERT(_cond, ...)                                                          \
        JAM_BEGIN_MACRO_BLOCK                                                               \
        if (!(_cond))                                                                       \
        {                                                                                   \
            jam::details::ReportAssertFailed(std::source_location::current(), __VA_ARGS__); \
            JAM_DEBUG_BREAK;                                                                \
        }                                                                                   \
        JAM_END_MACRO_BLOCK

#else
#    define JAM_ASSERT(...) JAM_NOOP
#endif

#define JAM_CRASH(...)                                                       \
    JAM_BEGIN_MACRO_BLOCK                                                    \
    jam::details::ReportCrash(std::source_location::current(), __VA_ARGS__); \
    std::abort();                                                            \
    JAM_END_MACRO_BLOCK

#define JAM_CHECK_COM(_func)                                                                                                                                            \
    JAM_BEGIN_MACRO_BLOCK                                                                                                                                               \
    if (HRESULT hr = (_func); FAILED(hr))                                                                                                                               \
    {                                                                                                                                                                   \
        jam::details::ReportCrash(std::source_location::current(), "Failed {}!\nError code: {:#X}\nDescription: {}", #_func, hr, jam::details::GetComErrorMessage(hr)); \
    }                                                                                                                                                                   \
    JAM_END_MACRO_BLOCK