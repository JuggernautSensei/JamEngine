#pragma once
#include "MemoryHasher.h"

namespace Jam
{

template<auto Func>
    requires(FunctionTraits<decltype(Func)>::Value)
[[nodiscard]] constexpr UInt64 FuncHashOf()
{
    constexpr StringView name { JAM_PRETTY_FUNCTION };
    return name.Hash();
}

template<auto Func, typename TCaller>
    requires(FunctionTraits<decltype(Func)>::Value)   // 호출자의 주소를 해싱에 포함. 같은 함수라도 호출자가 다르면 다른 해시값이 나옴
[[nodiscard]] UInt64 FuncHashOf(TCaller* pInst)
{
    constexpr UInt64 funcHash   = FuncHashOf<Func>();
    const UInt64     callerAddr = reinterpret_cast<UInt64>(pInst);

    Murmur3Hasher hasher {};
    hasher.Mix(AsMemoryView(funcHash));
    hasher.Mix(AsMemoryView(callerAddr));
    return hasher.Finish();
}

template<typename TCallable>
    requires(FunctionTraits<TCallable>::Value)
[[nodiscard]] UInt64 FuncHashOf(const TCallable&)   // 추론 기반
{
    constexpr StringView name { JAM_PRETTY_FUNCTION };
    return name.Hash();
}

}   // namespace Jam