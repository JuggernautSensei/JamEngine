#pragma once
#include "TypeTraits.h"
#include <EASTL/fixed_string.h>

namespace Jam
{

template<typename T>
concept IsCharT = AnyOfV<T, char, wchar_t, char8_t, char16_t, char32_t>;

///////// char traits //////////

template<typename TChar>
struct CharTraits
{
};

template<>
struct CharTraits<char>
{
    using CharT = char;

    template<typename... TArgs>
    using FormatStringT = fmt::format_string<TArgs...>;

    constexpr static CharT        kNullChar               = '\0';
    constexpr static CharT        kSpaceChars[]           = { ' ', '\t', '\n', '\r', '\v', '\f' };
    constexpr static CharT        kSlash                  = '/';
    constexpr static CharT        kDot                    = '.';
    constexpr static CharT        kColon                  = ':';
    constexpr static CharT        kUnderscore             = '_';
    constexpr static CharT        kPathPreferredSeparator = kSlash;
    constexpr static CharT        kPathSeparators[]       = { '/', '\\' };
    constexpr static CharT        kLowerA                 = 'a';
    constexpr static CharT        kUpperA                 = 'A';
    constexpr static CharT        kLowerZ                 = 'z';
    constexpr static CharT        kUpperZ                 = 'Z';
    constexpr static CharT        kCaseDiff               = kLowerA - kUpperA;
    constexpr static const CharT* kPlaceholder            = "";
};

template<>
struct CharTraits<wchar_t>
{
    using CharT = wchar_t;

    template<typename... TArgs>
    using FormatStringT = fmt::wformat_string<TArgs...>;

    constexpr static CharT        kNullChar               = L'\0';
    constexpr static CharT        kSpaceChars[]           = { L' ', L'\t', L'\n', L'\r', L'\v', L'\f' };
    constexpr static CharT        kSlash                  = L'/';
    constexpr static CharT        kDot                    = L'.';
    constexpr static CharT        kColon                  = L':';
    constexpr static CharT        kUnderscore             = L'_';
    constexpr static CharT        kPathPreferredSeparator = kSlash;
    constexpr static CharT        kPathSeparators[]       = { L'/', L'\\' };
    constexpr static CharT        kLowerA                 = L'a';
    constexpr static CharT        kUpperA                 = L'A';
    constexpr static CharT        kLowerZ                 = L'z';
    constexpr static CharT        kUpperZ                 = L'Z';
    constexpr static CharT        kCaseDiff               = kLowerA - kUpperA;
    constexpr static const CharT* kPlaceholder            = L"";
};

////////// numeric char converter //////////

template<typename TChar>
struct NumericCharConverter
{
};

template<>
struct NumericCharConverter<char>
{
    using CharTraits = CharTraits<char>;
    using CharT      = CharTraits::CharT;

    [[nodiscard]] static Int32 ParseAsInt32(const CharT* _str, const Int32 _base = 10)
    {
        return std::strtol(_str, nullptr, _base);
    }

    [[nodiscard]] static UInt32 ParseAsUInt32(const CharT* _str, const Int32 _base = 10)
    {
        return std::strtoul(_str, nullptr, _base);
    }

    [[nodiscard]] static Int64 ParseAsInt64(const CharT* _str, const Int32 _base = 10)
    {
        return std::strtoll(_str, nullptr, _base);
    }

    [[nodiscard]] static UInt64 ParseAsUInt64(const CharT* _str, const Int32 _base = 10)
    {
        return std::strtoull(_str, nullptr, _base);
    }

    [[nodiscard]] static float ParseAsFloat(const CharT* _str)
    {
        return std::strtof(_str, nullptr);
    }

    [[nodiscard]] static double ParseAsDouble(const CharT* _str)
    {
        return std::strtod(_str, nullptr);
    }

    [[nodiscard]] static long double ParseAsLongDouble(const CharT* _str)
    {
        return std::strtold(_str, nullptr);
    }

    template<typename T>
        requires std::is_arithmetic_v<T>
    static SizeT ConvertToChars(const T _value, CharT* _pOutBuf, const SizeT _bufCap)
    {
        const auto [it, _] = fmt::format_to_n(_pOutBuf, _bufCap - 1, "{}", +_value);
        *it                = CharTraits::kNullChar;
        return static_cast<SizeT>(it - _pOutBuf);
    }
};

template<>
struct NumericCharConverter<wchar_t>
{
    using CharTraits = CharTraits<wchar_t>;
    using CharT      = CharTraits::CharT;

    [[nodiscard]] static Int32 ParseAsInt32(const CharT* _str, const Int32 _base = 10)
    {
        return std::wcstol(_str, nullptr, _base);
    }

    [[nodiscard]] static UInt32 ParseAsUInt32(const CharT* _str, const Int32 _base = 10)
    {
        return std::wcstoul(_str, nullptr, _base);
    }

    [[nodiscard]] static Int64 ParseAsInt64(const CharT* _str, const Int32 _base = 10)
    {
        return std::wcstoll(_str, nullptr, _base);
    }
    [[nodiscard]] static UInt64 ParseAsUInt64(const CharT* _str, const Int32 _base = 10)
    {
        return std::wcstoull(_str, nullptr, _base);
    }

    [[nodiscard]] static float ParseAsFloat(const CharT* _str)
    {
        return std::wcstof(_str, nullptr);
    }

    [[nodiscard]] static double ParseAsDouble(const CharT* _str)
    {
        return std::wcstod(_str, nullptr);
    }

    [[nodiscard]] static long double ParseAsLongDouble(const CharT* _str)
    {
        return std::wcstold(_str, nullptr);
    }

    template<typename T>
        requires std::is_arithmetic_v<T>
    static SizeT ConvertToChars(const T _value, CharT* _pOutBuf, const SizeT _bufCap)
    {
        const auto [it, _] = fmt::format_to_n(_pOutBuf, _bufCap - 1, L"{}", +_value);
        *it                = CharTraits::kNullChar;
        return static_cast<SizeT>(it - _pOutBuf);
    }
};

////////// general char functions //////////

struct CharResult
{
    constexpr CharResult() = default;
    /* implicit */ constexpr CharResult(const SizeT _index)
        : bSucceeded(true)
        , pos(_index)
    {
    }

    constexpr static CharResult Failed()
    {
        return CharResult {};
    }

    [[nodiscard]] constexpr bool IsSucceeded() const
    {
        return bSucceeded;
    }

    [[nodiscard]] constexpr bool IsFailed() const
    {
        return !bSucceeded;
    }

    bool  bSucceeded = false;
    SizeT pos        = -1;
};

struct TrimResult   // [begin, end)
{
    SizeT pos = 0;
    SizeT len = 0;
};

enum class eJoin
{
    File,        // ex) Join("C:/aaa/bbb", "ccc") -> "C:/aaa/bbb/ccc"
    Directory,   // ex) Join("C:/aaa/bbb", "ccc", OpenedDirectory) -> "C:/aaa/bbb/ccc/"
    Auto         // if the joined part ends with dir separator, treat as directory, otherwise treat as file. ex) Join("C:/aaa/bbb", "ccc/") -> "C:/aaa/bbb/ccc/"
};

/// aaa/bbb/ccc.txt
/// dir         aaa/bbb/
/// filename    ccc
/// ext         .txt
struct PathDecomposeResult
{
    bool bHasDir      = false;
    bool bHasBaseName = false;
    bool bHasExt      = false;

    SizeT dirLen      = 0;
    SizeT baseNamePos = 0;
    SizeT baseNameLen = 0;
    SizeT extPos      = 0;
    SizeT extLen      = 0;
};

constexpr SizeT kMaxPathLength = 1024;

////////// char functions //////////

template<typename TChar>
struct CharFuncs
{
    using CharTraits           = CharTraits<TChar>;
    using CharT                = CharTraits::CharT;
    using NumericCharConverter = NumericCharConverter<CharT>;
    using StdCharTraits        = std::char_traits<TChar>;
    using StdStringView        = std::basic_string_view<TChar>;

    static constexpr bool IsUpper(const CharT _c)
    {
        return (_c >= CharTraits::kUpperA) && (_c <= CharTraits::kUpperZ);
    }

