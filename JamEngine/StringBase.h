#pragma once
#include "CharTraits.h"
#include "StringView.h"
#include "CharEncoder.h"

namespace Jam
{

template<typename TString,
         typename TChar,
         typename TStringView = BasicStringView<TChar>,
         typename CharTraits  = CharTraits<TChar>>
class StringBase
{
public:
    using Iter      = TChar*;
    using ConstIter = const TChar*;

    template<typename... Args>
    using FormatStringT = CharTraits::template FormatStringT<Args...>;

    ////////// basic //////////

    void Set(
        const TChar* _str)
    {
        Self_().Set(TStringView { _str });
    }

    void Set(
        const TChar* _str,
        const SizeT  _n)
    {
        Self_().Set({ _str, _n });
    }

    void Set(
        const TChar _c)
    {
        Self_().Set({ &_c, 1 });
    }

    void Append(
        const TChar _c)
    {
        Self_().Append({ &_c, 1 });
    }

    void Append(
        const TChar* _str)
    {
        Self_().Append({ _str, CharTraits::StrLen(_str) });
    }

    void Append(
        const TChar* _str,
        const SizeT  _n)
    {
        Self_().Append({ _str, _n });
    }

    void Insert(
        const SizeT  _pos,
        const TChar* _str)
    {
        Self_().Insert(_pos, { _str, CharTraits::StrLen(_str) });
    }

    void Insert(
        const SizeT _pos,
        const TChar _c)
    {
        Self_().Insert(_pos, { &_c, 1 });
    }

    void Insert(
        const SizeT  _pos,
        const TChar* _str,
        const SizeT  _n)
    {
        Self_().Insert(_pos, { _str, _n });
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return Self_().GetLength() == 0;
    }

    [[nodiscard]] const TChar* GetTerminate() const
    {
        return Self_().GetPtr() + Self_().GetLength();
    }

    [[nodiscard]] TStringView ToStringView(
        const SizeT _offset = 0) const
    {
        JAM_ASSERT(_offset <= Self_().GetLength(), "Offset out of bounds.");
        return { Self_().GetPtr() + _offset, Self_().GetLength() - _offset };
    }

    [[nodiscard]] TStringView ToStringView(
        const SizeT _offset,
        const SizeT _count) const
    {
        JAM_ASSERT(_offset <= Self_().GetLength(), "Offset out of bounds.");
        JAM_ASSERT(_count <= Self_().GetLength() - _offset, "Count exceeds string length from offset.");
        return { Self_().GetPtr() + _offset, _count };
    }

    [[nodiscard]] TChar operator[](
        const SizeT _index) const
    {
        JAM_ASSERT(_index >= 0 && _index < Self_().GetLength(), "Index out of bounds.");
        return Self_().GetPtr()[_index];
    }

    [[nodiscard]] TChar& operator[](
        const SizeT _index)
    {
        JAM_ASSERT(_index >= 0 && _index < Self_().GetLength(), "Index out of bounds.");
        return Self_().GetPtr()[_index];
    }

    [[nodiscard]] TChar Front() const
    {
        JAM_ASSERT(!IsEmpty(), "String is empty.");
        return Self_().GetPtr()[0];
    }

    [[nodiscard]] TChar& Front()
    {
        JAM_ASSERT(!IsEmpty(), "String is empty.");
        return Self_().GetPtr()[0];
    }

    [[nodiscard]] TChar Back() const
    {
        JAM_ASSERT(!IsEmpty(), "String is empty.");
        return Self_().GetPtr()[Self_().GetLength() - 1];
    }

    [[nodiscard]] TChar& Back()
    {
        JAM_ASSERT(!IsEmpty(), "String is empty.");
        return Self_().GetPtr()[Self_().GetLength() - 1];
    }

    [[nodiscard]] bool operator==(const TStringView _other) const
    {
        return ToStringView() == _other;
    }

    [[nodiscard]] bool operator==(const TChar* _str) const
    {
        return ToStringView() == _str;
    }

