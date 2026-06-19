#pragma once

namespace Jam
{

// 1. Char 데이터 타입 간의 인코딩 및 디코딩 유틸리티
// 2. null 문자를 포함하지 않음
// 3. _outBuf == nullptr && _bufCap == 0인 경우, 문자열을 인코딩하는 데 필요한 글자 수를 반환. 이때 반환되는 글자 수에는 null 문자에 대한 공간이 포함되지 않음.

template<typename TFromChar, typename TToChar>
struct CharEncoder
{
    static_assert(AlwaysFalseV<TFromChar>, "CharEncoder: Unsupported character types.");
    static_assert(AlwaysFalseV<TToChar>, "CharEncoder: Unsupported character types.");
};

template<>
struct CharEncoder<char, wchar_t>
{
    static SizeT Encode(const char* _str, const SizeT _len, wchar_t* _pOutBuf, const SizeT _bufCap)
    {
        JAM_ASSERT(_str, "CharEncoder<char, wchar_t>::Encode: _str cannot be null.");
        return ::MultiByteToWideChar(CP_UTF8, 0, _str, static_cast<Int32>(_len), _pOutBuf, static_cast<Int32>(_bufCap));
    }
};

template<>
struct CharEncoder<wchar_t, char>
{
    static SizeT Encode(const wchar_t* _str, const SizeT _len, char* _pOutBuf, const SizeT _bufCap)
    {
        JAM_ASSERT(_str, "CharEncoder<wchar_t, char>::Encode: _str cannot be null.");
        return ::WideCharToMultiByte(CP_UTF8, 0, _str, static_cast<Int32>(_len), _pOutBuf, static_cast<Int32>(_bufCap), nullptr, nullptr);
    }
};

template<IsCharT TChar>
struct CharEncoder<TChar, TChar>
{
    static SizeT Encode(const TChar* _str, const SizeT _len, TChar* _pOutBuf, const SizeT _bufCap)
    {
        JAM_ASSERT(_str, "CharEncoder<TChar, TChar>::Encode: _str cannot be null.");
        if (!_pOutBuf)
        {
            return _len;
        }

        JAM_ASSERT(_bufCap > _len, "CharEncoder<TChar, TChar::Encode: _bufCap is smaller than _len");
        CharTraits<TChar>::Runtime::MemCpy(_pOutBuf, _str, _len);
        return _len;
    }
};
}   // namespace Jam