    static constexpr bool IsLower(const CharT _c)
    {
        return (_c >= CharTraits::kLowerA) && (_c <= CharTraits::kLowerZ);
    }

    static constexpr bool IsAlphabet(const CharT _c)
    {
        return IsUpper(_c) || IsLower(_c);
    }

    static constexpr CharT ToUpper(const CharT _c)
    {
        return IsLower(_c) ? static_cast<CharT>(_c - CharTraits::kCaseDiff) : _c;
    }

    static constexpr CharT ToLower(const CharT _c)
    {
        return IsUpper(_c) ? static_cast<CharT>(_c + CharTraits::kCaseDiff) : _c;
    }

    static constexpr bool IsSpace(const CharT _c)
    {
        for (const CharT spaceChar: CharTraits::kSpaceChars)
        {
            if (_c == spaceChar)
            {
                return true;
            }
        }
        return false;
    }

    static constexpr SizeT StrLen(const CharT* _str)
    {
        JAM_ASSERT(_str, "StrLen: _str cannot be null.");
        return StdCharTraits::length(_str);
    }

    static constexpr const CharT* FindTerminate(const CharT* _str)
    {
        JAM_ASSERT(_str, "FindTerminate: _str cannot be null.");
        return _str + StrLen(_str);
    }

    static constexpr SizeT StrCpy(CharT* _pOutBuf, const SizeT _bufCap, const CharT* _str, const SizeT _len)
    {
        JAM_ASSERT(_pOutBuf, "StrCpy: _pOutBuf cannot be null.");
        JAM_ASSERT(_bufCap > 0, "StrCpy: _bufCap must be greater than zero.");
        JAM_ASSERT(_str, "StrCpy: _str cannot be null.");
        JAM_ASSERT(_len <= _bufCap - 1, "StrCpy: _len cannot be greater than _bufCap - 1.");

        const StdStringView str = { _str, static_cast<size_t>(_len) };
        str.copy(_pOutBuf, _len);
        _pOutBuf[_len] = CharTraits::kNullChar;
        return _len;
    }

    template<bool bEqual>
    static constexpr bool CharCompareEqualIgnoreCase(const CharT _c0, const CharT _c1)
    {
        if constexpr (bEqual)
        {
            return ToLower(_c0) == ToLower(_c1);
        }
        else
        {
            return ToLower(_c0) != ToLower(_c1);
        }
    }

    template<bool bEqual, bool bIgnoreCase>
    static constexpr bool StrEqual(const CharT* _str0, const SizeT _len0, const CharT* _str1, const SizeT _len1, const SizeT _n)
    {
        JAM_ASSERT(_str0, "StrEqual: _str0 cannot be null.");
        JAM_ASSERT(_str1, "StrEqual: _str1 cannot be null.");
        JAM_ASSERT(_len0 >= _n, "StrEqual: _len0 cannot be less than _n.");
        JAM_ASSERT(_len1 >= _n, "StrEqual: _len1 cannot be less than _n.");

        if constexpr (!bIgnoreCase)
        {
            if constexpr (bEqual)
            {
                return StdCharTraits::compare(_str0, _str1, _n) == 0;
            }
            else
            {
                return StdCharTraits::compare(_str0, _str1, _n) != 0;
            }
        }
        else
        {
            for (SizeT i = 0; i < _n; ++i)
            {
                if (!CharCompareEqualIgnoreCase<bEqual>(_str0[i], _str1[i]))
                {
                    return false;
                }
            }
            return true;
        }
    }

    template<bool bEqual, bool bIgnoreCase>
    static constexpr CharResult StrFind(const CharT* _str, const SizeT _len, const CharT* _findStr, const SizeT _findStrLen)
    {
        JAM_ASSERT(_str, "StrFind: _str cannot be null.");
        JAM_ASSERT(_findStr, "StrFind: _findStr cannot be null.");
        JAM_ASSERT(_len >= _findStrLen, "StrFind: _len cannot be less than _findStrLen.");

        if constexpr (!bIgnoreCase)
        {
            if constexpr (bEqual)
            {
                const StdStringView str     = { _str, static_cast<size_t>(_len) };
                const StdStringView findStr = { _findStr, static_cast<size_t>(_findStrLen) };
                const size_t        idx     = str.find(findStr);
                return idx != StdStringView::npos ? CharResult { static_cast<SizeT>(idx) } : CharResult::Failed();
            }
            else
            {
                const StdStringView str     = { _str, static_cast<size_t>(_len) };
                const StdStringView findStr = { _findStr, static_cast<size_t>(_findStrLen) };
                const size_t        idx     = str.find_first_not_of(findStr);
                return idx != StdStringView::npos ? CharResult { static_cast<SizeT>(idx) } : CharResult::Failed();
            }
        }
        else
        {
            for (SizeT i = 0; i <= _len - _findStrLen; ++i)
            {
                if (StrEqual<bEqual, bIgnoreCase>(_str + i, _len - i, _findStr, _findStrLen, _findStrLen))
                {
                    return CharResult { i };
                }
            }
            return CharResult::Failed();
        }
    }

    template<bool bEqual, bool bIgnoreCase>
    static constexpr CharResult StrRFind(const CharT* _str, const SizeT _len, const CharT* _findStr, const SizeT _findStrLen)
    {
        JAM_ASSERT(_str, "StrRFind: _str cannot be null.");
        JAM_ASSERT(_findStr, "StrRFind: _findStr cannot be null.");
        JAM_ASSERT(_len >= _findStrLen, "StrRFind: _len cannot be less than _findStrLen.");

        if constexpr (!bIgnoreCase)
        {
            if constexpr (bEqual)
            {
                const StdStringView str     = { _str, static_cast<size_t>(_len) };
                const StdStringView findStr = { _findStr, static_cast<size_t>(_findStrLen) };
                const size_t        idx     = str.rfind(findStr);
                return idx != StdStringView::npos ? CharResult { static_cast<SizeT>(idx) } : CharResult::Failed();
            }
            else
            {
                const StdStringView str     = { _str, static_cast<size_t>(_len) };
                const StdStringView findStr = { _findStr, static_cast<size_t>(_findStrLen) };
                const size_t        idx     = str.find_last_not_of(findStr);
                return idx != StdStringView::npos ? CharResult { static_cast<SizeT>(idx) } : CharResult::Failed();
            }
        }
        else
        {
            for (SizeT i = _len - _findStrLen + 1; i > 0; --i)
            {
                if (StrEqual<bEqual, bIgnoreCase>(_str + i - 1, _len - (i - 1), _findStr, _findStrLen, _findStrLen))
                {
                    return CharResult { i - 1 };
                }
            }
            return CharResult::Failed();
        }
    }

    static constexpr SizeT StrLTrim(const CharT* _pStart, const SizeT _len)
    {
        for (SizeT i = 0; i < _len; ++i)
        {
            if (!IsSpace(_pStart[i]))
            {
                return i;
            }
        }
        return _len;
    }

    static constexpr SizeT StrRTrim(const CharT* _pStart, const SizeT _len)
    {
        for (SizeT i = _len; i > 0; --i)
        {
            if (!IsSpace(_pStart[i - 1]))
            {
                return i;
            }
        }
        return 0;
    }

    static constexpr TrimResult StrTrim(const CharT* _pStart, const SizeT _len)
    {
        JAM_ASSERT(_pStart, "StrTrim: _pStart cannot be null.");
        const SizeT lTrimmed = StrLTrim(_pStart, _len);
        const SizeT rTrimmed = StrRTrim(_pStart + lTrimmed, _len - lTrimmed);
        return { lTrimmed, rTrimmed };
    }

    static constexpr SizeT StrCompare(const CharT* _str0, const SizeT _len0, const CharT* _str1, const SizeT _len1)
    {
        JAM_ASSERT(_str0, "StrCompare: _str0 cannot be null.");
        JAM_ASSERT(_str1, "StrCompare: _str1 cannot be null.");
        return StdCharTraits::compare(_str0, _str1, Min(_len0, _len1));
    }

