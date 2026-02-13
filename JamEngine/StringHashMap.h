#pragma once

namespace Jam
{

namespace Details
{
    struct StringHashMapHasher
    {
        using is_transparent = void;

        [[nodiscard]] std::size_t operator()(const StringView _key) const noexcept
        {
            return ankerl::unordered_dense::detail::wyhash::hash(_key.GetPtr(), _key.GetLength());
        }
    };

}   // namespace Details

template<
    typename TValue,
    typename TAlloc          = std::allocator<std::pair<String, TValue> >,
    typename Bucket          = ankerl::unordered_dense::bucket_type::standard,
    typename BucketContainer = ankerl::unordered_dense::detail::default_container_t>
using StringHashMap = ankerl::unordered_dense::map<String, TValue, Details::StringHashMapHasher, std::equal_to<>, TAlloc, Bucket, BucketContainer>;

template<
    typename TAlloc          = std::allocator<String>,
    typename Bucket          = ankerl::unordered_dense::bucket_type::standard,
    typename BucketContainer = ankerl::unordered_dense::detail::default_container_t>
using StringHashSet = ankerl::unordered_dense::set<String, Details::StringHashMapHasher, std::equal_to<>, TAlloc, Bucket, BucketContainer>;

}   // namespace Jam
