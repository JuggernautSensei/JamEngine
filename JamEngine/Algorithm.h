#pragma once
#include "Random.h"

namespace Jam
{

template<typename T, IsRng TRng>
void Shuffle(Span<T> _container, TRng& _rng) noexcept
{
    // Fisher-Yates Shuffle (Knuth Shuffle)
    const SizeT back = _container.size() - 1;
    for (SizeT i = back; i > 0; --i)
    {
        const SizeT j = MakeRandom<SizeT>(_rng, 0, i);
        std::swap(_container[i], _container[j]);
    }
}

template<typename TContainer, IsRng TRng>
void Shuffle(TContainer& _container, TRng& _rng) noexcept
{
    Shuffle(Span<typename TContainer::value_type> { _container }, _rng);
}

}   // namespace Jam
