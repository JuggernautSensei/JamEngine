#pragma once
#include <ankerl/unordered_dense.h>

namespace Jam
{

namespace Details
{

    template<typename T>
        requires std::is_integral_v<T>
    struct CASHasher
    {
        [[nodiscard]] size_t operator()(const T _value) const noexcept
        {
            return static_cast<size_t>(_value);
        }
    };

}   // namespace Details

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

template<typename KeyT,
         typename ValueT,
         typename Alloc           = std::allocator<std::pair<KeyT, ValueT>>,
         typename Bucket          = ankerl::unordered_dense::bucket_type::standard,
         typename BucketContainer = ankerl::unordered_dense::detail::default_container_t>
    requires std::is_integral_v<KeyT>
using CASHashMap = ankerl::unordered_dense::map<KeyT, ValueT, Details::CASHasher<KeyT>, std::equal_to<KeyT>, Alloc, Bucket, BucketContainer>;
// CASHashMap은 자체적으로 해시를 제공하지 않습니다.
// 해시된 값을 키로 사용할 때, 최적화된 성능을 제공합니다. 일반적인 용도로는 HashMap을 사용하세요.
// 텍스트의 유일성을 검증할 때, 텍스트의 긴 문장을 키로 사용하는 것이 아니라, 텍스트의 해시값을 키로 사용하는 경우가 유용한 예시입니다.
// 여기서 사용되는 CASHasher는 단순히 들어온 정수값이 해시값이라고 가정합니다. 그러므로 CASHashMap을 사용할 때는 키로 사용할 값이 이미 잘 분산된 값이여야 합니다.
// 수학적으로 엄밀하게 데이터의 유일성을 보장하지 않지만, 잘 분산된 해시값을 사용한다면, 충돌가능성은 0에 수렴합니다.

}   // namespace Jam
