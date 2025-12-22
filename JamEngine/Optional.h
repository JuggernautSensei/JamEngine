#pragma once
#include "Vendor/optional/optional.hpp"

namespace jam
{

template<typename T>
using Optional                           = tl::optional<T>;
static constexpr tl::nullopt_t k_nullopt = tl::nullopt;

}   // namespace jam
