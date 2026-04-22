// ReSharper disable CppClangTidyModernizeMacroToEnum
#pragma once

// Architecture
#if defined(__x86_64__)       \
    || defined(_M_X64)        \
    || defined(__aarch64__)   \
    || defined(__64BIT__)     \
    || defined(__mips64)      \
    || defined(__powerpc64__) \
    || defined(__ppc64__)     \
    || defined(__LP64__)
#    define JAM_ARCH_64BIT
#    define JAM_ARCH_NAME "64-bit"
#else
#    define JAM_ARCH_32BIT
#    define JAM_ARCH_NAME "32-bit"
#endif

// text encoding
#if defined(_UNICODE) || defined(__UNICODE) || defined(UNICODE)
#    define JAM_TEXT_ENCODING_UNICODE
#    define JAM_TEXT_ENCODING_NAME "Unicode"
#else
#    define JAM_TEXT_ENCODING_MULTIBYTE
#    define JAM_TEXT_ENCODING_NAME "Multibyte"
#endif

// Compiler
#ifdef _MSC_VER
#    define JAM_COMPILER_MSVC
#    define JAM_COMPILER_NAME "MSVC"
#elif defined(__clang__)
#    define JAM_COMPILER_CLANG
#    define JAM_COMPILER_NAME "Clang"
#elif defined(__GNUC__) || defined(__GNUG__)
#    define JAM_COMPILER_GCC
#    define JAM_COMPILER_NAME "GCC"
#else
#    error "Unsupported compiler"
#endif

// C++ Language
#ifdef __cplusplus
#    define JAM_CPP
#else
#    error "Not a C++ compiler"
#endif

// C++ Language Version
#ifdef __cplusplus
#    if defined(_MSVC_LANG) && _MSVC_LANG > __cplusplus
#        define JAM_CPP_LANG _MSVC_LANG
#    else
#        define JAM_CPP_LANG __cplusplus
#    endif
#else
#    define JAM_CPP_LANG 0L
#endif

#if JAM_CPP_LANG < 202002L   // C++17 이하
#    error "C++20 or higher is required"
#elif JAM_CPP_LANG < 202302L   // C++20
#    define JAM_CPP20
#    define JAM_CPP_NAME "C++20"
#elif JAM_CPP_LANG < 202402L   // C++23
#    define JAM_CPP23
#    define JAM_CPP_NAME "C++23"
#elif JAM_CPP_LANG < 202502L   // C++26
#    define JAM_CPP26
#    define JAM_CPP_NAME "C++26"
#else
#    error "Future C++ version is not supported"
#endif

// cpu
#if defined(__arm__)        \
    || defined(__aarch64__) \
    || defined(_M_ARM)
#    undef JAM_CPU_ARM
#    define JAM_CPU_ARM         1
#    define JAM_CACHE_LINE_SIZE 64
#    define JAM_CPU_NAME        "ARM"
#elif defined(__MIPSEL__)      \
    || defined(__mips_isa_rev) \
    || defined(__mips64)
#    undef JAM_CPU_MIPS
#    define JAM_CPU_MIPS        1
#    define JAM_CACHE_LINE_SIZE 64
#    define JAM_CPU_NAME        "MIPS"
#elif defined(_M_PPC)       \
    || defined(__powerpc__) \
    || defined(__powerpc64__)
#    undef JAM_CPU_PPC
#    define JAM_CPU_PPC         1
#    define JAM_CACHE_LINE_SIZE 128
#    define JAM_CPU_NAME        "PPC"
#elif defined(__riscv)    \
    || defined(__riscv__) \
    || defined(RISCVEL)
#    undef JAM_CPU_RISCV
#    define JAM_CPU_RISCV       1
#    define JAM_CACHE_LINE_SIZE 64
#    define JAM_CPU_NAME        "RISC-V"
#elif defined(_M_IX86)   \
    || defined(_M_X64)   \
    || defined(__i386__) \
    || defined(__x86_64__)
#    undef JAM_CPU_X86
#    define JAM_CPU_X86         1
#    define JAM_CACHE_LINE_SIZE 64
#    define JAM_CPU_NAME        "x86"
#else   // PNaCl doesn't have CPU defined.
#    undef JAM_CPU_JIT
#    define JAM_CPU_JIT         1
#    define JAM_CACHE_LINE_SIZE 64
#    define JAM_CPU_NAME        "JIT"
#endif   //

// platform
#if defined(_WIN32) || defined(_WIN64)
#    define JAM_PLATFORM_WINDOWS
#    ifdef _WIN64
#        define JAM_PLATFORM_WINDOWS_64
#        define JAM_PLATFORM_NAME "Windows 64-bit"
#    else
#        define JAM_PLATFORM_WINDOWS_32
#        define JAM_PLATFORM_NAME "Windows 32-bit"
#    endif
#else
#    error "Unsupported platform"
#endif

// gfx api
#define JAM_GFX_API_D3D11
#define JAM_GFX_API_NAME "Direct3D 11"

#if defined(_DEBUG) || defined(DEBUG)
#    define JAM_DEBUG
#endif

#ifdef JAM_DEBUG
#    define JAM_DEBUG_ENABLED 1
#else
#    define JAM_DEBUG_ENABLED 0
#endif

#define JAM_ENGINE_NAME "Jam Engine"

#define JAM_VERSION_TO_STRING_HELPER(_x) #_x
#define JAM_VERSION_TO_STRING(_x)        JAM_VERSION_TO_STRING_HELPER(_x)

#define JAM_VERSION_STRING(_major, _minor, _patch) \
    JAM_VERSION_TO_STRING(_major)                  \
    "." JAM_VERSION_TO_STRING(_minor) "." JAM_VERSION_TO_STRING(_patch)

#define JAM_VERSION_MAJOR 0
#define JAM_VERSION_MINOR 1
#define JAM_VERSION_PATCH 0
#define JAM_VERSION_NAME  JAM_VERSION_STRING(JAM_VERSION_MAJOR, JAM_VERSION_MINOR, JAM_VERSION_PATCH)

#if defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(__SSE2__) || defined(_M_X64)
#    define JAM_SIMD_SSE
#endif
