#pragma once

#define RS_MALLOC  Jam::Malloc
#define RS_REALLOC Jam::Realloc
#define RS_FREE    Jam::Free
#include "Vendor/rapidstring/rapidstring.h"

namespace Jam
{

////////// char utilities //////////

using CharTraits  = std::char_traits<char>;
using WCharTraits = std::char_traits<wchar_t>;

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

constexpr bool IsUpper(const char _c)
{
    return (_c >= 'A') && (_c <= 'Z');
}

constexpr bool IsLower(const char _c)
{
    return (_c >= 'a') && (_c <= 'z');
}

constexpr bool IsAlphabet(const char _c)
{
    return IsUpper(_c) || IsLower(_c);
}

constexpr char ToUpper(const char _c)
{
    return IsLower(_c) ? static_cast<char>(_c - ('a' - 'A')) : _c;
}

constexpr char ToLower(const char _c)
{
    return IsUpper(_c) ? static_cast<char>(_c + ('a' - 'A')) : _c;
}

constexpr bool IsSpace(const char _c)
{
    return ' ' == _c
        || '\t' == _c
        || '\n' == _c
        || '\r' == _c
        || '\v' == _c
        || '\f' == _c;
}

constexpr const char* FindTerminate(const char* _start)
{
    JAM_ASSERT(_start, "FindTerminate: _start cannot be null.");

    if (std::is_constant_evaluated())
    {
        const char* ptr = _start;
        for (; *ptr != '\0'; ++ptr)
        {
            JAM_NOOP;
        }
        return ptr;
    }
    else
    {
        return _start + CharTraits::length(_start);
    }
}

constexpr const wchar_t* FindTerminate(const wchar_t* _start)
{
    JAM_ASSERT(_start, "FindTerminate: _start cannot be null.");

    if (std::is_constant_evaluated())
    {
        const wchar_t* ptr = _start;
        for (; *ptr != L'\0'; ++ptr)
        {
            JAM_NOOP;
        }
        return ptr;
    }
    else
    {
        return _start + WCharTraits::length(_start);
    }
}

constexpr SizeT StrLen(const char* _start, const char* _endOrNull = nullptr)
{
    JAM_ASSERT(_start, "StrLen: _start cannot be null.");

    const char* end = _endOrNull ? _endOrNull : FindTerminate(_start);
    return end - _start;
}

constexpr SizeT StrLen(const wchar_t* _start, const wchar_t* _endOrNull = nullptr)
{
    JAM_ASSERT(_start, "StrLen: _start cannot be null.");

    const wchar_t* end = _endOrNull ? _endOrNull : FindTerminate(_start);
    return end - _start;
}

constexpr bool StrEqual(const char* _start1, const char* _end1OrNull, const char* _start2, const char* _end2OrNull)
{
    JAM_ASSERT(_start1, "StrEqual: _start1 cannot be null.");
    JAM_ASSERT(_start2, "StrEqual: _start2 cannot be null.");

    const SizeT len1 = StrLen(_start1, _end1OrNull);
    const SizeT len2 = StrLen(_start2, _end2OrNull);

    if (len1 != len2)
    {
        return false;
    }

    if (std::is_constant_evaluated())
    {
        for (SizeT i = 0; i < len1; ++i)
        {
            if (_start1[i] != _start2[i])
            {
                return false;
            }
        }
        return true;
    }
    else
    {
        return CharTraits::compare(_start1, _start2, len1) == 0;
    }
}

constexpr CharResult StrFind(const char* _start, const char* _endOrNull, const char _c)
{
    JAM_ASSERT(_start, "StrFind: _start cannot be null.");

    const SizeT len = StrLen(_start, _endOrNull);
    if (std::is_constant_evaluated())
    {
        for (SizeT i = 0; i < len; ++i)
        {
            if (_start[i] == _c)
            {
                return CharResult { i };
            }
        }
        return CharResult::Failed();
    }
    else
    {
        const char* ret = CharTraits::find(_start, len, _c);
        return ret ? CharResult { ret - _start } : CharResult::Failed();
    }
}

constexpr CharResult StrFind(const char* _start1, const char* _end1OrNull, const char* _start2, const char* _end2OrNull)
{
    JAM_ASSERT(_start1, "StrFind: _start1 cannot be null.");
    JAM_ASSERT(_start2, "StrFind: _start2 cannot be null.");

    const SizeT len1 = StrLen(_start1, _end1OrNull);
    const SizeT len2 = StrLen(_start2, _end2OrNull);

    if (len2 > len1)
    {
        return CharResult::Failed();
    }

    const SizeT n = len1 - len2 + 1;
    if (std::is_constant_evaluated())
    {
        for (SizeT i = 0; i < n; ++i)
        {
            bool bFound = true;
            for (SizeT j = 0; j < len2; ++j)
            {
                if (_start1[i + j] != _start2[j])
                {
                    bFound = false;
                    break;
                }
            }

            if (bFound)
            {
                return CharResult { i };
            }
        }
    }
    else
    {
        for (SizeT i = 0; i < n; ++i)
        {
            if (CharTraits::compare(_start1 + i, _start2, len2) == 0)
            {
                return CharResult { i };
            }
        }
    }

    return CharResult::Failed();
}

constexpr CharResult StrRFind(const char* _start, const char* _endOrNull, const char _c)
{
    JAM_ASSERT(_start, "StrRFind: _start cannot be null.");

    const SizeT len = StrLen(_start, _endOrNull);
    for (SizeT i = len; i > 0; --i)
    {
        if (_start[i - 1] == _c)
        {
            return CharResult { i - 1 };
        }
    }
    return CharResult::Failed();
}

constexpr CharResult StrRFind(const char* _start0, const char* _end0OrNull, const char* _start1, const char* _end1OrNull)
{
    JAM_ASSERT(_start0, "StrRFind: _start0 cannot be null.");

    const SizeT len0 = StrLen(_start0, _end0OrNull);
    const SizeT len1 = StrLen(_start1, _end1OrNull);

    if (len1 > len0)
    {
        return CharResult::Failed();
    }

    if (std::is_constant_evaluated())
    {
        for (SizeT i = len0 - len1; i >= 0; --i)
        {
            bool bFound = true;
            for (SizeT j = 0; j < len1; ++j)
            {
                if (_start0[i + j] != _start1[j])
                {
                    bFound = false;
                    break;
                }
            }

            if (bFound)
            {
                return CharResult { i };
            }
        }
    }
    else
    {
        for (SizeT i = len0 - len1; i >= 0; --i)
        {
            if (CharTraits::compare(_start0 + i, _start1, len1) == 0)
            {
                return CharResult { i };
            }
        }
    }

    return CharResult::Failed();
}

constexpr bool StrStartsWith(const char* _start0, const char* _end0OrNull, const char* _start1, const char* _end1OrNull)
{
    JAM_ASSERT(_start0, "StrStartsWith: _start0 cannot be null.");

    const SizeT len0 = StrLen(_start0, _end0OrNull);
    const SizeT len1 = StrLen(_start1, _end1OrNull);
    if (len0 < len1)
    {
        return false;
    }

    if (std::is_constant_evaluated())
    {
        for (SizeT i = 0; i < len1; ++i)
        {
            if (_start0[i] != _start1[i])
            {
                return false;
            }
        }
        return true;
    }
    else
    {
        return CharTraits::compare(_start0, _start1, len1) == 0;
    }
}

constexpr bool StrEndsWith(const char* _start0, const char* _end0OrNull, const char* _start1, const char* _end1OrNull)
{
    JAM_ASSERT(_start0, "StrEndsWith: _start0 cannot be null.");

    const SizeT len0 = StrLen(_start0, _end0OrNull);
    const SizeT len1 = StrLen(_start1, _end1OrNull);
    if (len0 < len1)
    {
        return false;
    }
    if (std::is_constant_evaluated())
    {
        for (SizeT i = 0; i < len1; ++i)
        {
            if (_start0[len0 - len1 + i] != _start1[i])
            {
                return false;
            }
        }
        return true;
    }
    else
    {
        return CharTraits::compare(_start0 + (len0 - len1), _start1, len1) == 0;
    }
}

constexpr bool StrCompare(const char* _start0, const char* _end0OrNull, const char* _start1, const char* _end1OrNull)
{
    JAM_ASSERT(_start0, "StrCompare: _start0 cannot be null.");
    JAM_ASSERT(_start1, "StrCompare: _start1 cannot be null.");

    const SizeT len0 = StrLen(_start0, _end0OrNull);
    const SizeT len1 = StrLen(_start1, _end1OrNull);
    if (len0 != len1)
    {
        return false;
    }

    if (std::is_constant_evaluated())
    {
        for (SizeT i = 0; i < len0; ++i)
        {
            if (_start0[i] != _start1[i])
            {
                return false;
            }
        }
        return true;
    }
    else
    {
        return CharTraits::compare(_start0, _start1, len0) == 0;
    }
}

constexpr bool StrCompareN(const char* _start0, const char* _end0OrNull, const char* _start1, const char* _end1OrNull, const SizeT _n)
{
    JAM_ASSERT(_start0, "StrCompareN: _start0 cannot be null.");
    JAM_ASSERT(_start1, "StrCompareN: _start1 cannot be null.");

    const SizeT len0 = StrLen(_start0, _end0OrNull);
    const SizeT len1 = StrLen(_start1, _end1OrNull);
    if (len0 < _n || len1 < _n)
    {
        return false;
    }

    if (std::is_constant_evaluated())
    {
        for (SizeT i = 0; i < _n; ++i)
        {
            if (_start0[i] != _start1[i])
            {
                return false;
            }
        }
        return true;
    }
    else
    {
        return CharTraits::compare(_start0, _start1, _n) == 0;
    }
}

constexpr bool StrCompareIgnoreCase(const char* _start0, const char* _end0OrNull, const char* _start1, const char* _end1OrNull)
{
    JAM_ASSERT(_start0, "StrCompareIgnoreCase: _start0 cannot be null.");
    JAM_ASSERT(_start1, "StrCompareIgnoreCase: _start1 cannot be null.");

    const SizeT len0 = StrLen(_start0, _end0OrNull);
    const SizeT len1 = StrLen(_start1, _end1OrNull);
    if (len0 != len1)
    {
        return false;
    }
    for (SizeT i = 0; i < len0; ++i)
    {
        if (ToLower(_start0[i]) != ToLower(_start1[i]))
        {
            return false;
        }
    }
    return true;
}

constexpr bool StrCompareIgnoreCaseN(const char* _start0, const char* _end0OrNull, const char* _start1, const char* _end1OrNull, const SizeT _n)
{
    JAM_ASSERT(_start0, "StrCompareIgnoreCaseN: _start0 cannot be null.");
    JAM_ASSERT(_start1, "StrCompareIgnoreCaseN: _start1 cannot be null.");

    const SizeT len0 = StrLen(_start0, _end0OrNull);
    const SizeT len1 = StrLen(_start1, _end1OrNull);
    if (len0 < _n || len1 < _n)
    {
        return false;
    }

    for (SizeT i = 0; i < _n; ++i)
    {
        if (ToLower(_start0[i]) != ToLower(_start1[i]))
        {
            return false;
        }
    }

    return true;
}

constexpr void StrLTrim(const char* _pStart, const char* _pEndOrNull, const char** _pOutStart)
{
    JAM_ASSERT(_pStart, "StrLTrim: _pStart cannot be null.");
    JAM_ASSERT(_pOutStart, "StrLTrim: _pOutStart cannot be null.");

    const char* ptr  = _pStart;
    const char* pEnd = _pEndOrNull ? _pEndOrNull : FindTerminate(_pStart);
    for (; ptr < pEnd; ++ptr)
    {
        if (!IsSpace(*ptr))
        {
            break;
        }
    }
    *_pOutStart = ptr;
}

constexpr void StrRTrim(const char* _pStart, const char* _pEndOrNull, const char** _pOutEnd)
{
    JAM_ASSERT(_pStart, "StrRTrim: _pStart cannot be null.");
    JAM_ASSERT(_pOutEnd, "StrRTrim: _pOutEnd cannot be null.");

    const char* pEnd = _pEndOrNull ? _pEndOrNull : FindTerminate(_pStart);
    const char* ptr  = pEnd;
    for (; ptr > _pStart; --ptr)
    {
        if (!IsSpace(*(ptr - 1)))
        {
            break;
        }
    }
    *_pOutEnd = ptr;
}

constexpr void StrTrim(const char* _pStart, const char* _pEndOrNull, const char** _pOutStart, const char** _pOutEnd)
{
    JAM_ASSERT(_pStart, "StrTrim: _pStart cannot be null.");
    JAM_ASSERT(_pOutStart, "StrTrim: _pOutStart cannot be null.");
    JAM_ASSERT(_pOutEnd, "StrTrim: _pOutEnd cannot be null.");

    const char* pEnd = _pEndOrNull ? _pEndOrNull : FindTerminate(_pStart);
    StrLTrim(_pStart, pEnd, _pOutStart);
    StrRTrim(*_pOutStart, pEnd, _pOutEnd);
}

// return: converted length, excluding null-terminator.
SizeT CalcRequiredUtf16BufferSize(const char* _start, const char* _endOrNull = nullptr);
SizeT CalcRequiredUtf8BufferSize(const wchar_t* _start, const wchar_t* _endOrNull = nullptr);
SizeT ConvertToUtf16(const char* _start, const char* _endOrNull, wchar_t* _pOutBuf, SizeT _outBufCap);
SizeT ConvertToUtf8(const wchar_t* _start, const wchar_t* _endOrNull, char* _pOutBuf, SizeT _outBufCap);

template<typename T>
    requires std::is_integral_v<T>
constexpr Optional<T> ParseAs(const char* _start, const char* _endOrNull, Int32 _base = 10)
{
    T output = {};

    const SizeT            len = StrLen(_start, _endOrNull);
    std::from_chars_result ret = std::from_chars(_start, _start + len, output, _base);
    if (ret.ec == std::errc {})
    {
        return output;
    }

    return k_nullopt;
}

////////// string hashers //////////

namespace Details
{
    constexpr UInt32 k_fnv1ah32Prime = 16777619U;
    constexpr UInt32 k_fnv1ah32Seed  = 2166136261UL;
    constexpr UInt64 k_fnv1ah64Prime = 1099511628211ULL;
    constexpr UInt64 k_fnv1ah64Seed  = 14695981039346656037ULL;
}   // namespace Details

// constexpr hash for string
class Fnv1ahHasher32
{
public:
    explicit constexpr Fnv1ahHasher32(
        const UInt32 _seed = Details::k_fnv1ah32Seed)
        : m_hash(_seed)
    {
    }

