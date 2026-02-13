#include "pch.h"
#include "String.h"

namespace Jam
{

SizeT CalcRequiredUtf16BufferSize(
    const char* _start,
    const char* _endOrNull)
{
    const SizeT len = StrLen(_start, _endOrNull);
    if (len == 0)
    {
        return 0;
    }

    return MultiByteToWideChar(CP_UTF8, 0, _start, NumericCast<Int32>(len), NULL, 0);
}

SizeT CalcRequiredUtf8BufferSize(
    const wchar_t* _start,
    const wchar_t* _endOrNull)
{
    const SizeT len = StrLen(_start, _endOrNull);
    if (len == 0)
    {
        return 0;
    }

    return WideCharToMultiByte(CP_UTF8, 0, _start, NumericCast<Int32>(len), NULL, 0, NULL, NULL);
}

SizeT ConvertToUtf16(
    const char* _start,
    const char* _endOrNull,
    wchar_t*    _pOutBuf,
    const SizeT _outBufCap)
{
    JAM_ASSERT(_outBufCap > 0, "Output buffer capacity must be greater than zero.");

    const SizeT len = StrLen(_start, _endOrNull);
    if (len == 0)
    {
        _pOutBuf[0] = L'\0';
        return 0;
    }

    const SizeT written = ::MultiByteToWideChar(CP_UTF8, 0, _start, NumericCast<Int32>(len), _pOutBuf, NumericCast<Int32>(_outBufCap - 1));
    if (written == 0)
    {
        _pOutBuf[0] = L'\0';
        return 0;
    }

    _pOutBuf[written] = L'\0';
    return written;
}

SizeT ConvertToUtf8(
    const wchar_t* _start,
    const wchar_t* _endOrNull,
    char*          _pOutBuf,
    const SizeT    _outBufCap)
{
    JAM_ASSERT(_outBufCap > 0, "Output buffer capacity must be greater than zero.");

    const SizeT len = StrLen(_start, _endOrNull);
    if (len == 0)
    {
        _pOutBuf[0] = '\0';
        return 0;
    }

    const SizeT written = ::WideCharToMultiByte(CP_UTF8, 0, _start, NumericCast<Int32>(len), _pOutBuf, NumericCast<Int32>(_outBufCap - 1), NULL, NULL);
    if (written == 0)
    {
        _pOutBuf[0] = '\0';
        return 0;
    }

    _pOutBuf[written] = '\0';
    return written;
}

}   // namespace Jam