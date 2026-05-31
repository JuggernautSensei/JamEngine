#pragma once
#include "StringHasher.h"

namespace Jam
{

template<IsCharT TChar>
class BasicStringView
{
public:
    using CharTraits = CharTraits<TChar>;
    using CharT      = CharTraits::CharT;
    using Iter       = const CharT*;
    using ConstIter  = const CharT*;

    constexpr BasicStringView() = default;

    /* implicit */ constexpr BasicStringView(
        const CharT* _str)
        : m_ptr(_str)
        , m_len(CharTraits::StrLen(_str))
    {
        JAM_ASSERT(_str, "Null pointer exception.");
    }

    /* implicit */ constexpr BasicStringView(
        const CharT* _str,
        const SizeT  _n)
        : m_ptr(_str)
        , m_len(_n)
    {
        JAM_ASSERT(_str, "Null pointer exception.");
    }

    [[nodiscard]] constexpr const CharT* GetPtr() const
    {
        return m_ptr;
    }

    [[nodiscard]] constexpr const CharT* GetTerminate() const
    {
        return m_ptr + m_len;
    }

    [[nodiscard]] constexpr SizeT GetLength() const
    {
        return m_len;
    }

    [[nodiscard]] constexpr SizeT GetLengthWithTerminate() const
    {
        return m_ptr ? m_len + 1 : 0;
    }

    [[nodiscard]] constexpr bool IsEmpty() const
    {
        return m_len == 0;
    }

    [[nodiscard]] constexpr bool IsNullTerminated() const
    {
        return m_ptr ? m_ptr[m_len] == CharTraits::kNullChar : false;
    }

    [[nodiscard]] constexpr const CharT& operator[](const SizeT _index) const
    {
        JAM_ASSERT(_index >= 0 && _index < m_len, "Index out of bounds.");
        return m_ptr[_index];
    }

    constexpr bool operator==(const BasicStringView _other) const
    {
        return CompareEqual(_other);
    }

    constexpr bool operator!=(const BasicStringView _other) const
    {
        return !(*this == _other);
    }

    template<typename T>
        requires std::is_integral_v<T>
    [[nodiscard]] constexpr Optional<T> ParseAs(const Int32 _base = 10) const
    {
        return CharTraits::template ParseAs<T>(m_ptr, _base);
    }

    template<typename T>
        requires std::is_floating_point_v<T>
    [[nodiscard]] constexpr Optional<T> ParseAs() const
    {
        return CharTraits::template ParseAs<T>(m_ptr);
    }

    [[nodiscard]] constexpr CharResult Find(const BasicStringView _other, const SizeT _offset, const SizeT _count) const
    {
        JAM_ASSERT(_offset <= m_len, "Offset out of bounds.");
        JAM_ASSERT(_count <= m_len - _offset, "Count exceeds string length from offset.");
        CharResult result = CharTraits::StrFind(m_ptr + _offset, _count, _other.m_ptr, _other.m_len);
        if (result.IsSucceeded())
        {
            result.pos += _offset;
        }
        return result;
    }

    [[nodiscard]] constexpr CharResult Find(const BasicStringView _other, const SizeT _offset = 0) const
    {
        return Find(_other, _offset, m_len - _offset);
    }

    [[nodiscard]] constexpr CharResult Find(const CharT _c, const SizeT _offset, const SizeT _count) const
    {
        JAM_ASSERT(_offset <= m_len, "Offset out of bounds.");
        JAM_ASSERT(_count <= m_len - _offset, "Count exceeds string length from offset.");
        CharResult result = CharTraits::StrFind(m_ptr + _offset, _count, _c);
        if (result.IsSucceeded())
        {
            result.pos += _offset;
        }
        return result;
    }

    [[nodiscard]] constexpr CharResult Find(const CharT _c, const SizeT _offset = 0) const
    {
        return Find(_c, _offset, m_len - _offset);
    }

    [[nodiscard]] constexpr CharResult RFind(const BasicStringView _other, const SizeT _offset, const SizeT _count) const
    {
        JAM_ASSERT(_count <= m_len, "Count exceeds length.");
        JAM_ASSERT(_offset <= m_len - _count, "Offset out of bounds for count.");
        CharResult result = CharTraits::StrRFind(m_ptr + _offset, _count, _other.m_ptr, _other.m_len);
        if (result.IsSucceeded())
        {
            result.pos += _offset;
        }
        return result;
    }