    [[nodiscard]] static constexpr bool IsPathSeparator(const CharT _c)
    {
        for (const CharT separator: CharTraits::kPathSeparators)
        {
            if (_c == separator)
            {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] static constexpr PathDecomposeResult DecomposePath(const CharT* _str, const SizeT _len)
    {
        JAM_ASSERT(_str, "DecomposePath: _str cannot be null.");

        if (_len == 0)
        {
            return {};
        }

        const bool          bIsDir = IsPathSeparator(_str[_len - 1]);
        const SizeT         end    = bIsDir ? _len - 1 : _len;
        PathDecomposeResult result = {};

        for (SizeT i = end; i > 0; --i)
        {
            CharT c = _str[i - 1];
            if (IsPathSeparator(c))
            {
                result.bHasDir      = true;
                result.dirLen       = i;
                result.bHasBaseName = true;
                result.baseNamePos  = i;
                result.baseNameLen  = result.bHasExt ? result.extPos - i : end - i;
                break;
            }
            else if (c == CharTraits::kDot && !result.bHasExt)
            {
                result.bHasExt = true;
                result.extPos  = i - 1;
                result.extLen  = end - result.extPos;
            }
        }

        if (!result.bHasDir)
        {
            if (end > 0)
            {
                result.bHasBaseName = true;
                result.baseNamePos  = 0;
                result.baseNameLen  = result.bHasExt ? result.extPos : end;
            }
        }

        return result;
    }

    [[nodiscard]] static constexpr SizeT Simplify(const CharT* _str, const SizeT _len, CharT* _pOutBuf, const SizeT _bufCap)
    {
        JAM_ASSERT(_str, "Simplify: _str cannot be null.");
        JAM_ASSERT(_pOutBuf, "Simplify: _pOutBuf cannot be null.");
        JAM_ASSERT(_bufCap > 0, "Simplify: _bufCap must be greater than zero.");
        JAM_ASSERT(_len < _bufCap, "Simplify: _len must be less than _bufCap.");

        if (_len == 0)
        {
            _pOutBuf[0] = CharTraits::kNullChar;
            return 0;
        }

        SizeT idx    = 0;
        SizeT dotdot = 0;
        SizeT outIdx = 0;

        if (_len >= 2 && IsAlphabet(_str[0]) && _str[1] == CharTraits::kColon)
        {
            _pOutBuf[outIdx++] = ToUpper(_str[0]);
            _pOutBuf[outIdx++] = CharTraits::kColon;
            idx += 2;
            dotdot = outIdx;
        }

        const bool bIsRoot = (idx < _len) && IsPathSeparator(_str[idx]);
        if (bIsRoot)
        {
            _pOutBuf[outIdx++] = CharTraits::kPathPreferredSeparator;
            ++idx;
            dotdot = outIdx;
        }

        bool bTrailingSlash = false;
        while (idx < _len)
        {
            if (IsPathSeparator(_str[idx]))
            {
                ++idx;
                bTrailingSlash = (idx == _len);
                continue;
            }

            if (_str[idx] == CharTraits::kDot)
            {
                if (idx + 1 == _len || IsPathSeparator(_str[idx + 1]))
                {
                    ++idx;
                    continue;
                }

                if (CharTraits::kDot == _str[idx + 1] && (idx + 2 == _len || IsPathSeparator(_str[idx + 2])))
                {
                    idx += 2;
                    if (dotdot < outIdx)
                    {
                        SizeT i = outIdx - 1;
                        for (; i > dotdot && !IsPathSeparator(_pOutBuf[i]); --i)
                        {
                        }
                        outIdx = i;
                    }
                    else if (!bIsRoot)
                    {
                        if (outIdx > 0 && !IsPathSeparator(_pOutBuf[outIdx - 1]))
                        {
                            _pOutBuf[outIdx++] = CharTraits::kPathPreferredSeparator;
                        }
                        _pOutBuf[outIdx++] = CharTraits::kDot;
                        _pOutBuf[outIdx++] = CharTraits::kDot;
                        dotdot             = outIdx;
                    }
                    continue;
                }
            }

            if ((bIsRoot && outIdx > dotdot) || (!bIsRoot && outIdx > 0))
            {
                if (outIdx > 0 && !IsPathSeparator(_pOutBuf[outIdx - 1]))
                {
                    _pOutBuf[outIdx++] = CharTraits::kPathPreferredSeparator;
                }
            }

            for (; idx < _len && !IsPathSeparator(_str[idx]); ++idx)
            {
                _pOutBuf[outIdx++] = _str[idx];
            }
        }

        if (outIdx == 0)
        {
            _pOutBuf[outIdx++] = CharTraits::kDot;
        }
        else if (bTrailingSlash && outIdx > 0 && !IsPathSeparator(_pOutBuf[outIdx - 1]))
        {
            _pOutBuf[outIdx++] = CharTraits::kPathPreferredSeparator;
        }

        _pOutBuf[outIdx] = CharTraits::kNullChar;
        return outIdx;
    }

    template<typename T>
        requires std::is_integral_v<T>
    [[nodiscard]] static Optional<T> ParseAs(const CharT* _str, const Int32 _base = 10)
    {
        JAM_ASSERT(_str, "ParseAs: _str cannot be null.");

        if constexpr (std::is_same_v<T, Int32> || std::is_same_v<T, Int16> || std::is_same_v<T, Int8> || std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
        {
            const Int32 value = NumericCharConverter::ParseAsInt32(_str, _base);
            return static_cast<T>(value);
        }
        else if constexpr (std::is_same_v<T, UInt32> || std::is_same_v<T, UInt16> || std::is_same_v<T, UInt8> || std::is_same_v<T, unsigned char>)
        {
            const UInt32 value = NumericCharConverter::ParseAsUInt32(_str, _base);
            return static_cast<T>(value);
        }
        else if constexpr (std::is_same_v<T, Int64>)
        {
            const Int64 value = NumericCharConverter::ParseAsInt64(_str, _base);
            return static_cast<T>(value);
        }
        else if constexpr (std::is_same_v<T, UInt64>)
        {
            const UInt64 value = NumericCharConverter::ParseAsUInt64(_str, _base);
            return static_cast<T>(value);
        }

        JAM_UNREACHABLE;
    }

    template<typename T>
        requires std::is_floating_point_v<T>
    [[nodiscard]] static Optional<T> ParseAs(const CharT* _str)
    {
        JAM_ASSERT(_str, "ParseAs: _str cannot be null.");

        if constexpr (std::is_same_v<T, float>)
        {
            const float value = NumericCharConverter::ParseAsFloat(_str);
            if (value == HUGE_VALF || value == -HUGE_VALF)
            {
                return kNullopt;
            }
            return static_cast<T>(value);
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            const double value = NumericCharConverter::ParseAsDouble(_str);
            if (value == HUGE_VAL || value == -HUGE_VAL)
            {
                return kNullopt;
            }
            return static_cast<T>(value);
        }
        else if constexpr (std::is_same_v<T, long double>)
        {
            const long double value = NumericCharConverter::ParseAsLongDouble(_str);
            if (value == HUGE_VALL || value == -HUGE_VALL)
            {
                return kNullopt;
            }
            return static_cast<T>(value);
        }

        JAM_UNREACHABLE;
    }

    template<typename T>
        requires std::is_arithmetic_v<T>
    constexpr static SizeT ConvertToChars(const T _value, CharT* _pOutBuf, const SizeT _bufCap)
    {
        return NumericCharConverter::ConvertToChars(_value, _pOutBuf, _bufCap);
    }
};

/////////// utf8 / utf16 conversion ///////////

template<typename TFromChar, typename TToChar>
struct CharEncoder
{
    static_assert(AlwaysFalseV<TFromChar>, "CharEncoder: Unsupported character types.");
    static_assert(AlwaysFalseV<TToChar>, "CharEncoder: Unsupported character types.");
};

template<>
struct CharEncoder<char, wchar_t>
{
    [[nodiscard]] static SizeT CalcRequiredBufferSize(const char* _str, const SizeT _len)
    {
        return ::MultiByteToWideChar(CP_UTF8, 0, _str, static_cast<Int32>(_len), nullptr, 0);
    }

    static SizeT Encode(const char* _str, const SizeT _len, wchar_t* _pOutBuf, const SizeT _bufCap)
    {
        JAM_ASSERT(_str, "CharEncoder<char, wchar_t>::Encode: _str cannot be null.");
        JAM_ASSERT(_pOutBuf, "CharEncoder<char, wchar_t>::Encode: _pOutBuf cannot be null.");
        JAM_ASSERT(_bufCap > 0, "CharEncoder<char, wchar_t>::Encode: _bufCap must be greater than zero.");

        const Int32 n = ::MultiByteToWideChar(CP_UTF8, 0, _str, static_cast<Int32>(_len), _pOutBuf, static_cast<Int32>(_bufCap));
        _pOutBuf[n]   = L'\0';
        return n;
    }
};

template<>
struct CharEncoder<wchar_t, char>
{
    [[nodiscard]] static SizeT CalcRequiredBufferSize(const wchar_t* _str, const SizeT _len)
    {
        return ::WideCharToMultiByte(CP_UTF8, 0, _str, static_cast<Int32>(_len), nullptr, 0, nullptr, nullptr);
    }

    static SizeT Encode(const wchar_t* _str, const SizeT _len, char* _pOutBuf, const SizeT _bufCap)
    {
        JAM_ASSERT(_str, "CharEncoder<wchar_t, char>::Encode: _str cannot be null.");
        JAM_ASSERT(_pOutBuf, "CharEncoder<wchar_t, char>::Encode: _pOutBuf cannot be null.");
        JAM_ASSERT(_bufCap > 0, "CharEncoder<wchar_t, char>::Encode: _bufCap must be greater than zero.");

        const Int32 n = ::WideCharToMultiByte(CP_UTF8, 0, _str, static_cast<Int32>(_len), _pOutBuf, static_cast<Int32>(_bufCap), nullptr, nullptr);
        _pOutBuf[n]   = '\0';
        return n;
    }
};

template<IsCharT TChar>
struct CharEncoder<TChar, TChar>
{
    [[nodiscard]] static SizeT CalcRequiredBufferSize(const TChar* _str, const SizeT _len)
    {
        return _len;
    }

    static SizeT Encode(const TChar* _str, const SizeT _len, TChar* _pOutBuf, const SizeT _bufCap)
    {
        JAM_ASSERT(_str, "CharEncoder<TChar, TChar>::Encode: _str cannot be null.");
        JAM_ASSERT(_pOutBuf, "CharEncoder<TChar, TChar>::Encode: _pOutBuf cannot be null.");
        JAM_ASSERT(_bufCap > 0, "CharEncoder<TChar, TChar>::Encode: _bufCap must be greater than zero.");
        JAM_ASSERT(_len < _bufCap, "CharEncoder<TChar, TChar>::Encode: _len must be less than _bufCap.");

        std::memcpy(_pOutBuf, _str, _len * sizeof(TChar));
        _pOutBuf[_len] = CharTraits<TChar>::kNullChar;
        return _len;
    }
};

////////// string hashers //////////

namespace Details
{
    constexpr UInt32 kFnv1ah32Prime = 16777619U;
    constexpr UInt32 kFnv1ah32Seed  = 2166136261UL;
    constexpr UInt64 kFnv1ah64Prime = 1099511628211ULL;
    constexpr UInt64 kFnv1ah64Seed  = 14695981039346656037ULL;
}   // namespace Details

// constexpr hash for stringt
template<typename TChar>
class Fnv1ahHasher32
{
public:
    explicit constexpr Fnv1ahHasher32(
        const UInt32 _seed = Details::kFnv1ah32Seed)
        : m_hash(_seed)
    {
    }

    constexpr void Mix(
        const TChar* _str,
        const SizeT  _len)
    {
        for (SizeT i = 0; i < _len; ++i)
        {
            m_hash = (m_hash ^ static_cast<UInt32>(_str[i])) * Details::kFnv1ah32Prime;
        }
    }

    [[nodiscard]] constexpr UInt32 Finish() const
    {
        return m_hash;
    }

private:
    UInt32 m_hash = 0;
};

template<typename TChar>
class Fnv1ahHasher64
{
public:
    explicit constexpr Fnv1ahHasher64(
        const UInt64 _seed = Details::kFnv1ah64Seed)
        : m_hash(_seed)
    {
    }

    constexpr void Mix(
        const TChar* _str,
        const SizeT  _len)
    {
        for (SizeT i = 0; i < _len; ++i)
        {
            m_hash = (m_hash ^ static_cast<UInt64>(_str[i])) * Details::kFnv1ah64Prime;
        }
    }

    [[nodiscard]] constexpr UInt64 Finish() const
    {
        return m_hash;
    }

private:
    UInt64 m_hash = 0;
};

template<typename TChar>
class IgnoreCaseFnv1ahHasher32
{
public:
    using CharFuncs = CharFuncs<TChar>;

    explicit constexpr IgnoreCaseFnv1ahHasher32(
        const UInt32 _seed = Details::kFnv1ah32Seed)
        : m_hash(_seed)
    {
    }

    constexpr void Mix(
        const TChar* _str,
        const SizeT  _len)
    {
        for (SizeT i = 0; i < _len; ++i)
        {
            m_hash = (m_hash ^ static_cast<UInt32>(CharFuncs::ToLower(_str[i]))) * Details::kFnv1ah32Prime;
        }
    }

    [[nodiscard]] constexpr UInt32 Finish() const
    {
        return m_hash;
    }

private:
    UInt32 m_hash = 0;
};

template<typename TChar>
class IgnoreCaseFnv1ahHasher64
{
public:
    using CharFuncs = CharFuncs<TChar>;

    explicit constexpr IgnoreCaseFnv1ahHasher64(
        const UInt64 _seed = Details::kFnv1ah64Seed)
        : m_hash(_seed)
    {
    }

    constexpr void Mix(
        const TChar* _str,
        const SizeT  _len)
    {
        for (SizeT i = 0; i < _len; ++i)
        {
            m_hash = (m_hash ^ static_cast<UInt64>(CharFuncs::ToLower(_str[i]))) * Details::kFnv1ah64Prime;
        }
    }

    [[nodiscard]] constexpr UInt64 Finish() const
    {
        return m_hash;
    }

private:
    UInt64 m_hash = 0;
};

////////// not guaranteed null-terminated string view //////////

template<IsCharT TChar>
class BasicStringView
{
public:
    using CharTraits = CharTraits<TChar>;
    using CharT      = CharTraits::CharT;
    using CharFuncs  = CharFuncs<TChar>;
    using Iter       = const CharT*;
    using ConstIter  = const CharT*;

    constexpr BasicStringView() = default;

    /* implicit */ constexpr BasicStringView(
        const CharT* _str)
        : m_ptr(_str)
        , m_len(CharFuncs::StrLen(_str))
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
        return CharFuncs::template ParseAs<T>(m_ptr, _base);
    }

    template<typename T>
        requires std::is_floating_point_v<T>
    [[nodiscard]] constexpr Optional<T> ParseAs() const
    {
        return CharFuncs::template ParseAs<T>(m_ptr);
    }

    [[nodiscard]] constexpr CharResult Find(const BasicStringView _other, const SizeT _offset, const SizeT _count) const
    {
        JAM_ASSERT(_offset <= m_len, "Offset out of bounds.");
        JAM_ASSERT(_count <= m_len - _offset, "Count exceeds string length from offset.");
        CharResult result = CharFuncs::template StrFind<true, false>(m_ptr + _offset, _count, _other.m_ptr, _other.m_len);
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
        return Find({ &_c, 1 }, _offset, _count);
    }

    [[nodiscard]] constexpr CharResult Find(const CharT _c, const SizeT _offset = 0) const
    {
        return Find({ &_c, 1 }, _offset, m_len - _offset);
    }

    [[nodiscard]] constexpr CharResult RFind(const BasicStringView _other, const SizeT _offset, const SizeT _count) const
    {
        JAM_ASSERT(_count <= m_len, "Count exceeds length.");
        JAM_ASSERT(_offset <= m_len - _count, "Offset out of bounds for count.");
        CharResult result = CharFuncs::template StrRFind<true, false>(m_ptr + _offset, _count, _other.m_ptr, _other.m_len);
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
        return RFind({ &_c, 1 }, _offset, _count);
    }

    [[nodiscard]] constexpr CharResult RFind(const CharT _c, const SizeT _offset = 0) const
    {
        return RFind({ &_c, 1 }, _offset, m_len - _offset);
    }

    [[nodiscard]] constexpr CharResult FindNotOf(const BasicStringView _other, const SizeT _offset, const SizeT _count) const
    {
        JAM_ASSERT(_count <= m_len, "Count exceeds length.");
        JAM_ASSERT(_offset <= m_len - _count, "Offset out of bounds for count.");
        CharResult result = CharFuncs::template StrFind<false, false>(m_ptr + _offset, _count, _other.m_ptr, _other.m_len);
        if (result.IsSucceeded())
        {
            result.pos += _offset;
        }
        return result;
    }

    [[nodiscard]] constexpr CharResult FindNotOf(const BasicStringView _other, const SizeT _offset = 0) const
    {
        return FindNotOf(_other, _offset, m_len - _offset);
    }

    [[nodiscard]] constexpr CharResult FindNotOf(const CharT _c, const SizeT _offset, const SizeT _count) const
    {
        return FindNotOf({ &_c, 1 }, _offset, _count);
    }

    [[nodiscard]] constexpr CharResult FindNotOf(const CharT _c, const SizeT _offset = 0) const
    {
        return FindNotOf({ &_c, 1 }, _offset, m_len - _offset);
    }

    [[nodiscard]] constexpr CharResult RFindNotOf(const BasicStringView _other, const SizeT _offset, const SizeT _count) const
    {
        JAM_ASSERT(_count <= m_len, "Count exceeds length.");
        JAM_ASSERT(_offset <= m_len - _count, "Offset out of bounds for count.");
        CharResult result = CharFuncs::template StrRFind<false, false>(m_ptr + _offset, _count, _other.m_ptr, _other.m_len);
        if (result.IsSucceeded())
        {
            result.pos += _offset;
        }
        return result;
    }

    [[nodiscard]] constexpr CharResult RFindNotOf(const BasicStringView _other, const SizeT _offset = 0) const
    {
        return RFindNotOf(_other, _offset, m_len - _offset);
    }

    [[nodiscard]] constexpr CharResult RFindNotOf(const CharT _c, const SizeT _offset, const SizeT _count) const
    {
        return RFindNotOf({ &_c, 1 }, _offset, _count);
    }

    [[nodiscard]] constexpr CharResult RFindNotOf(const CharT _c, const SizeT _offset = 0) const
    {
        return RFindNotOf({ &_c, 1 }, _offset, m_len - _offset);
    }

    [[nodiscard]] constexpr bool CompareEqual(const BasicStringView _other, const SizeT _offset = 0) const
    {
        JAM_ASSERT(_offset <= m_len, "Offset out of bounds.");
        if (m_len - _offset != _other.m_len)
        {
            return false;
        }

        return CharFuncs::template StrEqual<true, false>(m_ptr + _offset, m_len - _offset, _other.m_ptr, _other.m_len, _other.m_len);
    }

    [[nodiscard]] constexpr bool CompareEqual(const CharT _c, const SizeT _offset = 0) const
    {
        return CompareEqual({ &_c, 1 }, _offset);
    }

    [[nodiscard]] constexpr bool CompareEqualIgnoreCase(const BasicStringView _other, const SizeT _offset = 0) const
    {
        JAM_ASSERT(_offset <= m_len, "Offset out of bounds.");
        if (m_len - _offset != _other.m_len)
        {
            return false;
        }

        return CharFuncs::template StrEqual<true, true>(m_ptr + _offset, m_len - _offset, _other.m_ptr, _other.m_len, _other.m_len);
    }

    [[nodiscard]] constexpr bool CompareEqualIgnoreCase(const CharT _c, const SizeT _offset = 0) const
    {
        return CompareEqualIgnoreCase({ &_c, 1 }, _offset);
    }

    [[nodiscard]] constexpr SizeT Compare(const BasicStringView _other, const SizeT _offset = 0) const
    {
        JAM_ASSERT(_offset <= m_len, "Offset out of bounds.");
        return CharFuncs::template StrCompare(m_ptr + _offset, m_len - _offset, _other.m_ptr, _other.m_len);
    }

    [[nodiscard]] constexpr SizeT Compare(const BasicStringView _other, const SizeT _offset, const SizeT _count) const
    {
        return SubView(_offset, _count).Compare(_other);
    }

    [[nodiscard]] constexpr SizeT Compare(const CharT _c, const SizeT _offset = 0) const
    {
        return Compare({ &_c, 1 }, _offset);
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
        const SizeT lTrimmed = CharFuncs::StrLTrim(m_ptr, m_len);
        return SubView(lTrimmed);
    }

    [[nodiscard]] BasicStringView RTrimmed() const
    {
        const SizeT rTrimmed = CharFuncs::StrRTrim(m_ptr, m_len);
        return SubView(0, rTrimmed);
    }

    [[nodiscard]] BasicStringView Trimmed() const
    {
        const auto [lTrimmed, trimmedLen] = CharFuncs::StrTrim(m_ptr, m_len);
        return SubView(lTrimmed, trimmedLen);
    }

    [[nodiscard]] constexpr UInt32 Hash32() const
    {
        Fnv1ahHasher32<CharT> hasher {};
        hasher.Mix(m_ptr, m_len);
        return hasher.Finish();
    }

    [[nodiscard]] constexpr UInt32 IgnoreCaseHash32() const
    {
        IgnoreCaseFnv1ahHasher32<CharT> hasher {};
        hasher.Mix(m_ptr, m_len);
        return hasher.Finish();
    }

    [[nodiscard]] constexpr UInt64 Hash64() const
    {
        Fnv1ahHasher64<CharT> hasher {};
        hasher.Mix(m_ptr, m_len);
        return hasher.Finish();
    }

    [[nodiscard]] constexpr UInt64 IgnoreCaseHash64() const
    {
        IgnoreCaseFnv1ahHasher64<CharT> hasher {};
        hasher.Mix(m_ptr, m_len);
        return hasher.Finish();
    }

    [[nodiscard]] constexpr static UInt32 Hashed32(
        const BasicStringView _str)
    {
        return _str.Hash32();
    }

    [[nodiscard]] constexpr static UInt32 IgnoreCaseHashed32(
        const BasicStringView _str)
    {
        return _str.IgnoreCaseHash32();
    }

    [[nodiscard]] constexpr static UInt64 Hashed64(
        const BasicStringView _str)
    {
        return _str.Hash64();
    }

    [[nodiscard]] constexpr static UInt64 IgnoreCaseHashed64(
        const BasicStringView _str)
    {
        return _str.IgnoreCaseHash64();
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
        const PathDecomposeResult ret = CharFuncs::DecomposePath(m_ptr, m_len);
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
        return m_len > 2 && CharFuncs::IsAlphabet(m_ptr[0]) && m_ptr[1] == CharTraits::kColon
            || m_len > 0 && CharFuncs::IsPathSeparator(m_ptr[0]);
    }

    [[nodiscard]] bool IsRelative() const
    {
        return !IsAbsolute();
    }

    [[nodiscard]] bool IsDirectory() const
    {
        return !IsEmpty() && CharFuncs::IsPathSeparator(Back());
    }

    [[nodiscard]] bool IsFile() const
    {
        return !IsEmpty() && !CharFuncs::IsPathSeparator(Back());
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

    [[nodiscard]] constexpr std::basic_string<CharT> ToStdString() const
    {
        return { m_ptr, static_cast<size_t>(m_len) };
    }

    [[nodiscard]] constexpr std::basic_string_view<CharT> ToStdStringView() const
    {
        return { m_ptr, static_cast<size_t>(m_len) };
    }

private:
    const CharT* m_ptr = CharTraits::kPlaceholder;   // DefaultExport empty
    SizeT        m_len = 0;
};

using StringView  = BasicStringView<char>;
using WStringView = BasicStringView<wchar_t>;

////////// string-base //////////

template<typename TString,
         typename TChar,
         typename TStringView = BasicStringView<TChar>,
         typename CharFuncs   = CharFuncs<TChar>,
         typename CharTraits  = CharTraits<TChar>>
class StringUtilities
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
        Self_().Append({ _str, CharFuncs::StrLen(_str) });
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
        Self_().Insert(_pos, { _str, CharFuncs::StrLen(_str) });
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

    [[nodiscard]] CharResult Find(
        const TStringView _other,
        const SizeT       _offset = 0) const
    {
        return ToStringView().Find(_other, _offset);
    }

    [[nodiscard]] CharResult Find(
        const TStringView _other,
        const SizeT       _offset,
        const SizeT       _count) const
    {
        return ToStringView().Find(_other, _offset, _count);
    }

    [[nodiscard]] CharResult Find(
        const TChar _c,
        const SizeT _offset = 0) const
    {
        return ToStringView().Find(_c, _offset);
    }

    [[nodiscard]] CharResult Find(
        const TChar _c,
        const SizeT _offset,
        const SizeT _count) const
    {
        return ToStringView().Find(_c, _offset, _count);
    }

    [[nodiscard]] CharResult RFind(
        const TStringView _other,
        const SizeT       _offset = 0) const
    {
        return ToStringView().RFind(_other, _offset);
    }

    [[nodiscard]] CharResult RFind(
        const TStringView _other,
        const SizeT       _offset,
        const SizeT       _count) const
    {
        return ToStringView().RFind(_other, _offset, _count);
    }

    [[nodiscard]] CharResult RFind(
        const TChar _c,
        const SizeT _offset = 0) const
    {
        return ToStringView().RFind(_c, _offset);
    }

    [[nodiscard]] CharResult RFind(
        const TChar _c,
        const SizeT _offset,
        const SizeT _count) const
    {
        return ToStringView().RFind(_c, _offset, _count);
    }

    [[nodiscard]] CharResult FindNotOf(
        const TStringView _other,
        const SizeT       _offset = 0) const
    {
        return ToStringView().FindNotOf(_other, _offset);
    }

    [[nodiscard]] CharResult FindNotOf(
        const TStringView _other,
        const SizeT       _offset,
        const SizeT       _count) const
    {
        return ToStringView().FindNotOf(_other, _offset, _count);
    }

    [[nodiscard]] CharResult FindNotOf(
        const TChar _c,
        const SizeT _offset = 0) const
    {
        return ToStringView().FindNotOf(_c, _offset);
    }

    [[nodiscard]] CharResult FindNotOf(
        const TChar _c,
        const SizeT _offset,
        const SizeT _count) const
    {
        return ToStringView().FindNotOf(_c, _offset, _count);
    }

    [[nodiscard]] CharResult RFindNotOf(
        const TStringView _other,
        const SizeT       _offset = 0) const
    {
        return ToStringView().RFindNotOf(_other, _offset);
    }

    [[nodiscard]] CharResult RFindNotOf(
        const TStringView _other,
        const SizeT       _offset,
        const SizeT       _count) const
    {
        return ToStringView().RFindNotOf(_other, _offset, _count);
    }

    [[nodiscard]] CharResult RFindNotOf(
        const TChar _c,
        const SizeT _offset = 0) const
    {
        return ToStringView().RFindNotOf(_c, _offset);
    }

    [[nodiscard]] CharResult RFindNotOf(
        const TChar _c,
        const SizeT _offset,
        const SizeT _count) const
    {
        return ToStringView().RFindNotOf(_c, _offset, _count);
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
        const SizeT len = CharFuncs::ConvertToChars(_value, buffer, sizeof(buffer));
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
        using UCharFuncs  = Jam::CharFuncs<UChar>;
        Encode(UStringView { _str, UCharFuncs::StrLen(_str) });
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
        using UCharFuncs  = Jam::CharFuncs<UChar>;
        AppendEncode(UStringView { _str, UCharFuncs::StrLen(_str) });
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
        using UCharFuncs  = Jam::CharFuncs<UChar>;
        using UStringView = BasicStringView<UChar>;
        InsertEncode(_pos, UStringView { _str, UCharFuncs::StrLen(_str) });
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

    [[nodiscard]] UInt32 Hash32() const
    {
        return ToStringView().Hash32();
    }

    [[nodiscard]] UInt64 Hash64() const
    {
        return ToStringView().Hash64();
    }

    [[nodiscard]] UInt32 IgnoreCaseHash32() const
    {
        return ToStringView().IgnoreCaseHash32();
    }

    [[nodiscard]] UInt64 IgnoreCaseHash64() const
    {
        return ToStringView().IgnoreCaseHash64();
    }

    [[nodiscard]] constexpr static UInt32 Hashed32(
        const TStringView _str)
    {
        return TStringView::Hashed32(_str);
    }

    [[nodiscard]] constexpr static UInt64 Hashed64(
        const TStringView _str)
    {
        return TStringView::Hashed64(_str);
    }

    [[nodiscard]] constexpr static UInt32 IgnoreCaseHashed32(
        const TStringView _str)
    {
        return TStringView::IgnoreCaseHashed32(_str);
    }

    [[nodiscard]] constexpr static UInt64 IgnoreCaseHashed64(
        const TStringView _str)
    {
        return TStringView::IgnoreCaseHashed64(_str);
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
        const PathDecomposeResult result = CharFuncs::DecomposePath(Self_().GetPtr(), Self_().GetLength());
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
        const PathDecomposeResult result = CharFuncs::DecomposePath(Self_().GetPtr(), Self_().GetLength());
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
        const PathDecomposeResult result = CharFuncs::DecomposePath(Self_().GetPtr(), Self_().GetLength());
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
        TChar       buf[kMaxPathLength];
        const SizeT len = CharFuncs::Simplify(Self_().GetPtr(), Self_().GetLength(), buf, CountOf(buf));
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

        if (!CharFuncs::IsPathSeparator(Back()) && !_str.IsEmpty() && !CharFuncs::IsPathSeparator(_str.Front()))
        {
            Append(CharTraits::kPathPreferredSeparator);
        }

        Self_().Append(_str);
        if (_type == eJoin::Directory && !CharFuncs::IsPathSeparator(Back()))
        {
            Append(CharTraits::kPathPreferredSeparator);
        }

        if (_type == eJoin::File)
        {
            SizeT end = Self_().GetLength();
            while (end > 0 && CharFuncs::IsPathSeparator(operator[](end - 1)))
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
            if (CharFuncs::IsSpace(c))
            {
                bNextUpper = true;
            }
            else
            {
                result += bNextUpper ? CharFuncs::ToUpper(c) : CharFuncs::ToLower(c);
                bNextUpper = false;
            }
        }

        if (!result.IsEmpty())
        {
            result[0] = CharFuncs::ToLower(result[0]);
        }

        return result;
    }

    [[nodiscard]] static TString CamelCased(
        const TStringView _str)
    {
        TString result = {};
        result.Append(_str);
        return result.ToCamelCase();
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
            if (CharFuncs::IsSpace(c))
            {
                bNextUpper = true;
            }
            else
            {
                result += bNextUpper ? CharFuncs::ToUpper(c) : CharFuncs::ToLower(c);
                bNextUpper = false;
            }
        }
        return result;
    }

    [[nodiscard]] static TString PascalCased(
        const TStringView _str)
    {
        TString result = {};
        result.Append(_str);
        return result.ToPascalCase();
    }

    [[nodiscard]] TString ToSnakeCase() const
    {
        const SizeT len = Self_().GetLength();

        TString result = {};
        result.Reserve(len);
        for (Int32 i = 0; i < len; ++i)
        {
            const TChar c = operator[](i);
            if (CharFuncs::IsSpace(c))
            {
                if (!result.IsEmpty() && result.Back() != CharTraits::kUnderscore)
                {
                    result += CharTraits::kUnderscore;
                }
            }
            else
            {
                result += CharFuncs::ToLower(c);
            }
        }

        if (!result.IsEmpty() && result.Back() == CharTraits::kUnderscore)
        {
            result.Resize(result.GetLength() - 1);
        }

        return result;
    }

    [[nodiscard]] static TString SnakeCased(
        const TStringView _str)
    {
        TString result = {};
        result.Append(_str);
        return result.ToSnakeCase();
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

    [[nodiscard]] std::basic_string<TChar> ToStdString() const
    {
        return { Self_().GetPtr(), static_cast<size_t>(Self_().GetLength()) };
    }

    [[nodiscard]] std::basic_string_view<TChar> ToStdStringView() const
    {
        return { Self_().GetPtr(), static_cast<size_t>(Self_().GetLength()) };
    }

private:
    [[nodiscard]] TString& Self_()
    {
        return static_cast<TString&>(*this);
    }

    [[nodiscard]] const TString& Self_() const
    {
        return static_cast<const TString&>(*this);
    }
};

////////// string //////////

template<IsCharT TChar>
class BasicString : public StringUtilities<BasicString<TChar>, TChar>
{
public:
    using BaseT       = StringUtilities<BasicString, TChar>;
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

private:
    static constexpr Byte  kHeapEnabledBit = 0xFF;
    static constexpr SizeT kGrowthFactor   = 2;
    static constexpr SizeT kLayoutSize     = 32;
    static constexpr SizeT kSsoBufSize     = (kLayoutSize - 1) / sizeof(TChar);
    static constexpr SizeT kSsoCap         = kSsoBufSize - 1;

    union RawData
    {
        struct
        {
            TChar sso[kSsoBufSize];
        };

        struct
        {
            TChar* heap;
            SizeT  heapLen;
            SizeT  heapCap;
        };

        struct
        {
            Byte padding[kLayoutSize - 1];
            Byte bit;
        };
    };

    static_assert(sizeof(RawData) == 32);
    static_assert(offsetof(RawData, RawData::bit) == 31);

public:
    BasicString()
    {
        ResizeSSO_(0);
    }

    ~BasicString()
    {
        Free_();
    }

    /* implicit */ BasicString(
        const StringViewT _str)
    {
        ResizeSSO_(0);
        Set(_str);
    }

    /* implicit */ BasicString(
        const CharT* _str)
    {
        ResizeSSO_(0);
        Set(_str);
    }

    /* implicit */ BasicString(
        const CharT* _str,
        const SizeT  _n)
    {
        ResizeSSO_(0);
        Set({ _str, _n });
    }

    /* implicit */ BasicString(
        const SizeT _count,
        const CharT _c)
    {
        ResizeSSO_(0);
        Resize(_count, _c);
    }

    BasicString(
        const BasicString& _other)
        : BasicString(_other.ToStringView())
    {
    }

    BasicString(
        BasicString&& _other) noexcept
        : m_str(_other.m_str)
    {
        _other.ResizeSSO_(0);
    }

    BasicString& operator=(
        const BasicString& _other)
    {
        if (this != &_other)
        {
            Set(_other.ToStringView());
        }
        return *this;
    }

    BasicString& operator=(
        BasicString&& _other) noexcept
    {
        if (this != &_other)
        {
            Free_();
            m_str = _other.m_str;
            _other.ResizeSSO_(0);
        }
        return *this;
    }

    BasicString& operator=(
        const StringViewT _other)
    {
        Set(_other);
        return *this;
    }

    BasicString& operator=(
        const TChar _c)
    {
        Set(_c);
        return *this;
    }

    BasicString& operator=(
        const CharT* _ptr)
    {
        Set(_ptr);
        return *this;
    }

    void Set(
        const StringViewT _str)
    {
        const SizeT len = _str.GetLength();

        if (IsHeap_())
        {
            GrowHeapIfNeed_(len);
            SetToHeap_(_str);
        }
        else if (len > kSsoCap)   // change to heap
        {
            InitHeap_(len * kGrowthFactor);
            SetToHeap_(_str);
        }
        else
        {
            SetToSSO_(_str);
        }
    }

    void Append(
        const StringViewT _str)
    {
        const SizeT len = _str.GetLength();

        if (IsHeap_())
        {
            GrowHeapIfNeed_(m_str.heapLen + len);
            AppendToHeap_(_str);
        }
        else if (m_str.bit < len)   // change to heap
        {
            SSOToHeap_((GetLengthSSO_() + len) * kGrowthFactor);
            AppendToHeap_(_str);
        }
        else
        {
            AppendToSSO_(_str);
        }
    }

    void Insert(
        const SizeT       _pos,
        const StringViewT _str)
    {
        const SizeT len = _str.GetLength();
        if (IsHeap_())
        {
            GrowHeapIfNeed_(GetLength() + len);
            InsertToHeap_(_pos, _str);
        }
        else if (m_str.bit < len)   // change to heap
        {
            SSOToHeap_((GetLengthSSO_() + len) * kGrowthFactor);
            InsertToHeap_(_pos, _str);
        }
        else
        {
            InsertToSSO_(_pos, _str);
        }
    }

    void Erase(
        const SizeT _pos,
        const SizeT _count)
    {
        if (IsHeap_())
        {
            EraseAtHeap_(_pos, _count);
        }
        else
        {
            EraseAtSSO_(_pos, _count);
        }
    }

    void Clear()
    {
        if (IsHeap_())
        {
            ResizeHeap_(0);
        }
        else
        {
            ResizeSSO_(0);
        }
    }

    void Reserve(
        const SizeT _newCap)
    {
        if (IsHeap_())
        {
            if (m_str.heapCap < _newCap)
            {
                ReallocHeap_(_newCap);
            }
        }
        else
        {
            if (_newCap > kSsoCap)
            {
                SSOToHeap_(_newCap);
            }
        }
    }

    void Resize(
        const SizeT _newLen,
        const CharT _c = CharTraits::kNullChar)
    {
        const SizeT len = GetLength();

        // resize
        if (IsHeap_())
        {
            GrowHeapIfNeed_(_newLen);
            ResizeHeap_(_newLen);
        }
        else
        {
            if (_newLen > kSsoCap)
            {
                SSOToHeap_(_newLen * kGrowthFactor);
                ResizeHeap_(_newLen);
            }
            else
            {
                ResizeSSO_(_newLen);
            }
        }

        // fill
        if (len < _newLen)
        {
            CharT* ptr = GetPtr();
            std::fill(ptr + len, ptr + _newLen, _c);
        }
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return GetLength() == 0;
    }

    [[nodiscard]] SizeT GetLength() const
    {
        return IsHeap_() ? m_str.heapLen : kSsoCap - m_str.bit;
    }

    [[nodiscard]] SizeT GetCapacity() const
    {
        return IsHeap_() ? m_str.heapCap : kSsoCap;
    }

    [[nodiscard]] const CharT* GetPtr() const
    {
        return IsHeap_() ? m_str.heap : m_str.sso;
    }

    [[nodiscard]] CharT* GetPtr()
    {
        return IsHeap_() ? m_str.heap : m_str.sso;
    }

private:
    void Free_() const
    {
        if (IsHeap_())
        {
            Free(m_str.heap);
        }
    }

    [[nodiscard]] bool IsHeap_() const
    {
        return m_str.bit == kHeapEnabledBit;
    }

    [[nodiscard]] bool IsSSO_() const
    {
        return !IsHeap_();
    }

    void InitHeap_(
        const SizeT _cap)
    {
        m_str.heap    = static_cast<TChar*>(Malloc((_cap + 1) * sizeof(TChar)));
        m_str.heapCap = _cap;
        m_str.bit     = kHeapEnabledBit;
    }

    void GrowHeapIfNeed_(
        const SizeT _newCap)
    {
        if (m_str.heapCap < _newCap)
        {
            ReallocHeap_(_newCap * kGrowthFactor);
        }
    }

    void ReallocHeap_(
        const SizeT _newCap)
    {
        m_str.heap    = static_cast<TChar*>(Realloc(m_str.heap, (_newCap + 1) * sizeof(TChar)));
        m_str.heapCap = _newCap;
    }

    void SetToHeap_(
        const StringViewT _str)
    {
        JAM_ASSERT(IsHeap_(), "SetToHeap_: must be in heap mode to set heap data.");
        JAM_ASSERT(_str.GetLength() <= m_str.heapCap, "SetToHeap_: _str length exceeds heap capacity.");

        const SizeT len = _str.GetLength();
        ::memcpy(m_str.heap, _str.GetPtr(), len * sizeof(TChar));
        ResizeHeap_(len);   // for null terminator and length update
    }

    void ResizeHeap_(
        const SizeT _newLen)
    {
        JAM_ASSERT(IsHeap_(), "ResizeHeap_: must be in heap mode to resize heap data.");
        JAM_ASSERT(_newLen <= m_str.heapCap, "ResizeHeap_: _newLen exceeds heap capacity.");
        m_str.heap[_newLen] = CharTraits::kNullChar;
        m_str.heapLen       = _newLen;
    }

    void AppendToHeap_(
        const StringViewT _str)
    {
        JAM_ASSERT(IsHeap_(), "AppendToHeap_: must be in heap mode to append to heap.");
        JAM_ASSERT(_str.GetLength() + m_str.heapLen <= m_str.heapCap, "AppendToHeap_: combined length exceeds heap capacity.");

        ::memcpy(m_str.heap + m_str.heapLen, _str.GetPtr(), _str.GetLength() * sizeof(CharT));
        ResizeHeap_(m_str.heapLen + _str.GetLength());
    }

    void InsertToHeap_(
        const SizeT       _pos,
        const StringViewT _str)
    {
        JAM_ASSERT(IsHeap_(), "InsertToHeap_: must be in heap mode to insert to heap.");
        JAM_ASSERT(_pos <= m_str.heapLen, "InsertToHeap_: _pos exceeds string length.");

        ::memmove(m_str.heap + _pos + _str.GetLength(), m_str.heap + _pos, (m_str.heapLen - _pos) * sizeof(CharT));
        ::memcpy(m_str.heap + _pos, _str.GetPtr(), _str.GetLength() * sizeof(CharT));
        ResizeHeap_(m_str.heapLen + _str.GetLength());
    }

    void EraseAtHeap_(
        const SizeT _pos,
        const SizeT _count)
    {
        JAM_ASSERT(IsHeap_(), "EraseAtHeap_: must be in heap mode to erase heap data.");

        const SizeT total = _pos + _count;
        const SizeT len   = m_str.heapLen;
        JAM_ASSERT(_pos <= len, "EraseAtHeap_: _pos exceeds string length.");
        JAM_ASSERT(total <= len, "EraseAtHeap_: _pos + _count exceeds string length.");

        ::memmove(m_str.heap + _pos, m_str.heap + total, (len - total) * sizeof(CharT));
        ResizeHeap_(len - _count);
    }

    [[nodiscard]] SizeT GetLengthSSO_() const
    {
        JAM_ASSERT(IsSSO_(), "LengthOfSSO_: must be in stack mode to get stack data length.");
        return kSsoCap - m_str.bit;
    }

    void SetToSSO_(
        const StringViewT _str)
    {
        JAM_ASSERT(IsSSO_(), "SetToStack: must be in stack mode to set stack data.");
        JAM_ASSERT(_str.GetLength() <= kSsoCap, "SetToStack: _str length exceeds stack capacity.");

        const SizeT len = _str.GetLength();
        ::memcpy(m_str.sso, _str.GetPtr(), len * sizeof(TChar));
        ResizeSSO_(len);   // for null terminator and left update
    }

    void ResizeSSO_(
        const SizeT _len)
    {
        JAM_ASSERT(_len <= kSsoCap, "ResizeSSO_: _len exceeds sso capacity.");
        m_str.sso[_len] = CharTraits::kNullChar;
        m_str.bit       = static_cast<Byte>(kSsoCap - _len);
    }

    void AppendToSSO_(
        const StringViewT _str)
    {
        const SizeT len = GetLengthSSO_();

        JAM_ASSERT(IsSSO_(), "AppendToSSO_: must be in stack mode to append to stack.");
        JAM_ASSERT(_str.GetLength() + len <= kSsoCap, "AppendToSSO_: combined length exceeds stack capacity.");

        ::memcpy(m_str.sso + len, _str.GetPtr(), _str.GetLength() * sizeof(TChar));
        ResizeSSO_(len + _str.GetLength());
    }

    void InsertToSSO_(
        const SizeT       _pos,
        const StringViewT _str)
    {
        const SizeT len = GetLengthSSO_();
        JAM_ASSERT(IsSSO_(), "InsertToSSO_: must be in stack mode to insert to stack.");
        JAM_ASSERT(_pos <= len, "InsertToSSO_: _pos exceeds string length.");
        JAM_ASSERT(_str.GetLength() + len <= kSsoCap, "InsertToSSO_: combined length exceeds stack capacity.");

        ::memmove(m_str.sso + _pos + _str.GetLength(), m_str.sso + _pos, (len - _pos) * sizeof(TChar));
        ::memcpy(m_str.sso + _pos, _str.GetPtr(), _str.GetLength() * sizeof(TChar));
        ResizeSSO_(len + _str.GetLength());
    }

    void SSOToHeap_(
        const SizeT _newCap)
    {
        JAM_ASSERT(IsSSO_(), "SSOToHeap_: must be in stack mode to convert to heap.");
        JAM_ASSERT(_newCap > kSsoCap, "SSOToHeap_: _newCap must be greater than stack capacity.");

        TChar*      pHeap  = static_cast<TChar*>(Malloc((_newCap + 1) * sizeof(TChar)));
        const SizeT ssoLen = GetLengthSSO_();
        ::memcpy(pHeap, m_str.sso, ssoLen * sizeof(CharT));

        m_str.heap    = pHeap;
        m_str.heapCap = _newCap;
        m_str.bit     = kHeapEnabledBit;
        ResizeHeap_(ssoLen);
    }

    void EraseAtSSO_(
        const SizeT _pos,
        const SizeT _count)
    {
        JAM_ASSERT(IsSSO_(), "EraseAtSSO_: must be in stack mode to erase stack data.");
        const SizeT total = _pos + _count;
        const SizeT len   = GetLengthSSO_();
        JAM_ASSERT(_pos <= len, "EraseAtSSO_: _pos exceeds string length.");
        JAM_ASSERT(total <= len, "EraseAtSSO_: _pos + _count exceeds string length.");
        ::memmove(m_str.sso + _pos, m_str.sso + total, (len - total) * sizeof(TChar));
        ResizeSSO_(len - _count);
    }

    RawData m_str;
};

using String  = BasicString<char>;
using WString = BasicString<wchar_t>;

}   // namespace Jam

////////// std + external glue //////////

template<Jam::IsCharT CharT>
struct std::hash<Jam::BasicString<CharT>>
{
    std::size_t operator()(const Jam::String& _str) const noexcept
    {
        return _str.Hash64();
    }
};

template<Jam::IsCharT CharT>
struct std::hash<Jam::BasicStringView<CharT>>
{
    std::size_t operator()(const Jam::StringView& _strView) const noexcept
    {
        return _strView.Hash64();
    }
};

template<Jam::IsCharT CharT>
struct ankerl::unordered_dense::hash<Jam::BasicString<CharT>>
{
    [[nodiscard]] std::size_t operator()(const Jam::String& _str) const noexcept
    {
        return ankerl::unordered_dense::detail::wyhash::hash(_str.GetPtr(), _str.GetLength());
    }
};

template<Jam::IsCharT CharT>
struct ankerl::unordered_dense::hash<Jam::BasicStringView<CharT>>
{
    [[nodiscard]] std::size_t operator()(const Jam::StringView& _strView) const noexcept
    {
        return ankerl::unordered_dense::detail::wyhash::hash(_strView.GetPtr(), _strView.GetLength());
    }
};

template<>
struct fmt::formatter<Jam::String> : fmt::formatter<std::string_view>
{
    template<typename FormatContext>
    auto format(const Jam::String& _str, FormatContext& _ctx) const
    {
        return fmt::formatter<std::string_view>::format(_str.ToStdStringView(), _ctx);
    }
};

template<>
struct fmt::formatter<Jam::StringView> : fmt::formatter<std::string_view>
{
    template<typename FormatContext>
    auto format(const Jam::StringView& _strView, FormatContext& _ctx) const
    {
        return fmt::formatter<std::string_view>::format(_strView.ToStdStringView(), _ctx);
    }
};

template<>
struct fmt::formatter<Jam::WString, wchar_t> : fmt::formatter<std::wstring_view, wchar_t>
{
    template<typename FormatContext>
    auto format(const Jam::WString& _str, FormatContext& _ctx) const
    {
        return fmt::formatter<std::wstring_view, wchar_t>::format(_str.ToStdStringView(), _ctx);
    }
};

template<>
struct fmt::formatter<Jam::WStringView, wchar_t> : fmt::formatter<std::wstring_view, wchar_t>
{
    template<typename FormatContext>
    auto format(const Jam::WStringView& _strView, FormatContext& _ctx) const
    {
        return fmt::formatter<std::wstring_view, wchar_t>::format(_strView.ToStdStringView(), _ctx);
    }
};