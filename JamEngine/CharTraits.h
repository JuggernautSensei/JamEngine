// ReSharper disable CppClangTidyClangDiagnosticFloatEqual
#pragma once
#include <cwchar>
#include <filesystem>
#include "TypeTraits.h"
#include "Optional.h"

namespace Jam
{

template<typename T>
concept IsCharT = AnyOfV<T, char, wchar_t, char8_t, char16_t, char32_t>;

struct TrimResult   // [begin, end)
{
    SizeT pos = 0;
    SizeT len = 0;
};

enum class eJoin
{
    File,
    Directory,
    Auto
};

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

template<IsCharT TChar>
struct CharRuntime
{
};

template<>
struct CharRuntime<char>
{
    using CharT = char;

    static SizeT StrLen(const CharT* _s)
    {
        return static_cast<SizeT>(::strlen(_s));
    }

    static void MemCpy(CharT* _dst, const CharT* _src, const SizeT _n)
    {
        ::memcpy(_dst, _src, _n);
    }

    static const CharT* MemChr(const CharT* _s, const CharT _c, const SizeT _n)
    {
        return static_cast<const CharT*>(::memchr(_s, static_cast<unsigned char>(_c), _n));
    }

    static int MemCmp(const CharT* _a, const CharT* _b, const SizeT _n)
    {
        return ::memcmp(_a, _b, _n);
    }

    static Int32 ToInt32(const CharT* _s, const Int32 _base)
    {
        return ::strtol(_s, nullptr, _base);
    }

    static UInt32 ToUInt32(const CharT* _s, const Int32 _base)
    {
        return ::strtoul(_s, nullptr, _base);
    }

    static Int64 ToInt64(const CharT* _s, const Int32 _base)
    {
        return ::strtoll(_s, nullptr, _base);
    }

    static UInt64 ToUInt64(const CharT* _s, const Int32 _base)
    {
        return ::strtoull(_s, nullptr, _base);
    }

    static float ToFloat(const CharT* _s)
    {
        return ::strtof(_s, nullptr);
    }

    static double ToDouble(const CharT* _s)
    {
        return ::strtod(_s, nullptr);
    }

    static long double ToLongDouble(const CharT* _s)
    {
        return ::strtold(_s, nullptr);
    }

    template<typename... TArgs>
    using FormatStringT = fmt::format_string<TArgs...>;

    constexpr static const CharT* kFmtPattern  = "{}";
    constexpr static const CharT* kPlaceholder = "";
};

template<>
struct CharRuntime<wchar_t>
{
    using CharT = wchar_t;

    static SizeT StrLen(const CharT* _s)
    {
        return static_cast<SizeT>(::wcslen(_s));
    }

    static void MemCpy(CharT* _dst, const CharT* _src, const SizeT _n)
    {
        ::wmemcpy(_dst, _src, _n);
    }

    static const CharT* MemChr(const CharT* _s, const CharT _c, const SizeT _n)
    {
        return ::wmemchr(_s, _c, _n);
    }

    static int MemCmp(const CharT* _a, const CharT* _b, const SizeT _n)
    {
        return ::wmemcmp(_a, _b, _n);
    }

    static Int32 ToInt32(const CharT* _s, const Int32 _base)
    {
        return ::wcstol(_s, nullptr, _base);
    }

    static UInt32 ToUInt32(const CharT* _s, const Int32 _base)
    {
        return ::wcstoul(_s, nullptr, _base);
    }

    static Int64 ToInt64(const CharT* _s, const Int32 _base)
    {
        return ::wcstoll(_s, nullptr, _base);
    }

    static UInt64 ToUInt64(const CharT* _s, const Int32 _base)
    {
        return ::wcstoull(_s, nullptr, _base);
    }

    static float ToFloat(const CharT* _s)
    {
        return ::wcstof(_s, nullptr);
    }

    static double ToDouble(const CharT* _s)
    {
        return ::wcstod(_s, nullptr);
    }

    static long double ToLongDouble(const CharT* _s)
    {
        return ::wcstold(_s, nullptr);
    }

    template<typename... TArgs>
    using FormatStringT = fmt::wformat_string<TArgs...>;

    constexpr static const CharT* kFmtPattern  = L"{}";
    constexpr static const CharT* kPlaceholder = L"";
};

template<IsCharT TChar>
struct CharTraits
{
    using CharT   = TChar;
    using Runtime = CharRuntime<TChar>;

    template<typename... TArgs>
    using FormatStringT = typename Runtime::template FormatStringT<TArgs...>;

    ////////// constants //////////

