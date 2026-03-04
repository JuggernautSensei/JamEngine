#pragma once

namespace Jam
{

template<IsCharT TChar, SizeT k_cap>
class BasicStackString : public StringUtilities<BasicStackString<TChar, k_cap>, TChar>
{
public:
    using BaseT       = StringUtilities<BasicStackString, TChar>;
    using CharTraits  = CharTraits<TChar>;
    using CharT       = CharTraits::CharT;
    using CharFuncs   = CharFuncs<CharT>;
    using StringViewT = BasicStringView<CharT>;
    using Iter        = CharT*;
    using ConstIter   = const CharT*;

    template<typename... Args>
    using FormatStringT = CharTraits::template FormatStringT<Args...>;

    using BaseT::Append;
    using BaseT::Insert;
    using BaseT::Set;

    BasicStackString()
        : m_len(0)
    {
        m_str[0] = CharTraits::k_nullChar;
    }

    ~BasicStackString() = default;

    /* implicit */ BasicStackString(
        const StringViewT _str)
    {
        Set(_str);
    }

    /* implicit */ BasicStackString(
        const CharT* _str)
    {
        Set(_str);
    }

    /* implicit */ BasicStackString(
        const CharT* _str,
        const SizeT  _n)
    {
        Set({ _str, _n });
    }

    /* implicit */ BasicStackString(
        const SizeT _count,
        const CharT _c)
    {
        Resize(_count, _c);
    }

    BasicStackString(const BasicStackString& _other)            = default;
    BasicStackString& operator=(const BasicStackString& _other) = default;

    BasicStackString(
        BasicStackString&& _other) noexcept
        : m_str(_other.m_str)
        , m_len(_other.m_len)
    {
        _other.Resize(0);
    }

    BasicStackString& operator=(
        BasicStackString&& _other) noexcept
    {
        if (this != &_other)
        {
            m_str = _other.m_str;
            _other.Resize(0);
        }
        return *this;
    }

    BasicStackString& operator=(
        const StringViewT _other)
    {
        Set(_other);
        return *this;
    }

    BasicStackString& operator=(
        const TChar _c)
    {
        Set(_c);
        return *this;
    }

    void Set(
        const StringViewT _str)
    {
        JAM_ASSERT(_str.GetLength() <= k_cap, "String length exceeds stack string capacity.");
        m_len = CharFuncs::StrCpy(m_str, k_cap + 1, _str.GetPtr(), _str.GetLength());
    }

    void Append(
        const StringViewT _str)
    {
        JAM_ASSERT(GetLength() + _str.GetLength() <= k_cap, "String length exceeds stack string capacity.");
        m_len += CharFuncs::StrCpy(m_str + m_len, k_cap + 1 - m_len, _str.GetPtr(), _str.GetLength());
    }

    void Insert(
        const SizeT       _pos,
        const StringViewT _str)
    {
        JAM_ASSERT(_pos <= GetLength(), "OnInsert position out of bounds.");
        JAM_ASSERT(GetLength() + _str.GetLength() <= k_cap, "String length exceeds stack string capacity.");
        ::memmove(m_str + _pos + _str.GetLength(), m_str + _pos, (GetLength() - _pos) * sizeof(TChar));
        m_len += CharFuncs::StrCpy(m_str + _pos, k_cap + 1 - _pos, _str.GetPtr(), _str.GetLength());
    }

    void Erase(
        const SizeT _pos,
        const SizeT _count)
    {
        JAM_ASSERT(_pos <= GetLength(), "Erase position out of bounds.");
        JAM_ASSERT(_pos + _count <= GetLength(), "Erase count exceeds string length from position.");
        ::memmove(m_str + _pos, m_str + _pos + _count, (GetLength() - (_pos + _count)) * sizeof(TChar));
        m_len -= _count;
        m_str[m_len] = CharTraits::k_nullChar;   // null terminate after erase
    }

    void Clear()
    {
        m_len    = 0;
        m_str[0] = CharTraits::k_nullChar;
    }

    void Resize(
        const SizeT _newLen,
        const CharT _c = CharTraits::k_nullChar)
    {
        JAM_ASSERT(_newLen <= k_cap, "Resize length exceeds stack string capacity.");
        if (_newLen > GetLength())
        {
            std::fill(GetPtr() + GetLength(), GetPtr() + _newLen, _c);
        }

        m_len        = _newLen;
        m_str[m_len] = CharTraits::k_nullChar;   // null terminate after resize
    }

    [[nodiscard]] SizeT GetLength() const
    {
        return m_len;
    }

    [[nodiscard]] SizeT GetCapacity() const
    {
        return k_cap;
    }

    [[nodiscard]] const CharT* GetPtr() const
    {
        return m_str;
    }

    [[nodiscard]] CharT* GetPtr()
    {
        return m_str;
    }

private:
    CharT m_str[k_cap + 1];
    SizeT m_len;
};

template<SizeT k_cap>
using StackString = BasicStackString<char, k_cap>;

template<SizeT k_cap>
using WStackString = BasicStackString<wchar_t, k_cap>;

}   // namespace Jam

template<Jam::IsCharT TChar, Jam::SizeT k_cap>
struct fmt::formatter<Jam::BasicStackString<TChar, k_cap>>
{
    constexpr auto parse(const format_parse_context& ctx) const
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const Jam::BasicStackString<TChar, k_cap>& str, FormatContext& ctx) const
    {
        if constexpr (std::is_same_v<TChar, char>)
        {
            return fmt::format_to(ctx.out(), "{}", fmt::string_view { str.GetPtr(), Jam::NumericCast<Jam::USizeT>(str.GetLength()) });
        }
        else
        {
            return fmt::format_to(ctx.out(), "{}", fmt::wstring_view { str.GetPtr(), Jam::NumericCast<Jam::USizeT>(str.GetLength()) });
        }
    }
};