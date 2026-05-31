#pragma once

namespace Jam
{

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

        ::memcpy(_pOutBuf, _str, _len * sizeof(TChar));
        _pOutBuf[_len] = CharTraits<TChar>::kNullChar;
        return _len;
    }
};
}