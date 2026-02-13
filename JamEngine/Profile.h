#pragma once
#include "Timer.h"

namespace Jam
{

#if defined(JAM_DEBUG) && !defined(JAM_PROFILE_ENABLED) && !defined(JAM_PROFILE_DISABLED)
#    define JAM_PROFILE_ENABLED
#endif

#define JAM_PROFILE_TIME_UNIT_SEC eSIPrefix::Sec
#define JAM_PROFILE_TIME_UNIT_MS  eSIPrefix::Milli
#define JAM_PROFILE_TIME_UNIT_US  eSIPrefix::Micro
#define JAM_PROFILE_TIME_UNIT_NS  eSIPrefix::Nano
#define JAM_PROFILE_TIME_UNIT     JAM_PROFILE_TIME_UNIT_SEC

#ifdef JAM_PROFILE_ENABLED

#    define JAM_FUNCTION_PROFILE()        Jam::ScopedTimer JAM_CONCAT(timer, __LINE__) { JAM_PRETTY_FUNCTION, JAM_PROFILE_TIME_UNIT };
#    define JAM_FUNCTION_MSG_PROFILE(...) Jam::ScopedTimer JAM_CONCAT(timer, __LINE__) { fmt::format(JAM_PRETTY_FUNCTION##__VA_ARGS__), JAM_PROFILE_TIME_UNIT };
#    define JAM_PROFILE(...)              Jam::ScopedTimer JAM_CONCAT(timer, __LINE__) { fmt::format(__VA_ARGS__), JAM_PROFILE_TIME_UNIT };

#else

#    define JAM_FUNCTION_PROFILE()        JAM_NOOP
#    define JAM_FUNCTION_MSG_PROFILE(...) JAM_NOOP
#    define JAM_PROFILE(...)              JAM_NOOP

#endif

}   // namespace Jam