    [[nodiscard]] bool operator!=(const TStringView _other) const
    {
        return ToStringView() != _other;
    }

    [[nodiscard]] bool operator!=(const TChar* _str) const
    {
        return ToStringView() != _str;
    }

    /* implicit */ operator TStringView() const
    {
        return ToStringView();
    }

    TString& operator+=(const TStringView _other)
    {
        Self_().Append(_other);
        return Self_();
    }

    TString& operator+=(const TChar _c)
    {
        Append(_c);
        return Self_();
    }

    TString operator+(const TStringView _other) const
    {
        TString result(*this);
        result.Append(_other);
        return result;
    }

    TString operator+(const TChar _c) const
    {
        TString result(*this);
        result.Append(_c);
        return result;
    }

    [[nodiscard]] Optional<SizeT> Find(
        const TStringView _other,
        const SizeT       _offset = 0) const
    {
        return ToStringView().Find(_other, _offset);
    }

    [[nodiscard]] Optional<SizeT> Find(
        const TStringView _other,
        const SizeT       _offset,
        const SizeT       _count) const
    {
        return ToStringView().Find(_other, _offset, _count);
    }

    [[nodiscard]] Optional<SizeT> Find(
        const TChar _c,
        const SizeT _offset = 0) const
    {
        return ToStringView().Find(_c, _offset);
    }

    [[nodiscard]] Optional<SizeT> Find(
        const TChar _c,
        const SizeT _offset,
        const SizeT _count) const
    {
        return ToStringView().Find(_c, _offset, _count);
    }

    [[nodiscard]] Optional<SizeT> RFind(
        const TStringView _other,
        const SizeT       _offset = 0) const
    {
        return ToStringView().RFind(_other, _offset);
    }

    [[nodiscard]] Optional<SizeT> RFind(
        const TStringView _other,
        const SizeT       _offset,
        const SizeT       _count) const
    {
        return ToStringView().RFind(_other, _offset, _count);
    }

    [[nodiscard]] Optional<SizeT> RFind(
        const TChar _c,
        const SizeT _offset = 0) const
    {
        return ToStringView().RFind(_c, _offset);
    }

    [[nodiscard]] Optional<SizeT> RFind(
        const TChar _c,
        const SizeT _offset,
        const SizeT _count) const
    {
        return ToStringView().RFind(_c, _offset, _count);
    }

    [[nodiscard]] bool StartsWith(
        const TStringView _other) const
    {
        return ToStringView().StartsWith(_other);
    }

    [[nodiscard]] bool StartsWith(
        const TChar _c) const
    {
        return ToStringView().StartsWith(_c);
    }

    [[nodiscard]] bool EndsWith(
        const TStringView _other) const
    {
        return ToStringView().EndsWith(_other);
    }

    [[nodiscard]] bool EndsWith(
        const TChar _c) const
    {
        return ToStringView().EndsWith(_c);
    }

    [[nodiscard]] bool CompareEqual(
        const TStringView _other,
        const SizeT       _offset = 0) const
    {
        return ToStringView().CompareEqual(_other, _offset);
    }

    [[nodiscard]] bool CompareEqual(
        const TChar _c,
        const SizeT _offset = 0) const
    {
        return ToStringView().CompareEqual(_c, _offset);
    }

    [[nodiscard]] bool CompareEqualIgnoreCase(
        const TStringView _other,
        const SizeT       _offset = 0) const
    {
        return ToStringView().CompareEqualIgnoreCase(_other, _offset);
    }

    [[nodiscard]] bool CompareEqualIgnoreCase(
        const TChar _c,
        const SizeT _offset = 0) const
    {
        return ToStringView().CompareEqualIgnoreCase(_c, _offset);
    }

    [[nodiscard]] SizeT Compare(
        const TStringView _other,
        const SizeT       _offset = 0) const
    {
        return ToStringView().Compare(_other, _offset);
    }

    [[nodiscard]] SizeT Compare(
        const TStringView _other,
        const SizeT       _offset,
        const SizeT       _count) const
    {
        return ToStringView().Compare(_other, _offset, _count);
    }