    constexpr void Mix(
        const char* _str,
        const SizeT _len)
    {
        for (SizeT i = 0; i < _len; ++i)
        {
            m_hash = (m_hash ^ NumericCast<UInt32>(_str[i])) * Details::k_fnv1ah32Prime;
        }
    }

    [[nodiscard]] constexpr UInt32 Finish() const
    {
        return m_hash;
    }

private:
    UInt32 m_hash = 0;
};

class Fnv1ahHasher64
{
public:
    explicit constexpr Fnv1ahHasher64(
        const UInt64 _seed = Details::k_fnv1ah64Seed)
        : m_hash(_seed)
    {
    }

    constexpr void Mix(
        const char* _str,
        const SizeT _len)
    {
        for (SizeT i = 0; i < _len; ++i)
        {
            m_hash = (m_hash ^ NumericCast<UInt64>(_str[i])) * Details::k_fnv1ah64Prime;
        }
    }

    [[nodiscard]] constexpr UInt64 Finish() const
    {
        return m_hash;
    }

private:
    UInt64 m_hash = 0;
};

class IgnoreCaseFnv1ahHasher32
{
public:
    explicit constexpr IgnoreCaseFnv1ahHasher32(
        const UInt32 _seed = Details::k_fnv1ah32Seed)
        : m_hash(_seed)
    {
    }

