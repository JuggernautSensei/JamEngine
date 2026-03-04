#pragma once

// use int for avoid issues of unsigned type -> need to change std::ptrdiff_t
#define EASTL_SIZE_T  std::ptrdiff_t
#define EASTL_SSIZE_T std::ptrdiff_t

#include <EASTL/array.h>
#include <EASTL/vector.h>
#include <EASTL/list.h>
#include <EASTL/span.h>

namespace Jam
{

template<typename T, typename Allocator = eastl::allocator>
using Vector = eastl::vector<T, Allocator>;

template<typename T, size_t N>
using Array = eastl::array<T, N>;

template<typename T, typename Allocator = eastl::allocator>
using List = eastl::list<T, Allocator>;

template<typename T, size_t Extent = eastl::dynamic_extent>
using Span = eastl::span<T, Extent>;

}   // namespace Jam

////////// for eastl allocator //////////

inline void* operator new[](
    const size_t _size,
    const char*,
    int,
    unsigned,
    const char*,
    int)
{
    return Jam::New(_size);
}

inline void* operator new[](
    const size_t _size,
    const size_t _alignment,
    const size_t _alignmentOffset,
    const char*,
    int,
    unsigned,
    const char*,
    int)
{
    return Jam::AlignedNewAt(_size, _alignment, _alignmentOffset);
}