    [[nodiscard]] SizeT Compare(
        const TChar _c,
        const SizeT _offset = 0) const
    {
        return ToStringView().Compare(_c, _offset);
    }

    [[nodiscard]] TStringView SubView(
        const SizeT _pos) const
    {
        return ToStringView().SubView(_pos);
    }

    [[nodiscard]] TStringView SubView(
        const SizeT _pos,
        const SizeT _len) const
    {
        return ToStringView().SubView(_pos, _len);
    }

    [[nodiscard]] TStringView LTrimmed() const
    {
        return ToStringView().LTrimmed();
    }

    [[nodiscard]] TStringView RTrimmed() const
    {
        return ToStringView().RTrimmed();
    }

    [[nodiscard]] TStringView Trimmed() const
    {
        return ToStringView().Trimmed();
    }

    void Replace(const SizeT _pos, const SizeT _len, const TStringView _replacement)
    {
        const SizeT replacementLen = _replacement.GetLength();
        const SizeT numInplace     = (_len < replacementLen) ? _len : replacementLen;

        if (numInplace > 0)
        {
            std::memcpy(Self_().GetPtr() + _pos, _replacement.GetPtr(), numInplace * sizeof(TChar));
        }

        if (_len != replacementLen)
        {
            if (_len > replacementLen)
            {
                Self_().Erase(_pos + numInplace, _len - numInplace);
            }
            else
            {
                Self_().Insert(_pos + numInplace, _replacement.GetPtr() + numInplace, replacementLen - numInplace);
            }
        }
    }

    [[nodiscard]] bool operator<(
        const TStringView _other) const
    {
        return ToStringView().Compare(_other) < 0;
    }

    [[nodiscard]] bool operator>(
        const TStringView _other) const
    {
        return ToStringView().Compare(_other) > 0;
    }

    [[nodiscard]] bool operator<=(
        const TStringView _other) const
    {
        return ToStringView().Compare(_other) <= 0;
    }

    [[nodiscard]] bool operator>=(
        const TStringView _other) const
    {
        return ToStringView().Compare(_other) >= 0;
    }

    ////////// conversion //////////

    template<typename T>
        requires std::is_arithmetic_v<T>
    [[nodiscard]] static TString ConvertedFrom(const T _value)
    {
        TChar       buffer[64];
        const SizeT len = CharTraits::AsString(_value, buffer, sizeof(buffer));
        return { buffer, len };
    }

    template<typename T>
        requires std::is_integral_v<T>
    [[nodiscard]] Optional<T> ParseAs(const Int32 _base = 10) const
    {
        return ToStringView().template ParseAs<T>(_base);
    }

    template<typename T>
        requires std::is_floating_point_v<T>
    [[nodiscard]] Optional<T> ParseAs() const
    {
        return ToStringView().template ParseAs<T>();
    }

    ////////// encode //////////

    template<IsCharT UChar>
    void Encode(
        const BasicStringView<UChar> _str)
    {
        using CharEncoder = CharEncoder<UChar, TChar>;
        const SizeT len   = CharEncoder::CalcRequiredBufferSize(_str.GetPtr(), _str.GetLength());
        Self_().Resize(len);
        CharEncoder::Encode(_str.GetPtr(), _str.GetLength(), Self_().GetPtr(), len + 1);
    }

    template<IsCharT UChar>
    void Encode(
        const UChar _c)
    {
        using UStringView = BasicStringView<UChar>;
        Encode(UStringView { &_c, 1 });
    }

    template<IsCharT UChar>
    void Encode(
        const UChar* _str)
    {
        using UStringView = BasicStringView<UChar>;
        using UCharTraits = Jam::CharTraits<UChar>;
        Encode(UStringView { _str, UCharTraits::StrLen(_str) });
    }

    template<IsCharT UChar>
    void Encode(
        const UChar* _str,
        const SizeT  _n)
    {
        using UStringView = BasicStringView<UChar>;
        Encode(UStringView { _str, _n });
    }