    constexpr void Mix(
        const char* _str,
        const SizeT _len)
    {
        for (SizeT i = 0; i < _len; ++i)
        {
            m_hash = (m_hash ^ NumericCast<UInt32>(ToLower(_str[i]))) * Details::k_fnv1ah32Prime;
        }
    }

    [[nodiscard]] constexpr UInt32 Finish() const
    {
        return m_hash;
    }

private:
    UInt32 m_hash = 0;
};

class IgnoreCaseFnv1ahHasher64
{
public:
    explicit constexpr IgnoreCaseFnv1ahHasher64(
        const UInt64 _seed = Details::k_fnv1ah64Seed)
        : m_hash(_seed)
    {
    }

    constexpr void Mix(
        const char* _str,
        const SizeT _len)
    {
        for (SizeT i = 0; i < _len; ++i)
        {
            m_hash = (m_hash ^ NumericCast<UInt64>(ToLower(_str[i]))) * Details::k_fnv1ah64Prime;
        }
    }

    [[nodiscard]] constexpr UInt64 Finish() const
    {
        return m_hash;
    }

private:
    UInt64 m_hash = 0;
};

////////// fast String than std::string //////////

class String
{
public:
    using Iter      = char*;
    using ConstIter = const char*;

    String()
    {
        rs_init(&m_str);
    }

    /* implicit */ String(const char* _str)
    {
        JAM_ASSERT(_str, "String constructor: _str cannot be null.");
        rs_init_w(&m_str, _str);
    }

    /* implicit */ String(const char* _str, const SizeT _n)
    {
        JAM_ASSERT(_str, "String constructor: _str cannot be null.");
        rs_init_w_n(&m_str, _str, _n);
    }

    template<size_t N>
    /* implicit */ String(const char (&_str)[N])
    {
        rs_init_w_n(&m_str, _str, N - 1);
    }

    /* implicit */ String(const SizeT _n, const char _c)   // fill constructor
    {
        rs_init(&m_str);
        rs_resize_w(&m_str, _n, _c);
    }

    String(const String& other)
    {
        rs_init_w_rs(&m_str, &other.m_str);
    }