    [[nodiscard]] constexpr CharResult RFind(const BasicStringView _other, const SizeT _offset = 0) const
    {
        return RFind(_other, _offset, m_len - _offset);
    }

    [[nodiscard]] constexpr CharResult RFind(const CharT _c, const SizeT _offset, const SizeT _count) const
    {
        JAM_ASSERT(_count <= m_len, "Count exceeds length.");
        JAM_ASSERT(_offset <= m_len - _count, "Offset out of bounds for count.");
        CharResult result = CharTraits::StrRFind(m_ptr + _offset, _count, _c);
        if (result.IsSucceeded())
        {
            result.pos += _offset;
        }
        return result;
    }

    [[nodiscard]] constexpr CharResult RFind(const CharT _c, const SizeT _offset = 0) const
    {
        return RFind(_c, _offset, m_len - _offset);
    }

    [[nodiscard]] constexpr bool CompareEqual(const BasicStringView _other, const SizeT _offset = 0) const
    {
        JAM_ASSERT(_offset <= m_len, "Offset out of bounds.");
        if (m_len - _offset != _other.m_len)
        {
            return false;
        }

        return CharTraits::StrEqual(m_ptr + _offset, _other.m_ptr, _other.m_len);
    }

    [[nodiscard]] constexpr bool CompareEqual(const CharT _c, const SizeT _offset = 0) const
    {
        JAM_ASSERT(_offset <= m_len, "Offset out of bounds.");
        if (m_len - _offset != 1)
        {
            return false;
        }
        return m_ptr[_offset] == _c;
    }

    [[nodiscard]] constexpr bool CompareEqualIgnoreCase(const BasicStringView _other, const SizeT _offset = 0) const
    {
        JAM_ASSERT(_offset <= m_len, "Offset out of bounds.");
        if (m_len - _offset != _other.m_len)
        {
            return false;
        }

        return CharTraits::StrEqualIgnoreCase(m_ptr + _offset, _other.m_ptr, _other.m_len);
    }

    [[nodiscard]] constexpr bool CompareEqualIgnoreCase(const CharT _c, const SizeT _offset = 0) const
    {
        JAM_ASSERT(_offset <= m_len, "Offset out of bounds.");
        if (m_len - _offset != 1)
        {
            return false;
        }

        return CharTraits::ToLower(m_ptr[_offset]) == CharTraits::ToLower(_c);
    }

    [[nodiscard]] constexpr SizeT Compare(const BasicStringView _other, const SizeT _offset = 0) const
    {
        JAM_ASSERT(_offset <= m_len, "Offset out of bounds.");
        return CharTraits::StrCompare(m_ptr + _offset, m_len - _offset, _other.m_ptr, _other.m_len);
    }

    [[nodiscard]] constexpr SizeT Compare(const BasicStringView _other, const SizeT _offset, const SizeT _count) const
    {
        return SubView(_offset, _count).Compare(_other);
    }

    [[nodiscard]] constexpr bool StartsWith(const CharT _c) const
    {
        return SubView(0, 1).CompareEqual(_c);
    }

    [[nodiscard]] constexpr bool StartsWith(const BasicStringView _other) const
    {
        return SubView(0, _other.m_len).CompareEqual(_other);
    }

    [[nodiscard]] constexpr bool EndsWith(const CharT _c) const
    {
        return SubView(m_len - 1, 1).CompareEqual(_c);
    }

    [[nodiscard]] constexpr bool EndsWith(const BasicStringView _other) const
    {
        return SubView(m_len - _other.m_len, _other.m_len).CompareEqual(_other);
    }

    [[nodiscard]] constexpr BasicStringView SubView(const SizeT _pos) const
    {
        JAM_ASSERT(_pos >= 0, "SubView position cannot be negative.");
        JAM_ASSERT(_pos <= m_len, "SubView position exceeds string length.");
        return { m_ptr + _pos, m_len - _pos };
    }

    [[nodiscard]] constexpr BasicStringView SubView(const SizeT _pos, const SizeT _len) const
    {
        JAM_ASSERT(_pos >= 0, "SubView position cannot be negative.");
        JAM_ASSERT(_pos + _len <= m_len, "SubView range exceeds string length.");
        return { m_ptr + _pos, _len };
    }