    template<IsCharT UChar>
    void AppendEncode(
        const BasicStringView<UChar> _str)
    {
        using CharEncoder  = CharEncoder<UChar, TChar>;
        const SizeT len    = CharEncoder::CalcRequiredBufferSize(_str.GetPtr(), _str.GetLength());
        const SizeT oldLen = Self_().GetLength();
        Self_().Resize(oldLen + len);
        CharEncoder ::Encode(_str.GetPtr(), _str.GetLength(), Self_().GetPtr() + oldLen, len + 1);
    }

    template<IsCharT UChar>
    void AppendEncode(const UChar _c)
    {
        using UStringView = BasicStringView<UChar>;
        AppendEncode(UStringView { &_c, 1 });
    }

    template<IsCharT UChar>
    void AppendEncode(
        const UChar* _str)
    {
        using UStringView = BasicStringView<UChar>;
        using UCharTraits = Jam::CharTraits<UChar>;
        AppendEncode(UStringView { _str, UCharTraits::StrLen(_str) });
    }

    template<IsCharT UChar>
    void AppendEncode(
        const UChar* _str,
        const SizeT  _n)
    {
        using UStringView = BasicStringView<UChar>;
        AppendEncode(UStringView { _str, _n });
    }

    template<IsCharT UChar>
    void InsertEncode(
        const SizeT                  _pos,
        const BasicStringView<UChar> _str)
    {
        using CharEncoder  = CharEncoder<UChar, TChar>;
        const SizeT len    = CharEncoder::CalcRequiredBufferSize(_str.GetPtr(), _str.GetLength());
        const SizeT oldLen = Self_().GetLength();
        Self_().Resize(oldLen + len);
        std::memmove(Self_().GetPtr() + _pos + len, Self_().GetPtr() + _pos, (oldLen - _pos) * sizeof(TChar));
        CharEncoder ::Encode(_str.GetPtr(), _str.GetLength(), Self_().GetPtr() + _pos, len + 1);
    }

    template<IsCharT UChar>
    void InsertEncode(
        const SizeT _pos,
        const UChar _c)
    {
        using UStringView = BasicStringView<UChar>;
        InsertEncode(_pos, UStringView { &_c, 1 });
    }

    template<IsCharT UChar>
    void InsertEncode(
        const SizeT  _pos,
        const UChar* _str)
    {
        using UCharTraits = Jam::CharTraits<UChar>;
        using UStringView = BasicStringView<UChar>;
        InsertEncode(_pos, UStringView { _str, UCharTraits::StrLen(_str) });
    }

    template<IsCharT UChar>
    void InsertEncode(
        const SizeT  _pos,
        const UChar* _str,
        const SizeT  _n)
    {
        using UStringView = BasicStringView<UChar>;
        InsertEncode(_pos, UStringView { _str, _n });
    }

    template<IsCharT UChar>
    [[nodiscard]] static TString Encoded(
        const BasicStringView<UChar> _str)
    {
        TString result = {};
        result.Encode(_str);
        return result;
    }

    /////////// format ///////////

    template<typename... Args>
    void Format(const FormatStringT<Args...> _fmt, Args&&... _args)
    {
        Self_().Clear();
        fmt::format_to(std::back_inserter(*this), _fmt, std::forward<Args>(_args)...);
    }

    template<typename... Args>
    void AppendFormat(const FormatStringT<Args...> _fmt, Args&&... _args)
    {
        fmt::format_to(std::back_inserter(*this), _fmt, std::forward<Args>(_args)...);
    }

    template<typename... Args>
    [[nodiscard]] static TString Formatted(
        const FormatStringT<Args...> _fmt,
        Args&&... _args)
    {
        TString result = {};
        result.Format(_fmt, std::forward<Args>(_args)...);
        return result;
    }

    /////////// hash //////////

    [[nodiscard]] UInt32 Hash() const
    {
        return ToStringView().Hash();
    }

    [[nodiscard]] UInt64 IgnoreCaseHash64() const
    {
        return ToStringView().IgnoreCaseHash();
    }

    /////////// file system //////////

