#pragma once
#include "Timer.h"

namespace jam
{

#define JAM_PROFILE_ENABLED

#ifdef JAM_PROFILE_ENABLED

#    define JAM_PROFILE_SCOPE_NS(...)  jam::ScopedTimer JAM_CONCAT(scopedTimer, __LINE__)(std::format(__VA_ARGS__), eTimeUnit::Nano)
#    define JAM_PROFILE_SCOPE_US(...)  jam::ScopedTimer JAM_CONCAT(scopedTimer, __LINE__)(std::format(__VA_ARGS__), eTimeUnit::Micro)
#    define JAM_PROFILE_SCOPE_MS(...)  jam::ScopedTimer JAM_CONCAT(scopedTimer, __LINE__)(std::format(__VA_ARGS__), eTimeUnit::Milli)
#    define JAM_PROFILE_SCOPE_SEC(...) jam::ScopedTimer JAM_CONCAT(scopedTimer, __LINE__)(std::format(__VA_ARGS__), eTimeUnit::Sec)

#else

#    define JAM_PROFILE_SCOPE_NS(...)  JAM_NOOP
#    define JAM_PROFILE_SCOPE_US(...)  JAM_NOOP
#    define JAM_PROFILE_SCOPE_MS(...)  JAM_NOOP
#    define JAM_PROFILE_SCOPE_SEC(...) JAM_NOOP

#endif

}   // namespace jam