    [[nodiscard]] BasicStringView LTrimmed() const
    {
        const SizeT lTrimmed = CharTraits::StrLTrim(m_ptr, m_len);
        return SubView(lTrimmed);
    }

    [[nodiscard]] BasicStringView RTrimmed() const
    {
        const SizeT rTrimmed = CharTraits::StrRTrim(m_ptr, m_len);
        return SubView(0, rTrimmed);
    }

    [[nodiscard]] BasicStringView Trimmed() const
    {
        const auto [lTrimmed, trimmedLen] = CharTraits::StrTrim(m_ptr, m_len);
        return SubView(lTrimmed, trimmedLen);
    }

    [[nodiscard]] constexpr UInt64 Hash() const
    {
        Fnv1ahHasher<CharT> hasher {};
        hasher.Mix(m_ptr, m_len);
        return hasher.Finish();
    }

    [[nodiscard]] constexpr UInt64 IgnoreCaseHash() const
    {
        IgnoreCaseFnv1ahHasher<CharT> hasher {};
        hasher.Mix(m_ptr, m_len);
        return hasher.Finish();
    }

    [[nodiscard]] constexpr CharT Back() const
    {
        JAM_ASSERT(!IsEmpty(), "StringView is empty.");
        return m_ptr[m_len - 1];
    }

    [[nodiscard]] constexpr CharT Front() const
    {
        JAM_ASSERT(!IsEmpty(), "StringView is empty.");
        return m_ptr[0];
    }

    [[nodiscard]] constexpr bool operator<(const BasicStringView _other) const
    {
        return Compare(_other) < 0;
    }

    [[nodiscard]] constexpr bool operator>(const BasicStringView _other) const
    {
        return Compare(_other) > 0;
    }

    [[nodiscard]] constexpr bool operator<=(const BasicStringView _other) const
    {
        return Compare(_other) <= 0;
    }

    [[nodiscard]] constexpr bool operator>=(const BasicStringView _other) const
    {
        return Compare(_other) >= 0;
    }

    ////////// file system //////////

    constexpr void Decompose(
        BasicStringView* _pOutDirOrNull,
        BasicStringView* _pOutFileNameOrNull,
        BasicStringView* _pOutBaseNameOrNull,
        BasicStringView* _pOutExtOrNull) const
    {
        const PathDecomposeResult ret = CharTraits::DecomposePath(m_ptr, m_len);
        if (_pOutDirOrNull)
        {
            *_pOutDirOrNull = ret.bHasDir ? SubView(0, ret.dirLen) : BasicStringView {};
        }

        if (_pOutFileNameOrNull)
        {
            *_pOutFileNameOrNull = ret.bHasBaseName ? SubView(ret.baseNamePos, ret.baseNameLen + (ret.bHasExt ? ret.extLen : 0)) : BasicStringView {};
        }

        if (_pOutBaseNameOrNull)
        {
            *_pOutBaseNameOrNull = ret.bHasBaseName ? SubView(ret.baseNamePos, ret.baseNameLen) : BasicStringView {};
        }

        if (_pOutExtOrNull)
        {
            *_pOutExtOrNull = ret.bHasExt ? SubView(ret.extPos, ret.extLen) : BasicStringView {};
        }
    }

    [[nodiscard]] BasicStringView GetDirectory() const
    {
        BasicStringView dir = {};
        Decompose(&dir, nullptr, nullptr, nullptr);
        return dir;
    }

    [[nodiscard]] BasicStringView GetFileName() const
    {
        BasicStringView fileName = {};
        Decompose(nullptr, &fileName, nullptr, nullptr);
        return fileName;
    }

    [[nodiscard]] BasicStringView GetBaseName() const
    {
        BasicStringView baseName = {};
        Decompose(nullptr, nullptr, &baseName, nullptr);
        return baseName;
    }

    [[nodiscard]] BasicStringView GetExtension() const
    {
        BasicStringView ext = {};
        Decompose(nullptr, nullptr, nullptr, &ext);
        return ext;
    }

    [[nodiscard]] bool IsAbsolute() const
    {
        return m_len > 2 && CharTraits::IsAlphabet(m_ptr[0]) && m_ptr[1] == CharTraits::kColon
            || m_len > 0 && CharTraits::IsPathSeparator(m_ptr[0]);
    }