    constexpr static CharT           kNull               = static_cast<CharT>('\0');
    constexpr static Array<CharT, 6> kSpaces             = { static_cast<CharT>(' '), static_cast<CharT>('\t'), static_cast<CharT>('\n'), static_cast<CharT>('\r'), static_cast<CharT>('\v'), static_cast<CharT>('\f') };
    constexpr static CharT           kSlash              = static_cast<CharT>('/');
    constexpr static CharT           kDot                = static_cast<CharT>('.');
    constexpr static CharT           kColon              = static_cast<CharT>(':');
    constexpr static CharT           kUnderscore         = static_cast<CharT>('_');
    constexpr static CharT           kPreferredSeparator = kSlash;
    constexpr static Array<CharT, 2> kSeparators         = { static_cast<CharT>('/'), static_cast<CharT>('\\') };
    constexpr static CharT           kLowerA             = static_cast<CharT>('a');
    constexpr static CharT           kUpperA             = static_cast<CharT>('A');
    constexpr static CharT           kLowerZ             = static_cast<CharT>('z');
    constexpr static CharT           kUpperZ             = static_cast<CharT>('Z');
    constexpr static CharT           kCaseDiff           = kLowerA - kUpperA;
    constexpr static const CharT*    kPlaceholder        = Runtime::kPlaceholder;

    ////////// numeric conversion //////////

    [[nodiscard]] static Int32 AsInt32(const CharT* _str, const Int32 _base = 10)
    {
        return Runtime::ToInt32(_str, _base);
    }

    [[nodiscard]] static UInt32 AsUInt32(const CharT* _str, const Int32 _base = 10)
    {
        return Runtime::ToUInt32(_str, _base);
    }

    [[nodiscard]] static Int64 AsInt64(const CharT* _str, const Int32 _base = 10)
    {
        return Runtime::ToInt64(_str, _base);
    }

    [[nodiscard]] static UInt64 AsUInt64(const CharT* _str, const Int32 _base = 10)
    {
        return Runtime::ToUInt64(_str, _base);
    }

    [[nodiscard]] static float AsFloat(const CharT* _str)
    {
        return Runtime::ToFloat(_str);
    }

    [[nodiscard]] static double AsDouble(const CharT* _str)
    {
        return Runtime::ToDouble(_str);
    }

    [[nodiscard]] static long double AsLongDouble(const CharT* _str)
    {
        return Runtime::ToLongDouble(_str);
    }

    ////////// character predicates //////////

    static constexpr bool IsUpper(const CharT _c)
    {
        return _c >= kUpperA && _c <= kUpperZ;
    }

    static constexpr bool IsLower(const CharT _c)
    {
        return _c >= kLowerA && _c <= kLowerZ;
    }

    static constexpr bool IsAlphabet(const CharT _c)
    {
        return IsUpper(_c) || IsLower(_c);
    }

    static constexpr CharT ToUpper(const CharT _c)
    {
        return IsLower(_c) ? static_cast<CharT>(_c - kCaseDiff) : _c;
    }

    static constexpr CharT ToLower(const CharT _c)
    {
        return IsUpper(_c) ? static_cast<CharT>(_c + kCaseDiff) : _c;
    }

