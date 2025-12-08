#pragma once

#ifdef JAM_COMPILER_MSVC
#    define JAM_PRETTY_FUNCTION __FUNCSIG__
#    define JAM_DEBUG_BREAK     __debugbreak()
#elif defined(JAM_COMPILER_CLANG)
#    define JAM_PRETTY_FUNCTION __PRETTY_FUNCTION__
#    define JAM_DEBUG_BREAK     __builtin_debugtrap()
#elif defined(JAM_COMPILER_GCC)
#    define JAM_PRETTY_FUNCTION __PRETTY_FUNCTION__
#    define JAM_DEBUG_BREAK     __builtin_trap()
#endif

#define JAM_NOOP __noop
#define JAM_UNUSED(_x) (void)_x

#define JAM_BEGIN_MACRO_BLOCK \
    do                        \
    {

#define JAM_END_MACRO_BLOCK \
    }                       \
    while (false)

#define JAM_CONCAT_(_a, _b) _a##_b
#define JAM_CONCAT(_a, _b)  JAM_CONCAT_(_a, _b)

#define JAM_STR_(x) #x
#define JAM_STR(x)  JAM_STR_(x)

#define JAM_STRINGIFY_MACRO(x) #x
#define JAM_EXPAND_MACRO(x)    x

