#pragma once

namespace Jam
{

template<typename T>
struct FunctionTraits : FunctionTraits<decltype(&T::operator())>
{
    constexpr static bool Value = true;
};

template<typename R, typename C, typename... Args>
struct FunctionTraits<R (C::*)(Args...) const>   // const member function traits
{
    using Type                  = R(Args...);
    constexpr static bool Value = true;
};

template<typename R, typename C, typename... Args>
struct FunctionTraits<R (C::*)(Args...)>   // member function traits
{
    using Type                  = R(Args...);
    constexpr static bool Value = true;
};

template<typename R, typename... Args>
struct FunctionTraits<R (*)(Args...)>   // free function traits
{
    using Type                  = R(Args...);
    constexpr static bool Value = true;
};

}   // namespace Jam