    static constexpr bool IsSpace(const CharT _c)
    {
        for (const CharT ch: kSpaces)
        {
            if (_c == ch)
            {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] static constexpr bool IsPathSeparator(const CharT _c)
    {
        for (const CharT s: kSeparators)
        {
            if (_c == s)
            {
                return true;
            }
        }
        return false;
    }

    ////////// raw string ops //////////

    static constexpr SizeT StrLen(const CharT* _str)
    {
        JAM_ASSERT(_str, "StrLen: _str cannot be null.");
        if (std::is_constant_evaluated())
        {
            SizeT n = 0;
            while (_str[n] != kNull)
            {
                ++n;
            }
            return n;
        }
        return Runtime::StrLen(_str);
    }

    static constexpr SizeT StrCpy(CharT* _pOutBuf, const SizeT _bufCap, const CharT* _str, const SizeT _len)
    {
        JAM_ASSERT(_pOutBuf, "StrCpy: _pOutBuf cannot be null.");
        JAM_ASSERT(_bufCap > 0, "StrCpy: _bufCap must be greater than zero.");
        JAM_ASSERT(_str, "StrCpy: _str cannot be null.");
        JAM_ASSERT(_len + 1 <= _bufCap, "StrCpy: capacity is too small");

        // 주의. StrCpy는 널 문자를 포함한 크기를 요구함.

        if (std::is_constant_evaluated())
        {
            for (SizeT i = 0; i < _len; ++i)
            {
                _pOutBuf[i] = _str[i];
            }
        }
        else
        {
            Runtime::MemCpy(_pOutBuf, _str, _len);
        }
        _pOutBuf[_len] = kNull;
        return _len;
    }

    static constexpr bool Contains(const CharT* _str, const SizeT _len, const CharT _c)
    {
        if (std::is_constant_evaluated())
        {
            for (SizeT i = 0; i < _len; ++i)
            {
                if (_str[i] == _c)
                {
                    return true;
                }
            }
            return false;
        }
        return Runtime::MemChr(_str, _c, _len) != nullptr;
    }

    static constexpr bool StrEqual(const CharT* _str0, const CharT* _str1, const SizeT _n)
    {
        JAM_ASSERT(_str0, "StrEqual: _str0 cannot be null.");
        JAM_ASSERT(_str1, "StrEqual: _str1 cannot be null.");
        if (std::is_constant_evaluated())
        {
            for (SizeT i = 0; i < _n; ++i)
            {
                if (_str0[i] != _str1[i])
                {
                    return false;
                }
            }
            return true;
        }
        return Runtime::MemCmp(_str0, _str1, _n) == 0;
    }

    static constexpr bool StrEqualIgnoreCase(const CharT* _str0, const CharT* _str1, const SizeT _n)
    {
        JAM_ASSERT(_str0, "StrEqualIgnoreCase: _str0 cannot be null.");
        JAM_ASSERT(_str1, "StrEqualIgnoreCase: _str1 cannot be null.");

        for (SizeT i = 0; i < _n; ++i)
        {
            if (ToLower(_str0[i]) != ToLower(_str1[i]))
            {
                return false;
            }
        }
        return true;
    }

    static constexpr Optional<SizeT> StrFind(const CharT* _str, const SizeT _len, const CharT _c)
    {
        using StdStringView = std::basic_string_view<CharT>;

        const StdStringView str = { _str, static_cast<USizeT>(_len) };
        const USizeT        pos = str.find(_c);
        return pos != StdStringView::npos ? Optional<SizeT> { static_cast<SizeT>(pos) } : kNullopt;
    }

    static constexpr Optional<SizeT> StrFind(const CharT* _str, const SizeT _len, const CharT* _findStr, const SizeT _findStrLen)
    {
        using StdStringView = std::basic_string_view<CharT>;

        const StdStringView str = { _str, static_cast<USizeT>(_len) };
        const USizeT        pos = str.find(_findStr, 0, _findStrLen);
        return pos != StdStringView::npos ? Optional<SizeT> { static_cast<SizeT>(pos) } : kNullopt;
    }

    static constexpr Optional<SizeT> StrRFind(const CharT* _str, const SizeT _len, const CharT _c)
    {
        using StdStringView = std::basic_string_view<CharT>;

        const StdStringView str = { _str, static_cast<USizeT>(_len) };
        const USizeT        pos = str.rfind(_c);
        return pos != StdStringView::npos ? Optional<SizeT> { static_cast<SizeT>(pos) } : kNullopt;
    }

    static constexpr Optional<SizeT> StrRFind(const CharT* _str, const SizeT _len, const CharT* _findStr, const SizeT _findStrLen)
    {
        using StdStringView = std::basic_string_view<CharT>;

        const StdStringView sv  = { _str, static_cast<USizeT>(_len) };
        const USizeT        pos = sv.rfind(_findStr, StdStringView::npos, _findStrLen);
        return pos != StdStringView::npos ? Optional<SizeT> { static_cast<SizeT>(pos) } : kNullopt;
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
        const SizeT n = Min(_len0, _len1);
        if (std::is_constant_evaluated())
        {
            for (SizeT i = 0; i < n; ++i)
            {
                if (_str0[i] != _str1[i])
                {
                    return _str0[i] < _str1[i] ? static_cast<SizeT>(-1) : 1;
                }
            }
            return 0;
        }
        return static_cast<SizeT>(Runtime::MemCmp(_str0, _str1, n));
    }

    ////////// path ops //////////

    [[nodiscard]] static constexpr PathDecomposeResult DecomposePath(const CharT* _str, const SizeT _len)
    {
        JAM_ASSERT(_str, "DecomposePath: _str cannot be null.");

        if (_len == 0)
        {
            return {};
        }

        // /aaa/bbb/ccc.txt
        // directory: /aaa/bbb/
        // base name: ccc
        // extension: .txt
        // file name: ccc + .txt = ccc.txt
        //
        // /aaa/bbb/ccc/eee/
        // directory: /aaa/bbb/ccc/
        // base name: eee
        // extension: (none)
        // file name: eee

        const SizeT         end    = IsPathSeparator(_str[_len - 1]) ? _len - 1 : _len;
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
            else if (c == kDot && !result.bHasExt)
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
        JAM_ASSERT(_len <= _bufCap, "Simplify: _len must be less than _bufCap.");

        // simplify는 null 문자를 처리하지 않음

        if (_len == 0)
        {
            _pOutBuf[0] = kDot;
            return 1;
        }

        SizeT inIdx  = 0;
        SizeT outIdx = 0;
        SizeT dotdot = 0;

        if (_len >= 2 && IsAlphabet(_str[0]) && _str[1] == kColon)
        {
            _pOutBuf[outIdx++] = ToUpper(_str[0]);
            _pOutBuf[outIdx++] = kColon;
            inIdx += 2;
            dotdot = outIdx;
        }

        const bool bIsRoot = (inIdx < _len) && IsPathSeparator(_str[inIdx]);
        if (bIsRoot)
        {
            _pOutBuf[outIdx++] = kPreferredSeparator;
            ++inIdx;
            dotdot = outIdx;
        }

        bool bTrailingSlash = false;
        while (inIdx < _len)
        {
            if (IsPathSeparator(_str[inIdx]))
            {
                ++inIdx;
                bTrailingSlash = (inIdx == _len);
                continue;
            }

            if (_str[inIdx] == kDot)
            {
                if (inIdx + 1 == _len || IsPathSeparator(_str[inIdx + 1]))
                {
                    ++inIdx;
                    continue;
                }

                if (kDot == _str[inIdx + 1] && (inIdx + 2 == _len || IsPathSeparator(_str[inIdx + 2])))
                {
                    inIdx += 2;
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
                            _pOutBuf[outIdx++] = kPreferredSeparator;
                        }
                        _pOutBuf[outIdx++] = kDot;
                        _pOutBuf[outIdx++] = kDot;
                        dotdot             = outIdx;
                    }
                    continue;
                }
            }

            if ((bIsRoot && outIdx > dotdot) || (!bIsRoot && outIdx > 0))
            {
                if (outIdx > 0 && !IsPathSeparator(_pOutBuf[outIdx - 1]))
                {
                    _pOutBuf[outIdx++] = kPreferredSeparator;
                }
            }

            for (; inIdx < _len && !IsPathSeparator(_str[inIdx]); ++inIdx)
            {
                _pOutBuf[outIdx++] = _str[inIdx];
            }
        }

        if (outIdx == 0)
        {
            _pOutBuf[outIdx++] = kDot;
        }
        else if (bTrailingSlash && outIdx > 0 && !IsPathSeparator(_pOutBuf[outIdx - 1]))
        {
            _pOutBuf[outIdx++] = kPreferredSeparator;
        }

        return outIdx;
    }

