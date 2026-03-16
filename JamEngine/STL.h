#pragma once

// use int for avoid issues of unsigned type -> need to change std::ptrdiff_t
#define EASTL_SIZE_T  std::ptrdiff_t
#define EASTL_SSIZE_T std::ptrdiff_t

#include <EASTL/array.h>
#include <EASTL/vector.h>
#include <EASTL/list.h>
#include <EASTL/span.h>
#include <EASTL/deque.h>

namespace Jam
{

template<typename T, typename Allocator = eastl::allocator>
using Vector = eastl::vector<T, Allocator>;

template<typename T, size_t N>
using Array = eastl::array<T, N>;

template<typename T, typename Allocator = eastl::allocator>
using List = eastl::list<T, Allocator>;

template<typename T, typename Allocator = eastl::allocator>
using Deque = eastl::deque<T, Allocator>;

template<typename T, size_t Extent = eastl::dynamic_extent>
using Span = eastl::span<T, Extent>;

template<typename TKey, typename TValue, typename TCompare = std::less<TKey>, typename Allocator = std::allocator<std::pair<const TKey, TValue>>>
using RBMap = std::map<TKey, TValue, TCompare, Allocator>;

template<typename TKey, typename TCompare = std::less<TKey>, typename Allocator = std::allocator<TKey>>
using RBSet = std::set<TKey, TCompare, Allocator>;

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