    void Decompose(
        TStringView* _pOutDirOrNull,
        TStringView* _pOutFileNameOrNull,
        TStringView* _pOutBaseNameOrNull,
        TStringView* _pOutExtOrNull) const
    {
        ToStringView().Decompose(_pOutDirOrNull, _pOutFileNameOrNull, _pOutBaseNameOrNull, _pOutExtOrNull);
    }

    [[nodiscard]] TStringView GetDirectory() const
    {
        return ToStringView().GetDirectory();
    }

    [[nodiscard]] TStringView GetFileName() const
    {
        return ToStringView().GetFileName();
    }

    [[nodiscard]] TStringView GetBaseName() const
    {
        return ToStringView().GetBaseName();
    }

    [[nodiscard]] TStringView GetExtension() const
    {
        return ToStringView().GetExtension();
    }

    [[nodiscard]] bool IsAbsolute() const
    {
        return ToStringView().IsAbsolute();
    }

    [[nodiscard]] bool IsRelative() const
    {
        return ToStringView().IsRelative();
    }

    [[nodiscard]] bool IsDirectory() const
    {
        return ToStringView().IsDirectory();
    }

    [[nodiscard]] bool IsFile() const
    {
        return ToStringView().IsFile();
    }

    [[nodiscard]] bool MatchExtension(
        const TStringView _ext) const
    {
        return ToStringView().MatchExtension(_ext);
    }

    [[nodiscard]] bool IsBasePathOf(
        const TStringView _other) const
    {
        return ToStringView().IsBasePathOf(_other);
    }

    void ReplaceExtension(
        const TStringView _newExt)
    {
        JAM_ASSERT(IsFile(), "ReplaceExtension: only file dir can be replaced extension.");
        const PathDecomposeResult result = CharTraits::DecomposePath(Self_().GetPtr(), Self_().GetLength());
        if (result.bHasExt)
        {
            Self_().Erase(result.extPos, result.extLen);
        }

        if (!_newExt.StartsWith(CharTraits::kDot))
        {
            Append(CharTraits::kDot);
        }

        Self_().Append(_newExt);
    }

    void ReplaceBaseName(
        const TStringView _newBaseName)
    {
        const PathDecomposeResult result = CharTraits::DecomposePath(Self_().GetPtr(), Self_().GetLength());
        if (result.bHasBaseName)
        {
            Self_().Erase(result.baseNamePos, result.baseNameLen);
            Self_().Insert(result.baseNamePos, _newBaseName);
        }
        else
        {
            Self_().Insert(0, _newBaseName);
        }
    }

    void ReplaceFileName(
        const TStringView _newFileName)
    {
        const PathDecomposeResult result = CharTraits::DecomposePath(Self_().GetPtr(), Self_().GetLength());
        if (result.bHasBaseName)
        {
            Self_().Erase(result.baseNamePos, result.baseNameLen + (result.bHasExt ? result.extLen : 0));
            Self_().Insert(result.baseNamePos, _newFileName);
        }
        else
        {
            Self_().Insert(0, _newFileName);
        }
    }

    void Simplify()
    {
        // simplified dir is shorter or equal to the original dir, so it won't cause heap to grow if it's already in heap mode.
        TChar       buf[512];
        const SizeT len = CharTraits::Simplify(Self_().GetPtr(), Self_().GetLength(), buf, CountOf(buf));
        Set(buf, len);
    }

    void Join(
        const TStringView _str,
        eJoin             _type = eJoin::Auto)
    {
        _type = (_type == eJoin::Auto) ? (_str.IsDirectory() ? eJoin::Directory : eJoin::File) : _type;

        if (IsEmpty())
        {
            Self_().Set(_str);
            return;
        }

        if (!CharTraits::IsPathSeparator(Back()) && !_str.IsEmpty() && !CharTraits::IsPathSeparator(_str.Front()))
        {
            Append(CharTraits::kPathPreferredSeparator);
        }

        Self_().Append(_str);
        if (_type == eJoin::Directory && !CharTraits::IsPathSeparator(Back()))
        {
            Append(CharTraits::kPathPreferredSeparator);
        }

        if (_type == eJoin::File)
        {
            SizeT end = Self_().GetLength();
            while (end > 0 && CharTraits::IsPathSeparator(operator[](end - 1)))
            {
                --end;
            }

            Self_().Resize(end);
        }
    }

