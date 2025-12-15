#pragma once

#include <EASTL/array.h>
#include <EASTL/vector.h>

namespace jam
{

template<typename T>
using Vector = eastl::vector<T>;

template<typename T, size_t N>
using Array = eastl::array<T, N>;

}   // namespace jam