    [[nodiscard]] bool IsRelative() const
    {
        return !IsAbsolute();
    }

    [[nodiscard]] bool IsDirectory() const
    {
        return !IsEmpty() && CharTraits::IsPathSeparator(Back());
    }

    [[nodiscard]] bool IsFile() const
    {
        return !IsEmpty() && !CharTraits::IsPathSeparator(Back());
    }

    [[nodiscard]] bool MatchExtension(const BasicStringView _ext) const
    {
        BasicStringView ext = GetExtension();
        ext                 = _ext.StartsWith(CharTraits::kDot) ? ext : ext.SubView(1);   // If _ext starts with dot, then ext should also start with dot for comparison, otherwise both should not start with dot.
        return ext.CompareEqualIgnoreCase(_ext, 0);
    }

    [[nodiscard]] bool IsBasePathOf(const BasicStringView _other) const
    {
        JAM_ASSERT(IsDirectory(), "IsBasePathOf can only be called on directory paths.");
        return _other.StartsWith(*this);
    }

    ////////// std glue //////////

    using value_type     = CharT;
    using iterator       = Iter;
    using const_iterator = ConstIter;

    [[nodiscard]] constexpr size_t size() const
    {
        return static_cast<size_t>(m_len);
    }

    [[nodiscard]] constexpr size_t length() const
    {
        return static_cast<size_t>(m_len);
    }

    [[nodiscard]] constexpr TChar* data()
    {
        return const_cast<TChar*>(m_ptr);
    }

    [[nodiscard]] constexpr const TChar* data() const
    {
        return m_ptr;
    }

    [[nodiscard]] constexpr iterator begin()
    {
        return m_ptr;
    }

    [[nodiscard]] constexpr iterator end()
    {
        return m_ptr + m_len;
    }

    [[nodiscard]] constexpr const_iterator begin() const
    {
        return m_ptr;
    }

    [[nodiscard]] constexpr const_iterator end() const
    {
        return m_ptr + m_len;
    }

private:
    const CharT* m_ptr = CharTraits::kPlaceholder;   // DefaultExport empty
    SizeT        m_len = 0;
};

using StringView  = BasicStringView<char>;
using WStringView = BasicStringView<wchar_t>;

////////// String Hash Helper For Literal Strings //////////

template<typename TChar>
[[nodiscard]] constexpr UInt64 HashOf(const TChar* _str)
{
    Fnv1ahHasher<TChar> hasher {};
    hasher.Mix(_str);
    return hasher.Finish();
}

template<typename TChar>
[[nodiscard]] constexpr UInt64 IgnoreCaseHashOf(const TChar* _str)
{
    IgnoreCaseFnv1ahHasher<TChar> hasher {};
    hasher.Mix(_str);
    return hasher.Finish();
}

}   // namespace Jam

template<Jam::IsCharT CharT>
struct std::hash<Jam::BasicStringView<CharT>>
{
    std::size_t operator()(const Jam::StringView& _sv) const noexcept
    {
        return _sv.Hash();
    }
};

template<Jam::IsCharT CharT>
struct ankerl::unordered_dense::hash<Jam::BasicStringView<CharT>>
{
    [[nodiscard]] std::size_t operator()(const Jam::StringView& _sv) const noexcept
    {
        return ankerl::unordered_dense::detail::wyhash::hash(_sv.GetPtr(), static_cast<size_t>(_sv.GetLength()));
    }
};

template<>
struct fmt::formatter<Jam::StringView> : fmt::formatter<std::string_view>
{
    template<typename FormatContext>
    auto format(const Jam::StringView& _sv, FormatContext& _ctx) const
    {
        return fmt::formatter<std::string_view>::format({ _sv.GetPtr(), static_cast<size_t>(_sv.GetLength()) }, _ctx);
    }
};

template<>
struct fmt::formatter<Jam::WStringView, wchar_t> : fmt::formatter<std::wstring_view, wchar_t>
{
    template<typename FormatContext>
    auto format(const Jam::WStringView& _sv, FormatContext& _ctx) const
    {
        return fmt::formatter<std::wstring_view, wchar_t>::format({ _sv.GetPtr(), static_cast<size_t>(_sv.GetLength()) }, _ctx);
    }
};