    String(String&& other) noexcept
    {
        m_str = other.m_str;
        rs_init(&other.m_str);
    }

    ~String()
    {
        rs_free(&m_str);
    }

    String& operator=(const String& other)
    {
        if (this != &other)
        {
            rs_cpy_rs(&m_str, &other.m_str);
        }
        return *this;
    }

    String& operator=(String&& other) noexcept
    {
        if (this != &other)
        {
            rs_free(&m_str);
            m_str = other.m_str;
            rs_init(&other.m_str);
        }
        return *this;
    }

    String& operator=(const char* _str)
    {
        JAM_ASSERT(_str, "String assignment: _str cannot be null.");
        rs_cpy(&m_str, _str);
        return *this;
    }

    String& operator+=(const String& other)
    {
        rs_cat_rs(&m_str, &other.m_str);
        return *this;
    }

    String& operator+=(const char* _str)
    {
        JAM_ASSERT(_str, "String concatenation: _str cannot be null.");
        rs_cat(&m_str, _str);
        return *this;
    }

    String operator+(const String& other) const
    {
        String result { *this };
        result += other;
        return result;
    }

    String operator+(const char* _str) const
    {
        String result { *this };
        result += _str;
        return result;
    }

    [[nodiscard]] char& operator[](const SizeT _index)
    {
        return rs_data(&m_str)[_index];
    }

    [[nodiscard]] const char& operator[](const SizeT _index) const
    {
        return rs_data_c(&m_str)[_index];
    }

    bool operator==(const String& other) const
    {
        return StrEqual(GetPtr(), GetTerminate(), other.GetPtr(), other.GetTerminate());
    }

    bool operator==(const char* _str) const
    {
        JAM_ASSERT(_str, "String comparison: _str cannot be null.");
        return StrEqual(GetPtr(), GetTerminate(), _str, nullptr);
    }

    bool operator!=(const String& other) const
    {
        return !(*this == other);
    }

    bool operator!=(const char* _str) const
    {
        JAM_ASSERT(_str, "String comparison: _str cannot be null.");
        return !(*this == _str);
    }

    template<typename T>
        requires std::is_integral_v<T>
    [[nodiscard]] Optional<T> ParseAs()
    {
        return Jam::ParseAs<T>(GetPtr(), GetTerminate());
    }

    template<typename T>
        requires std::is_integral_v<T>
    [[nodiscard]] static String FromIntegral(const T _value, const Int32 _base = 10)
    {
        char buffer[65];   // enough for binary representation of 64-bit integer plus null terminator

        std::to_chars_result ret = std::to_chars(buffer, buffer + sizeof(buffer), _value, _base);
        JAM_ASSERT(ret.ec == std::errc {}, "Failed to convert integral to string.");
        return { buffer, ret.ptr - buffer };
    }

    void Assign(const char* _str)
    {
        JAM_ASSERT(_str, "String assignment: _str cannot be null.");
        rs_cpy(&m_str, _str);
    }

    void Assign(const char* _str, const SizeT _n)
    {
        JAM_ASSERT(_str, "String assignment: _str cannot be null.");
        rs_cpy_n(&m_str, _str, _n);
    }

    void Assign(const String& _other)
    {
        rs_cpy_rs(&m_str, &_other.m_str);
    }

    [[nodiscard]] const char* GetPtr() const
    {
        return rs_data_c(&m_str);
    }

    [[nodiscard]] const char* GetTerminate() const
    {
        return GetPtr() + GetLength();
    }

    [[nodiscard]] char* GetPtr()
    {
        return rs_data(&m_str);
    }

    [[nodiscard]] SizeT GetLength() const
    {
        return NumericCast<SizeT>(rs_len(&m_str));
    }

    [[nodiscard]] SizeT GetCapacity() const
    {
        return NumericCast<SizeT>(rs_cap(&m_str));
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return rs_empty(&m_str) == 1;
    }

    void Clear()
    {
        rs_clear(&m_str);
    }

    [[nodiscard]] String SubStr(const SizeT _offset) const
    {
        JAM_ASSERT(_offset <= GetLength(), "SubView offset exceeds string length.");
        return { GetPtr() + _offset, GetLength() - _offset };
    }

    [[nodiscard]] String SubStr(const SizeT _offset, const SizeT _count) const
    {
        JAM_ASSERT(_offset + _count <= GetLength(), "SubView range exceeds string length.");
        return { GetPtr() + _offset, _count };
    }

    void Reserve(const SizeT _n)
    {
        rs_reserve(&m_str, _n);
    }

    void ShrinkToFit()
    {
        rs_shrink_to_fit(&m_str);
    }

    void Resize(const SizeT _n)
    {
        rs_resize(&m_str, _n);
    }

    void Resize(const SizeT _n, const char _c)
    {
        rs_resize_w(&m_str, _n, _c);
    }

    void Append(const char _c)
    {
        rs_cat_n(&m_str, &_c, 1);
    }

    void Append(const char* _str)
    {
        JAM_ASSERT(_str, "String assignment: _str cannot be null.");
        rs_cat(&m_str, _str);
    }

    void Append(const char* _str, const SizeT _n)
    {
        JAM_ASSERT(_str, "String assignment: _str cannot be null.");
        rs_cat_n(&m_str, _str, _n);
    }

    void Append(const String& _other)
    {
        rs_cat_rs(&m_str, &_other.m_str);
    }

    template<typename... TArgs>
    void Format(const fmt::format_string<TArgs...> _format, TArgs&&... _args)
    {
        Clear();
        fmt::format_to(std::back_inserter(*this), _format, std::forward<TArgs>(_args)...);
    }

    template<typename... Args>
    void AppendFormat(const fmt::format_string<Args...> _format, Args&&... _args)
    {
        fmt::format_to(std::back_inserter(*this), _format, std::forward<Args>(_args)...);
    }

    void Erase(const SizeT _index, const SizeT _count)
    {
        rs_erase(&m_str, _index, _count);
    }

    [[nodiscard]] CharResult Find(const char _c, const SizeT _offset = 0) const
    {
        return StrFind(GetPtr() + _offset, GetTerminate(), _c);
    }

