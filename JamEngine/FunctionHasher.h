#pragma once
#include "MemoryHasher.h"

namespace Jam
{

template<auto Func>
    requires(FunctionTraits<decltype(Func)>::Value)
[[nodiscard]] constexpr UInt64 HashOf()
{
    constexpr StringView name { JAM_PRETTY_FUNCTION };
    return name.Hash64();
}

template<auto Func, typename TCaller>
    requires(FunctionTraits<decltype(Func)>::Value)   // 호출자의 주소를 해싱에 포함. 같은 함수라도 호출자가 다르면 다른 해시값이 나옴
[[nodiscard]] UInt64 HashOf(TCaller* pInst)
{
    constexpr UInt64 funcHash   = HashOf<Func>();
    const UInt64     callerAddr = reinterpret_cast<UInt64>(pInst);

    Murmur3Hasher64 hasher {};
    hasher.Mix(funcHash);
    hasher.Mix(callerAddr);
    return hasher.Finish();
}

template<typename TCallable>
    requires(FunctionTraits<TCallable>::Value)
[[nodiscard]] UInt64 HashOf(const TCallable&)   // 추론 기반
{
    constexpr StringView name { JAM_PRETTY_FUNCTION };
    return name.Hash64();
}

}   // namespace Jam