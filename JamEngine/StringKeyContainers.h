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

    struct StringCompareLess
    {
        using is_transparent = void;

        [[nodiscard]] bool operator()(const StringView _lhs, const StringView _rhs) const noexcept
        {
            return _lhs < _rhs;
        }
    };

}   // namespace Details

template<
    typename TValue,
    typename TAlloc          = std::allocator<std::pair<String, TValue>>,
    typename Bucket          = ankerl::unordered_dense::bucket_type::standard,
    typename BucketContainer = ankerl::unordered_dense::detail::default_container_t>
using StringHashMap = HashMap<String, TValue, Details::StringHashMapHasher, std::equal_to<String>, TAlloc, Bucket, BucketContainer>;

template<
    typename TAlloc          = std::allocator<String>,
    typename Bucket          = ankerl::unordered_dense::bucket_type::standard,
    typename BucketContainer = ankerl::unordered_dense::detail::default_container_t>
using StringHashSet = HashSet<String, Details::StringHashMapHasher, std::equal_to<String>, TAlloc, Bucket, BucketContainer>;

template<typename TValue,
         typename TAlloc = std::allocator<std::pair<const String, TValue>>>
using StringMap = std::map<String, TValue, Details::StringCompareLess, TAlloc>;

template<typename TAlloc = std::allocator<String>>
using StringSet = std::set<String, Details::StringCompareLess, TAlloc>;

}   // namespace Jam
