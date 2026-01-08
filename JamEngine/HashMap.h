#pragma once
#include "vendor/unordered_dense/unordered_dense.h"

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

#define JAM_HASH_MAP_HASHER_SPECIALIZATION(T)                        \
    template<>                                                       \
    struct ankerl::unordered_dense::hash<T>                          \
    {                                                                \
        using is_avalanching = void;                                 \
        auto operator()(T const& obj) const noexcept -> uint64_t     \
        {                                                            \
            return detail::wyhash::hash(static_cast<uint64_t>(obj)); \
        }                                                            \
    }