    [[nodiscard]] static TString Joined(
        const TStringView _str1,
        const TStringView _str2,
        const eJoin       _type = eJoin::Auto)
    {
        TString result(_str1);
        result.Join(_str2, _type);
        return result;
    }

    ////////// style converter //////////

    [[nodiscard]] TString ToCamelCase() const
    {
        const SizeT len        = Self_().GetLength();
        bool        bNextUpper = false;

        TString result = {};
        result.Reserve(len);
        for (Int32 i = 0; i < len; ++i)
        {
            const TChar c = operator[](i);
            if (CharTraits::IsSpace(c))
            {
                bNextUpper = true;
            }
            else
            {
                result += bNextUpper ? CharTraits::ToUpper(c) : CharTraits::ToLower(c);
                bNextUpper = false;
            }
        }

        if (!result.IsEmpty())
        {
            result[0] = CharTraits::ToLower(result[0]);
        }

        return result;
    }

    [[nodiscard]] TString ToPascalCase() const
    {
        const SizeT len        = Self_().GetLength();
        bool        bNextUpper = true;

        TString result = {};
        result.Reserve(len);
        for (Int32 i = 0; i < len; ++i)
        {
            const TChar c = operator[](i);
            if (CharTraits::IsSpace(c))
            {
                bNextUpper = true;
            }
            else
            {
                result += bNextUpper ? CharTraits::ToUpper(c) : CharTraits::ToLower(c);
                bNextUpper = false;
            }
        }
        return result;
    }

    [[nodiscard]] TString ToSnakeCase() const
    {
        const SizeT len = Self_().GetLength();

        TString result = {};
        result.Reserve(len);
        for (Int32 i = 0; i < len; ++i)
        {
            const TChar c = operator[](i);
            if (CharTraits::IsSpace(c))
            {
                if (!result.IsEmpty() && result.Back() != CharTraits::kUnderscore)
                {
                    result += CharTraits::kUnderscore;
                }
            }
            else
            {
                result += CharTraits::ToLower(c);
            }
        }

        if (!result.IsEmpty() && result.Back() == CharTraits::kUnderscore)
        {
            result.Resize(result.GetLength() - 1);
        }

        return result;
    }

    ////////// std glue //////////

    using value_type     = TChar;
    using iterator       = Iter;
    using const_iterator = ConstIter;

    void push_back(const TChar _c)
    {
        Append(_c);
    }

    [[nodiscard]] size_t size() const
    {
        return static_cast<size_t>(Self_().GetLength());
    }

    [[nodiscard]] size_t length() const
    {
        return static_cast<size_t>(Self_().GetLength());
    }

    [[nodiscard]] TChar* data()
    {
        return Self_().GetPtr();
    }

    [[nodiscard]] const TChar* data() const
    {
        return Self_().GetPtr();
    }

    [[nodiscard]] Iter begin()
    {
        return Self_().GetPtr();
    }

    [[nodiscard]] Iter end()
    {
        return Self_().GetPtr() + Self_().GetLength();
    }

    [[nodiscard]] ConstIter begin() const
    {
        return Self_().GetPtr();
    }

    [[nodiscard]] ConstIter end() const
    {
        return Self_().GetPtr() + Self_().GetLength();
    }

    [[nodiscard]] ConstIter cbegin() const
    {
        return Self_().GetPtr();
    }

    [[nodiscard]] ConstIter cend() const
    {
        return Self_().GetPtr() + Self_().GetLength();
    }

private:
    StringBase() = default;

    [[nodiscard]] TString& Self_()
    {
        return static_cast<TString&>(*this);
    }

    [[nodiscard]] const TString& Self_() const
    {
        return static_cast<const TString&>(*this);
    }

    friend TString;
};

}   // namespace Jam