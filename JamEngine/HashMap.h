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

namespace jam
{

namespace hash_map = ankerl::unordered_dense;

}