    [[nodiscard]] CharResult Find(const char* _str, const SizeT _offset = 0) const
    {
        JAM_ASSERT(_str, "String assignment: _str cannot be null.");
        return StrFind(GetPtr() + _offset, GetTerminate(), _str, nullptr);
    }

    [[nodiscard]] CharResult Find(const String& _other, const SizeT _offset = 0) const
    {
        return StrFind(GetPtr() + _offset, GetTerminate(), _other.GetPtr(), _other.GetTerminate());
    }

    [[nodiscard]] CharResult RFind(const char _c) const
    {
        return StrRFind(GetPtr(), GetTerminate(), _c);
    }

    [[nodiscard]] CharResult RFind(const char* _str) const
    {
        JAM_ASSERT(_str, "String assignment: _str cannot be null.");
        return StrRFind(GetPtr(), GetTerminate(), _str, nullptr);
    }

    [[nodiscard]] CharResult RFind(const String& _other) const
    {
        return StrRFind(GetPtr(), GetTerminate(), _other.GetPtr(), _other.GetTerminate());
    }

    [[nodiscard]] CharResult RFind(const char _c, const SizeT _count) const
    {
        return StrRFind(GetPtr(), GetPtr() + _count, _c);
    }

    [[nodiscard]] CharResult RFind(const char* _str, const SizeT _count) const
    {
        JAM_ASSERT(_str, "String assignment: _str cannot be null.");
        return StrRFind(GetPtr(), GetPtr() + _count, _str, nullptr);
    }

    [[nodiscard]] CharResult RFind(const String& _other, const SizeT _count) const
    {
        return StrRFind(GetPtr(), GetPtr() + _count, _other.GetPtr(), _other.GetTerminate());
    }

    [[nodiscard]] bool StartsWith(const char _c, const SizeT _offset = 0) const
    {
        return !IsEmpty() && operator[](_offset) == _c;
    }

    [[nodiscard]] bool StartsWith(const char* _str, const SizeT _offset = 0) const
    {
        JAM_ASSERT(_str, "String assignment: _str cannot be null.");
        return StrStartsWith(GetPtr() + _offset, GetTerminate(), _str, nullptr);
    }

    [[nodiscard]] bool StartsWith(const String& _other, const SizeT _offset = 0) const
    {
        return StrStartsWith(GetPtr() + _offset, GetTerminate(), _other.GetPtr(), _other.GetTerminate());
    }

    [[nodiscard]] bool EndsWith(const char _c) const
    {
        return !IsEmpty() && operator[](GetLength() - 1) == _c;
    }

    [[nodiscard]] bool EndsWith(const char* _str) const
    {
        JAM_ASSERT(_str, "String assignment: _str cannot be null.");
        return StrEndsWith(GetPtr(), GetTerminate(), _str, nullptr);
    }

    [[nodiscard]] bool EndsWith(const String& _other) const
    {
        return StrEndsWith(GetPtr(), GetTerminate(), _other.GetPtr(), _other.GetTerminate());
    }

    [[nodiscard]] bool EndsWith(const char _c, const SizeT _count) const
    {
        return !IsEmpty() && operator[](_count - 1) == _c;
    }

    [[nodiscard]] bool EndsWith(const char* _str, const SizeT _count) const
    {
        JAM_ASSERT(_str, "String assignment: _str cannot be null.");
        return StrEndsWith(GetPtr(), GetPtr() + _count, _str, nullptr);
    }

    [[nodiscard]] bool EndsWith(const String& _other, const SizeT _count) const
    {
        return StrEndsWith(GetPtr(), GetPtr() + _count, _other.GetPtr(), _other.GetTerminate());
    }

    [[nodiscard]] bool Compare(const char* _str, const SizeT _offset = 0) const
    {
        JAM_ASSERT(_str, "String assignment: _str cannot be null.");
        return StrCompare(GetPtr() + _offset, GetTerminate(), _str, nullptr);
    }

    [[nodiscard]] bool Compare(const String& _str, const SizeT _offset = 0) const
    {
        return StrCompare(GetPtr() + _offset, GetTerminate(), _str.GetPtr(), _str.GetTerminate());
    }

    [[nodiscard]] bool Compare(const char* _str, const SizeT _offset, const SizeT _count) const
    {
        JAM_ASSERT(_str, "String assignment: _str cannot be null.");
        return StrCompareN(GetPtr() + _offset, GetTerminate(), _str, nullptr, _count);
    }

    [[nodiscard]] bool Compare(const String& _str, const SizeT _offset, const SizeT _count) const
    {
        return StrCompareN(GetPtr() + _offset, GetTerminate(), _str.GetPtr(), _str.GetTerminate(), _count);
    }

    [[nodiscard]] bool CompareIgnoreCase(const char* _str, const SizeT _offset = 0) const
    {
        JAM_ASSERT(_str, "String assignment: _str cannot be null.");
        return StrCompareIgnoreCase(GetPtr() + _offset, GetTerminate(), _str, nullptr);
    }

    [[nodiscard]] bool CompareIgnoreCase(const String& _str, const SizeT _offset = 0) const
    {
        return StrCompareIgnoreCase(GetPtr() + _offset, GetTerminate(), _str.GetPtr(), _str.GetTerminate());
    }

    [[nodiscard]] bool CompareIgnoreCase(const char* _str, const SizeT _offset, const SizeT _count) const
    {
        JAM_ASSERT(_str, "String assignment: _str cannot be null.");
        return StrCompareIgnoreCaseN(GetPtr() + _offset, GetTerminate(), _str, nullptr, _count);
    }

    [[nodiscard]] bool CompareIgnoreCase(const String& _str, const SizeT _offset, const SizeT _count) const
    {
        return StrCompareIgnoreCaseN(GetPtr() + _offset, GetTerminate(), _str.GetPtr(), _str.GetTerminate(), _count);
    }

    [[nodiscard]] std::string ToStdString(const SizeT _offset = 0) const
    {
        JAM_ASSERT(_offset >= 0 && _offset <= GetLength(), "Offset out of bounds.");
        return { GetPtr() + _offset, NumericCast<USizeT>(GetLength() - _offset) };
    }

