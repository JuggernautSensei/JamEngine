#pragma once
#include "Timer.h"

namespace jam
{

#define JAM_PROFILE_ENABLED

#ifdef JAM_PROFILE_ENABLED

#    define JAM_PROFILE_SCOPE_NS(_name)  jam::ScopedTimer scopedTimer##__LINE__(_name, eTimeUnit::Nano)
#    define JAM_PROFILE_SCOPE_US(_name)  jam::ScopedTimer scopedTimer##__LINE__(_name, eTimeUnit::Micro)
#    define JAM_PROFILE_SCOPE_MS(_name)  jam::ScopedTimer scopedTimer##__LINE__(_name, eTimeUnit::Milli)
#    define JAM_PROFILE_SCOPE_SEC(_name) jam::ScopedTimer scopedTimer##__LINE__(_name, eTimeUnit::Sec)

#else

#    define JAM_PROFILE_SCOPE_NS(_name)  JAM_NOOP
#    define JAM_PROFILE_SCOPE_US(_name)  JAM_NOOP
#    define JAM_PROFILE_SCOPE_MS(_name)  JAM_NOOP
#    define JAM_PROFILE_SCOPE_SEC(_name) JAM_NOOP

#endif

}   // namespace jam