    ////////// parsing //////////

    template<typename T>
        requires std::is_integral_v<T>
    [[nodiscard]] static Optional<T> ParseAs(const CharT* _str, const Int32 _base = 10)
    {
        JAM_ASSERT(_str, "ParseAs: _str cannot be null.");

        if constexpr (std::is_same_v<T, Int32> || std::is_same_v<T, Int16> || std::is_same_v<T, Int8> || std::is_same_v<T, char> || std::is_same_v<T, wchar_t>)
        {
            return static_cast<T>(AsInt32(_str, _base));
        }
        else if constexpr (std::is_same_v<T, UInt32> || std::is_same_v<T, UInt16> || std::is_same_v<T, UInt8> || std::is_same_v<T, unsigned char>)
        {
            return static_cast<T>(AsUInt32(_str, _base));
        }
        else if constexpr (std::is_same_v<T, Int64>)
        {
            return static_cast<T>(AsInt64(_str, _base));
        }
        else if constexpr (std::is_same_v<T, UInt64>)
        {
            return static_cast<T>(AsUInt64(_str, _base));
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
            const float value = AsFloat(_str);
            if (value == HUGE_VALF || value == -HUGE_VALF)
            {
                return kNullopt;
            }
            return value;
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            const double value = AsDouble(_str);
            if (value == HUGE_VAL || value == -HUGE_VAL)
            {
                return kNullopt;
            }
            return value;
        }
        else if constexpr (std::is_same_v<T, long double>)
        {
            const long double value = AsLongDouble(_str);
            if (value == HUGE_VALL || value == -HUGE_VALL)
            {
                return kNullopt;
            }
            return value;
        }

        JAM_UNREACHABLE;
    }
};

}   // namespace Jam