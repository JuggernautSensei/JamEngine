#pragma once

#include <EASTL/array.h>
#include <EASTL/vector.h>
#include <EASTL/bonus/ring_buffer.h>
#include <EASTL/bonus/fixed_ring_buffer.h>

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

}   // namespace jam