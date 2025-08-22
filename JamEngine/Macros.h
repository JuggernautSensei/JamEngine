#pragma once

#define MAYBE_UNUSED [[maybe_unused]]
#define NODISCARD    [[nodiscard]]
#define FALLTHROUGH  [[fallthrough]]
#define UNUSED(x)    (void(x))
#define FORCE_INLINE __forceinline

#define JAM_CONCAT(a, b) a##b
#define JAM_STR_IMPL(x)  #x
#define JAM_STR(x)       JAM_STR_IMPL(x)