    [[nodiscard]] std::string ToStdString(const SizeT _offset, const SizeT _count) const
    {
        JAM_ASSERT(_offset >= 0 && _offset + _count <= GetLength(), "Range out of bounds.");
        return { GetPtr() + _offset, NumericCast<USizeT>(_count) };
    }

    [[nodiscard]] std::string_view ToStdStringView(const SizeT _offset = 0) const
    {
        JAM_ASSERT(_offset >= 0 && _offset <= GetLength(), "Offset out of bounds.");
        return { GetPtr() + _offset, NumericCast<USizeT>(GetLength() - _offset) };
    }

    [[nodiscard]] std::string_view ToStdStringView(const SizeT _offset, const SizeT _count) const
    {
        JAM_ASSERT(_offset >= 0 && _offset + _count <= GetLength(), "Range out of bounds.");
        return { GetPtr() + _offset, NumericCast<USizeT>(_count) };
    }

    [[nodiscard]] std::wstring ToStdWString(const SizeT _offset = 0) const
    {
        JAM_ASSERT(_offset >= 0 && _offset <= GetLength(), "Offset out of bounds.");
        const SizeT  len = CalcRequiredUtf16BufferSize(GetPtr() + _offset, GetTerminate());
        std::wstring wstr(len, L'\0');
        ConvertToUtf16(GetPtr() + _offset, GetTerminate(), wstr.data(), len + 1);
        return wstr;
    }

    [[nodiscard]] std::wstring ToStdWString(const SizeT _offset, const SizeT _count) const
    {
        JAM_ASSERT(_offset >= 0 && _offset + _count <= GetLength(), "Range out of bounds.");
        const SizeT  len = CalcRequiredUtf16BufferSize(GetPtr() + _offset, GetPtr() + _offset + _count);
        std::wstring wstr(len, L'\0');
        ConvertToUtf16(GetPtr() + _offset, GetPtr() + _offset + _count, wstr.data(), len + 1);
        return wstr;
    }

    [[nodiscard]] Iter begin()
    {
        return GetPtr();
    }

    [[nodiscard]] Iter end()
    {
        return GetPtr() + GetLength();
    }

    [[nodiscard]] ConstIter begin() const
    {
        return GetPtr();
    }

    [[nodiscard]] ConstIter end() const
    {
        return GetPtr() + GetLength();
    }

    [[nodiscard]] UInt32 Hash32() const
    {
        Fnv1ahHasher32 hasher {};
        hasher.Mix(GetPtr(), GetLength());
        return hasher.Finish();
    }

    [[nodiscard]] UInt64 Hash64() const
    {
        Fnv1ahHasher64 hasher {};
        hasher.Mix(GetPtr(), GetLength());
        return hasher.Finish();
    }

    [[nodiscard]] UInt32 Hash32IgnoreCase() const
    {
        IgnoreCaseFnv1ahHasher32 hasher {};
        hasher.Mix(GetPtr(), GetLength());
        return hasher.Finish();
    }

    [[nodiscard]] UInt64 Hash64IgnoreCase() const
    {
        IgnoreCaseFnv1ahHasher64 hasher {};
        hasher.Mix(GetPtr(), GetLength());
        return hasher.Finish();
    }

    [[nodiscard]] char Back()
    {
        JAM_ASSERT(!IsEmpty(), "String is empty.");
        return operator[](GetLength() - 1);
    }

    [[nodiscard]] char Front()
    {
        JAM_ASSERT(!IsEmpty(), "String is empty.");
        return operator[](0);
    }

    [[nodiscard]] String LTrimed() const
    {
        const char* pStart = nullptr;
        StrLTrim(GetPtr(), GetTerminate(), &pStart);
        return SubStr(pStart - GetPtr());
    }

    [[nodiscard]] String RTrimed() const
    {
        const char* pEnd = nullptr;
        StrRTrim(GetPtr(), GetTerminate(), &pEnd);
        return SubStr(0, pEnd - GetPtr());
    }

    [[nodiscard]] String Trimed() const
    {
        const char* pStart = nullptr;
        const char* pEnd   = nullptr;
        StrTrim(GetPtr(), GetTerminate(), &pStart, &pEnd);
        return SubStr(pStart - GetPtr(), pEnd - pStart);
    }

    // std glue
    using value_type = char;

    void push_back(const value_type& _c)
    {
        Append(_c);
    }

    void push_back(value_type&& _c)
    {
        Append(_c);
    }

private:
    rapidstring m_str = {};
};

// do not guarantee null-termination
class StringView
{
public:
    using Iter = const char*;

    constexpr StringView() = default;

    /* implicit */ constexpr StringView(const char* _str)
        : m_ptr(_str)
        , m_len(StrLen(_str))
    {
        JAM_ASSERT(_str, "Null pointer exception.");
    }

    /* implicit */ constexpr StringView(const char* _str, const SizeT _n)
        : m_ptr(_str)
        , m_len(_n)
    {
        JAM_ASSERT(_str, "Null pointer exception.");
    }

    template<size_t N>
    /* implicit */ constexpr StringView(const char (&_str)[N])
        : m_ptr(_str)
        , m_len(NumericCast<SizeT>(N - 1))
    {
    }

    /* implicit */ StringView(const String& _str)
        : m_ptr(_str.GetPtr())
        , m_len(_str.GetLength())
    {
    }

    /* implicit */ StringView(const std::string& _str)
        : m_ptr(_str.c_str())
        , m_len(NumericCast<SizeT>(_str.length()))
    {
    }

    [[nodiscard]] constexpr const char* GetPtr() const
    {
        return m_ptr;
    }

    [[nodiscard]] constexpr const char* GetTerminate() const
    {
        return m_ptr + m_len;
    }

    [[nodiscard]] constexpr SizeT GetLength() const
    {
        return m_len;
    }

    [[nodiscard]] constexpr bool IsEmpty() const
    {
        return m_len == 0;
    }

