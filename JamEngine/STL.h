#pragma once

// use int for avoid issues of unsigned type
#define EASTL_SIZE_T  __int32
#define EASTL_SSIZE_T __int32

#include <EASTL/array.h>
#include <EASTL/vector.h>
#include <EASTL/bonus/ring_buffer.h>
#include <EASTL/bonus/fixed_ring_buffer.h>
#include <EASTL/list.h>
#include <EASTL/span.h>

namespace jam
{

template<typename T>
using Vector = eastl::vector<T>;

template<typename T, size_t N>
using Array = eastl::array<T, N>;

template<typename T>
using CircularQ = eastl::ring_buffer<T>;

template<typename T, size_t N>
using FixedCircularQ = eastl::fixed_ring_buffer<T, N>;

template<typename T>
using List = eastl::list<T>;

template<typename T>
using Span = eastl::span<T>;

}   // namespace jam