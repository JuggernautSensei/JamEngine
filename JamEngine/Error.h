#pragma once
#include <source_location>

namespace jam
{

namespace details
{

    void Assert(std::string_view _msg, const std::source_location& _loc = std::source_location::current());
    void Crash(std::string_view _msg, const std::source_location& _loc = std::source_location::current());
    void CrashWithCom(std::string_view _msg, HRESULT _hr, const std::source_location& _loc = std::source_location::current());

}   // namespace details

void          SetErrorString(std::string_view _msg);
JAM_NODISCARD std::string_view GetErrorString();

template<typename... Args>
void SetErrorString(std::string_view _format, Args&&... args)
{
    SetErrorString(std::format(_format, std::forward<Args>(args)...));
}

}   // namespace jam

#if defined(JAM_DEBUG) && !defined(JAM_ENABLE_ASSERTS) && !defined(JAM_DISABLE_ASSERTS)
#    define JAM_ENABLE_ASSERTS
#endif

#ifdef JAM_ENABLE_ASSERTS
#    define JAM_ASSERT_(_condition, ...) (void)(!!(_condition) || jam::details::Assert(std::format(__VA_ARGS__)))
#else
#    define JAM_ASSERT_(_condition, _msg) JAM_NOOP
#endif

#define JAM_ASSERT(...) JAM_ASSERT_(__VA_ARGS__)

#define JAM_CRASH_(...) jam::details::ReportCrash(std::format(__VA_ARGS__))
#define JAM_CRASH(...) JAM_CRASH_(__VA_ARGS__)

#define JAM_CHECK_COM(_func)                                \
    JAM_BEGIN_MACRO_BLOCK                                   \
    if (HRESULT hr = (_func); FAILED(hr))                   \
        jam::details::ReportComCrash("Error: " #_func, hr); \
        jam::details::ReportComCrash("Error: " #_func, hr); \
    JAM_END_MACRO_BLOCK