    [[nodiscard]] constexpr const char& operator[](const SizeT _index) const
    {
        JAM_ASSERT(_index >= 0 && _index < m_len, "Index out of bounds.");
        return m_ptr[_index];
    }

    constexpr bool operator==(const StringView _other) const
    {
        return StrEqual(GetPtr(), GetTerminate(), _other.GetPtr(), _other.GetTerminate());
    }

    constexpr bool operator==(const char* _str) const
    {
        JAM_ASSERT(_str, "String comparison: _str cannot be null.");
        return StrEqual(GetPtr(), GetTerminate(), _str, nullptr);
    }

    constexpr bool operator!=(const StringView _other) const
    {
        return !(*this == _other);
    }

    constexpr bool operator!=(const char* _str) const
    {
        return !(*this == _str);
    }

    template<typename T>
        requires std::is_integral_v<T>
    [[nodiscard]] constexpr Optional<T> ParseAs() const
    {
        return Jam::ParseAs<T>(GetPtr(), GetTerminate());
    }

    [[nodiscard]] constexpr CharResult Find(const char _c, const SizeT _offset = 0) const
    {
        return StrFind(GetPtr() + _offset, GetTerminate(), _c);
    }

    [[nodiscard]] constexpr CharResult Find(const StringView _other, const SizeT _offset = 0) const
    {
        return StrFind(GetPtr() + _offset, GetTerminate(), _other.GetPtr(), _other.GetTerminate());
    }

    [[nodiscard]] constexpr CharResult RFind(const char _c) const
    {
        return StrRFind(GetPtr(), GetTerminate(), _c);
    }

    [[nodiscard]] constexpr CharResult RFind(const StringView _other) const
    {
        return StrRFind(GetPtr(), GetTerminate(), _other.GetPtr(), _other.GetTerminate());
    }

    [[nodiscard]] constexpr CharResult RFind(const char _c, const SizeT _count) const
    {
        return StrRFind(GetPtr(), GetPtr() + _count, _c);
    }

    [[nodiscard]] constexpr CharResult RFind(const StringView _other, const SizeT _count) const
    {
        return StrRFind(GetPtr(), GetPtr() + _count, _other.GetPtr(), _other.GetTerminate());
    }

    [[nodiscard]] constexpr bool StartsWith(const char _c, const SizeT _offset = 0) const
    {
        return !IsEmpty() && m_ptr[_offset] == _c;
    }

    [[nodiscard]] constexpr bool StartsWith(const StringView _other, const SizeT _offset = 0) const
    {
        return StrStartsWith(GetPtr() + _offset, GetTerminate(), _other.GetPtr(), _other.GetTerminate());
    }

    [[nodiscard]] constexpr bool EndsWith(const char _c) const
    {
        return !IsEmpty() && m_ptr[m_len - 1] == _c;
    }

    [[nodiscard]] constexpr bool EndsWith(const StringView _other) const
    {
        return StrEndsWith(GetPtr(), GetTerminate(), _other.GetPtr(), _other.GetTerminate());
    }

    [[nodiscard]] constexpr bool EndsWith(const char _c, const SizeT _count) const
    {
        return !IsEmpty() && m_ptr[_count - 1] == _c;
    }

    [[nodiscard]] constexpr bool EndsWith(const StringView _other, const SizeT _count) const
    {
        return StrEndsWith(GetPtr(), GetPtr() + _count, _other.GetPtr(), _other.GetTerminate());
    }

    [[nodiscard]] constexpr bool Compare(const StringView _other, const SizeT _offset = 0) const
    {
        return StrCompare(GetPtr() + _offset, GetTerminate(), _other.GetPtr(), _other.GetTerminate());
    }

    [[nodiscard]] constexpr bool Compare(const StringView _other, const SizeT _offset, const SizeT _count) const
    {
        return StrCompareN(GetPtr() + _offset, GetTerminate(), _other.GetPtr(), _other.GetTerminate(), _count);
    }

    [[nodiscard]] constexpr bool CompareIgnoreCase(const StringView _other, const SizeT _offset = 0) const
    {
        return StrCompareIgnoreCase(GetPtr() + _offset, GetTerminate(), _other.GetPtr(), _other.GetTerminate());
    }

    [[nodiscard]] constexpr bool CompareIgnoreCase(const StringView _other, const SizeT _offset, const SizeT _count) const
    {
        return StrCompareIgnoreCaseN(GetPtr() + _offset, GetTerminate(), _other.GetPtr(), _other.GetTerminate(), _count);
    }

    [[nodiscard]] constexpr StringView SubView(const SizeT _pos) const
    {
        JAM_ASSERT(_pos >= 0, "SubView position cannot be negative.");
        JAM_ASSERT(_pos <= m_len, "SubView position exceeds string length.");
        return { m_ptr + _pos, m_len - _pos };
    }

    // this is not guaranteed to be null-terminated
    [[nodiscard]] constexpr StringView SubView(const SizeT _pos, const SizeT _len) const
    {
        JAM_ASSERT(_pos >= 0, "SubView position cannot be negative.");
        JAM_ASSERT(_pos + _len <= m_len, "SubView range exceeds string length.");
        return { m_ptr + _pos, _len };
    }

    [[nodiscard]] String ToString(const SizeT offset = 0) const
    {
        JAM_ASSERT(offset <= m_len, "SubView position exceeds string length.");
        return { m_ptr + offset, m_len - offset };
    }

    [[nodiscard]] String ToString(const SizeT offset, const SizeT _len) const
    {
        JAM_ASSERT(offset + _len <= m_len, "SubView range exceeds string length.");
        return { m_ptr + offset, _len };
    }

    [[nodiscard]] std::string ToStdString(const SizeT offset = 0) const
    {
        JAM_ASSERT(offset <= m_len, "SubView position exceeds string length.");
        return { m_ptr + offset, NumericCast<USizeT>(m_len - offset) };
    }

    [[nodiscard]] std::string ToStdString(const SizeT offset, const SizeT _len) const
    {
        JAM_ASSERT(offset + _len <= m_len, "SubView range exceeds string length.");
        return { m_ptr + offset, NumericCast<USizeT>(_len) };
    }

