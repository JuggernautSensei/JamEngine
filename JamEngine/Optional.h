#pragma once
#include "Vendor/optional/optional.hpp"

namespace jam
{

template<typename T>
using Optional                           = tl::optional<T>;
static constexpr tl::nullopt_t k_nullopt = tl::nullopt;

template<typename T>
bool MoveIfHasValue(Optional<T>& _opt, T& _outValue)
{
    if (!_opt)
    {
        return false;
    }

    _outValue = std::move(*_opt);
    return true;
}

template<typename T>
bool MoveIfHasValue(Optional<T>&& _opt, T& _outValue)
{
    if (_opt)
    {
        _outValue = std::move(*_opt);
        return true;
    }

    return false;
}

}   // namespace jam
