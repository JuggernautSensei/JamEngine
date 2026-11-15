#pragma once

namespace Jam
{

template<IsCharT TChar, SizeT kCap>
class BasicStaticString : public StringBase<BasicStaticString<TChar, kCap>, TChar>
{
public:
    using BaseT       = StringBase<BasicStaticString, TChar>;
    using CharTraits  = CharTraits<TChar>;
    using CharT       = CharTraits::CharT;
    using StringViewT = BasicStringView<CharT>;
    using Iter        = CharT*;
    using ConstIter   = const CharT*;

    template<typename... Args>
    using FormatStringT = CharTraits::template FormatStringT<Args...>;

    using BaseT::Append;
    using BaseT::Insert;
    using BaseT::Set;

    BasicStaticString()
        : m_len(0)
    {
        m_str[0] = CharTraits::kNull;
    }

    ~BasicStaticString() = default;

    /* implicit */ BasicStaticString(
        const StringViewT _str)
    {
        Set(_str);
    }

    /* implicit */ BasicStaticString(
        const CharT* _str)
    {
        Set(_str);
    }

    /* implicit */ BasicStaticString(
        const CharT* _str,
        const SizeT  _n)
    {
        Set({ _str, _n });
    }

    /* implicit */ BasicStaticString(
        const SizeT _count,
        const CharT _c)
    {
        Resize(_count, _c);
    }

    BasicStaticString(const BasicStaticString& _other)            = default;
    BasicStaticString& operator=(const BasicStaticString& _other) = default;

    BasicStaticString(
        BasicStaticString&& _other) noexcept
        : m_str(_other.m_str)
        , m_len(_other.m_len)
    {
        _other.Resize(0);
    }

    BasicStaticString& operator=(
        BasicStaticString&& _other) noexcept
    {
        if (this != &_other)
        {
            m_str = _other.m_str;
            _other.Resize(0);
        }
        return *this;
    }

    BasicStaticString& operator=(
        const StringViewT _other)
    {
        Set(_other);
        return *this;
    }

    BasicStaticString& operator=(
        const TChar _c)
    {
        Set(_c);
        return *this;
    }

    void Set(
        const StringViewT _str)
    {
        JAM_ASSERT(_str.GetLength() <= kCap, "String length exceeds stack string capacity.");
        m_len = CharTraits::StrCpy(m_str, kCap + 1, _str.GetPtr(), _str.GetLength());
    }

    void Append(
        const StringViewT _str)
    {
        JAM_ASSERT(GetLength() + _str.GetLength() <= kCap, "String length exceeds stack string capacity.");
        m_len += CharTraits::StrCpy(m_str + m_len, kCap + 1 - m_len, _str.GetPtr(), _str.GetLength());
    }

    void Insert(
        const SizeT       _pos,
        const StringViewT _str)
    {
        JAM_ASSERT(_pos <= GetLength(), "OnInsert position out of bounds.");
        JAM_ASSERT(GetLength() + _str.GetLength() <= kCap, "String length exceeds stack string capacity.");
        ::memmove(m_str + _pos + _str.GetLength(), m_str + _pos, (GetLength() - _pos) * sizeof(TChar));
        m_len += CharTraits::StrCpy(m_str + _pos, kCap + 1 - _pos, _str.GetPtr(), _str.GetLength());
    }

    void Erase(
        const SizeT _pos,
        const SizeT _count)
    {
        JAM_ASSERT(_pos <= GetLength(), "Erase position out of bounds.");
        JAM_ASSERT(_pos + _count <= GetLength(), "Erase count exceeds string length from position.");
        ::memmove(m_str + _pos, m_str + _pos + _count, (GetLength() - (_pos + _count)) * sizeof(TChar));
        m_len -= _count;
        m_str[m_len] = CharTraits::kNull;   // null terminate after erase
    }

    void Clear()
    {
        m_len    = 0;
        m_str[0] = CharTraits::kNull;
    }

    void Resize(
        const SizeT _newLen,
        const CharT _c = CharTraits::kNull)
    {
        JAM_ASSERT(_newLen <= kCap, "Resize length exceeds stack string capacity.");
        if (_newLen > GetLength())
        {
            std::fill(GetPtr() + GetLength(), GetPtr() + _newLen, _c);
        }

        m_len        = _newLen;
        m_str[m_len] = CharTraits::kNull;   // null terminate after resize
    }

    [[nodiscard]] SizeT GetLength() const
    {
        return m_len;
    }

    [[nodiscard]] SizeT GetCapacity() const
    {
        return kCap;
    }

    [[nodiscard]] const CharT* GetPtr() const
    {
        return m_str;
    }

    [[nodiscard]] CharT* GetPtr()
    {
        return m_str;
    }

    [[nodiscard]] BasicString<CharT> ToString() const
    {
        return { GetPtr(), GetLength() };
    }

private:
    CharT m_str[kCap + 1];
    SizeT m_len;
};

template<SizeT kCap>
using StaticString = BasicStaticString<char, kCap>;

template<SizeT kCap>
using StaticWString = BasicStaticString<wchar_t, kCap>;

}   // namespace Jam

template<Jam::SizeT kCap>
struct fmt::formatter<Jam::StaticString<kCap>> : fmt::formatter<std::string_view>
{
    template<typename FormatContext>
    auto format(const Jam::StaticString<kCap>& _str, FormatContext& _ctx) const
    {
        return fmt::formatter<std::string_view>::format(_str.ToStringView(), _ctx);
    }
};

template<Jam::SizeT kCap>
struct fmt::formatter<Jam::StaticWString<kCap>, wchar_t> : fmt::formatter<std::wstring_view, wchar_t>
{
    template<typename FormatContext>
    auto format(const Jam::StaticWString<kCap>& _str, FormatContext& _ctx) const
    {
        return fmt::formatter<std::wstring_view, wchar_t>::format(_str.ToStringView(), _ctx);
    }
};