    [[nodiscard]] std::wstring ToStdWString(const SizeT offset = 0) const
    {
        JAM_ASSERT(offset <= m_len, "SubView position exceeds string length.");
        const SizeT  len = CalcRequiredUtf16BufferSize(m_ptr + offset, m_ptr + m_len);
        std::wstring wstr(len, L'\0');
        ConvertToUtf16(GetPtr() + offset, GetTerminate(), wstr.data(), len + 1);
        return wstr;
    }

    [[nodiscard]] std::wstring ToStdWString(const SizeT offset, const SizeT _len) const
    {
        JAM_ASSERT(offset + _len <= m_len, "SubView range exceeds string length.");
        const SizeT  len = CalcRequiredUtf16BufferSize(m_ptr + offset, m_ptr + offset + _len);
        std::wstring wstr(len, L'\0');
        ConvertToUtf16(GetPtr() + offset, GetPtr() + offset + _len, wstr.data(), len + 1);
        return wstr;
    }

    [[nodiscard]] constexpr std::string_view ToStdStringView() const
    {
        return { m_ptr, NumericCast<USizeT>(m_len) };
    }

    [[nodiscard]] StringView LTrimed() const
    {
        const char* pStart = nullptr;
        StrLTrim(m_ptr, m_ptr + m_len, &pStart);
        return SubView(pStart - m_ptr);
    }

    [[nodiscard]] StringView RTrimed() const
    {
        const char* pEnd = nullptr;
        StrRTrim(m_ptr, m_ptr + m_len, &pEnd);
        return SubView(0, pEnd - m_ptr);
    }

    [[nodiscard]] StringView Trimed() const
    {
        const char* pStart = nullptr;
        const char* pEnd   = nullptr;
        StrTrim(m_ptr, m_ptr + m_len, &pStart, &pEnd);
        return SubView(pStart - m_ptr, pEnd - pStart);
    }

    [[nodiscard]] constexpr UInt32 Hash32() const
    {
        Fnv1ahHasher32 hasher {};
        hasher.Mix(m_ptr, m_len);
        return hasher.Finish();
    }

    [[nodiscard]] constexpr UInt64 Hash64() const
    {
        Fnv1ahHasher64 hasher {};
        hasher.Mix(m_ptr, m_len);
        return hasher.Finish();
    }

    [[nodiscard]] constexpr UInt32 Hash32IgnoreCase() const
    {
        IgnoreCaseFnv1ahHasher32 hasher {};
        hasher.Mix(m_ptr, m_len);
        return hasher.Finish();
    }

    [[nodiscard]] constexpr UInt64 Hash64IgnoreCase() const
    {
        IgnoreCaseFnv1ahHasher64 hasher {};
        hasher.Mix(m_ptr, m_len);
        return hasher.Finish();
    }

    [[nodiscard]] constexpr char Back() const
    {
        JAM_ASSERT(!IsEmpty(), "StringView is empty.");
        return operator[](m_len - 1);
    }

    [[nodiscard]] constexpr char Front() const
    {
        JAM_ASSERT(!IsEmpty(), "StringView is empty.");
        return operator[](0);
    }

    [[nodiscard]] constexpr Iter begin() const
    {
        return m_ptr;
    }

    [[nodiscard]] constexpr Iter end() const
    {
        return m_ptr + m_len;
    }

private:
    const char* m_ptr = "";
    SizeT       m_len = 0;
};

////////// String with StringView glue //////////

inline bool operator==(const String& _lhs, const StringView _rhs)
{
    return _rhs.operator==(_lhs);
}

inline bool operator!=(const String& _lhs, const StringView _rhs)
{
    return _rhs.operator!=(_lhs);
}

inline String operator+(const String& _lhs, const StringView _rhs)
{
    String result { _lhs };
    result.Append(_rhs.GetPtr(), _rhs.GetLength());
    return result;
}

inline String& operator+=(String& _lhs, const StringView _rhs)
{
    _lhs.Append(_rhs.GetPtr(), _rhs.GetLength());
    return _lhs;
}

////////// Hash functions shortcuts //////////

[[nodiscard]] constexpr UInt32 HashString32(const StringView _str)
{
    return _str.Hash32();
}

[[nodiscard]] constexpr UInt64 HashString64(const StringView _str)
{
    return _str.Hash64();
}

[[nodiscard]] constexpr UInt32 HashString32IgnoreCase(const StringView _str)
{
    return _str.Hash32IgnoreCase();
}

[[nodiscard]] constexpr UInt64 HashString64IgnoreCase(const StringView _str)
{
    return _str.Hash64IgnoreCase();
}

}   // namespace Jam

////////// std + external glue //////////

template<>
struct std::hash<Jam::String>
{
    std::size_t operator()(const Jam::String& _str) const noexcept
    {
        return _str.Hash64();
    }
};

template<>
struct std::hash<Jam::StringView>
{
    std::size_t operator()(const Jam::StringView& _strView) const noexcept
    {
        return _strView.Hash64();
    }
};

template<>
struct ankerl::unordered_dense::hash<Jam::String>
{
    [[nodiscard]] std::size_t operator()(const Jam::String& _str) const noexcept
    {
        return ankerl::unordered_dense::detail::wyhash::hash(_str.GetPtr(), _str.GetLength());
    }
};

template<>
struct ankerl::unordered_dense::hash<Jam::StringView>
{
    [[nodiscard]] std::size_t operator()(const Jam::StringView& _strView) const noexcept
    {
        return ankerl::unordered_dense::detail::wyhash::hash(_strView.GetPtr(), _strView.GetLength());
    }
};

template<>
struct fmt::formatter<Jam::String>
{
    constexpr auto parse(const format_parse_context& ctx) const
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const Jam::String& str, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", fmt::string_view { str.GetPtr(), Jam::NumericCast<Jam::USizeT>(str.GetLength()) });
    }
};

template<>
struct fmt::formatter<Jam::StringView>
{
    constexpr auto parse(const format_parse_context& ctx) const
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const Jam::StringView& strView, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", fmt::string_view { strView.GetPtr(), Jam::NumericCast<Jam::USizeT>(strView.GetLength()) });
    }
};
