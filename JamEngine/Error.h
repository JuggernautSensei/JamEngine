#pragma once
#include <source_location>

namespace jam::detail
{

std::string CreateErrorMessage(std::string_view _msg, const std::source_location& _loc = std::source_location::current());

void ReportCrash(std::string_view _msg, const std::source_location& = std::source_location::current());
void ReportError(std::string_view _msg, const std::source_location& = std::source_location::current());

}   // namespace jam::detail

#ifdef _DEBUG
#    define JAM_DEBUG_BREAK __debugbreak()
#else
#    define JAM_DEBUG_BREAK __noop
#endif

#define JAM_CRASH_IMPL(msg_)            \
    do                                  \
    {                                   \
        jam::detail::ReportCrash(msg_); \
        std::terminate();               \
    } while (false)

#define JAM_ERROR_IMPL(msg_)            \
    do                                  \
    {                                   \
        jam::detail::ReportError(msg_); \
        JAM_DEBUG_BREAK;                \
    } while (false)

#ifdef _DEBUG
#    define JAM_ASSERT_IMPL(cond_, msg_) \
        do                               \
        {                                \
            if (!(cond_))                \
            {                            \
                JAM_ERROR_IMPL(msg_);    \
            }                            \
        } while (false)
#else
#    define JAM_ASSERT_IMPL __noop
#endif

#define JAM_CRASH(...)         JAM_CRASH_IMPL(std::format(__VA_ARGS__))
#define JAM_ERROR(...)         JAM_ERROR_IMPL(std::format(__VA_ARGS__))
#define JAM_ASSERT(cond_, ...) JAM_ASSERT_IMPL(cond_, std::format(__VA_ARGS__))
