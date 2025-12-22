#pragma once

namespace jam
{

template<typename T>
class BasicStringView;

using StringView  = BasicStringView<char>;
using WStringView = BasicStringView<wchar_t>;

}   // namespace jam
