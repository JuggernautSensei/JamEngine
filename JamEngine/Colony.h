#pragma once
#include "Vendor/plf_colony/plf_colony.h"

namespace Jam
{

// what is colony?
// vector와 list를 교묘하게 섞은 자료구조.
// 삽입, 삭제 등이 O(1)에 가깝고. iterator가 무효화되지 않는다는 점에서 list와 비슷하다.
// 메모리가 비교적 연속적으로 할당되기에 vector처럼 캐시 친화적이다.
//
// when should I use colony?
// 요소의 삽입, 삭제가 빈번히 일어나고, iterator 무효화가 없어야 하는 경우에 유용하다.

template<
    typename TValue,
    typename TAlloc          = std::allocator<TValue>,
    plf::priority k_priority = plf::priority::performance>
using Colony = plf::colony<TValue, TAlloc, k_priority>;

}   // namespace Jam