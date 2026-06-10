#pragma once
#define EASTL_SIZE_T  std::ptrdiff_t
#define EASTL_SSIZE_T std::ptrdiff_t

#include <EASTL/array.h>
#include <EASTL/vector.h>
#include <EASTL/list.h>
#include <EASTL/span.h>
#include <EASTL/map.h>

#include "Allocator.h"

namespace Jam
{

namespace Details
{

    template<typename T>
        requires std::is_integral_v<T>
    struct NoopHasher
    {
        [[nodiscard]] size_t operator()(const T _value) const noexcept
        {
            return static_cast<size_t>(_value);
        }
    };

}   // namespace Details

template<typename T, typename Allocator = eastl::allocator>
using Vector = eastl::vector<T, Allocator>;

template<typename T, size_t N>
using Array = eastl::array<T, N>;

template<typename T, typename Allocator = eastl::allocator>
using List = eastl::list<T, Allocator>;

template<typename T, size_t kExtent = eastl::dynamic_extent>
using Span = eastl::span<T, kExtent>;

template<typename T, typename TPred = std::less<T>, typename Allocator = eastl::allocator>
using RBSet = std::set<T, TPred, Allocator>;

template<typename Key, typename Value, typename TPred = std::less<Key>, typename Allocator = eastl::allocator>
using RBMap = std::map<Key, Value, TPred, Allocator>;

template<typename KeyT,
         typename ValueT,
         typename Hash            = ankerl::unordered_dense::hash<KeyT>,
         typename KeyEqual        = std::equal_to<KeyT>,
         typename Alloc           = std::allocator<std::pair<KeyT, ValueT>>,
         typename Bucket          = ankerl::unordered_dense::bucket_type::standard,
         typename BucketContainer = ankerl::unordered_dense::detail::default_container_t>
using HashMap = ankerl::unordered_dense::map<KeyT, ValueT, Hash, KeyEqual, Alloc, Bucket, BucketContainer>;

template<typename KeyT,
         typename Hash            = ankerl::unordered_dense::hash<KeyT>,
         typename KeyEqual        = std::equal_to<KeyT>,
         typename Alloc           = std::allocator<KeyT>,
         typename Bucket          = ankerl::unordered_dense::bucket_type::standard,
         typename BucketContainer = ankerl::unordered_dense::detail::default_container_t>
using HashSet = ankerl::unordered_dense::set<KeyT, Hash, KeyEqual, Alloc, Bucket, BucketContainer>;

// PrehashedMap은 키가 이미 해시된 정수값임을 전제로 동작합니다.
// 해시 함수로 항등 함수를 사용하므로, 해시 연산 비용이 없습니다.

template<typename KeyT,
         typename ValueT,
         typename Alloc           = std::allocator<std::pair<KeyT, ValueT>>,
         typename Bucket          = ankerl::unordered_dense::bucket_type::standard,
         typename BucketContainer = ankerl::unordered_dense::detail::default_container_t>
    requires std::is_integral_v<KeyT>
using PreHashedMap = ankerl::unordered_dense::map<KeyT, ValueT, Details::NoopHasher<KeyT>, std::equal_to<KeyT>, Alloc, Bucket, BucketContainer>;

template<typename KeyT,
         typename Alloc           = std::allocator<KeyT>,
         typename Bucket          = ankerl::unordered_dense::bucket_type::standard,
         typename BucketContainer = ankerl::unordered_dense::detail::default_container_t>
    requires std::is_integral_v<KeyT>
using PreHashedSet = ankerl::unordered_dense::set<KeyT, Details::NoopHasher<KeyT>, std::equal_to<KeyT>, Alloc, Bucket, BucketContainer>;

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
