#include "pch.h"
#include "AdvString.h"

namespace V2
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
    const SizeT _bufCap)
{
    JAM_ASSERT(_bufCap > 0, "Output buffer capacity must be greater than zero.");

    const SizeT len = StrLen(_start, _endOrNull);
    if (len == 0)
    {
        _pOutBuf[0] = L'\0';
        return 0;
    }

    const SizeT written = ::MultiByteToWideChar(CP_UTF8, 0, _start, NumericCast<Int32>(len), _pOutBuf, NumericCast<Int32>(_bufCap - 1));
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
    const SizeT    _bufCap)
{
    JAM_ASSERT(_bufCap > 0, "Output buffer capacity must be greater than zero.");

    const SizeT len = StrLen(_start, _endOrNull);
    if (len == 0)
    {
        _pOutBuf[0] = '\0';
        return 0;
    }

    const SizeT written = ::WideCharToMultiByte(CP_UTF8, 0, _start, NumericCast<Int32>(len), _pOutBuf, NumericCast<Int32>(_bufCap - 1), NULL, NULL);
    if (written == 0)
    {
        _pOutBuf[0] = '\0';
        return 0;
    }

    _pOutBuf[written] = '\0';
    return written;
}

bool StringView::IsAbsolute() const
{
    return (GetLength() >= 2 && IsAlphabet(operator[](0)) && operator[](1) == ':')   // drive letter, ex) "C:" or "D:"
        || (GetLength() >= 1 && IsPathSeparator(operator[](0)));                     // root, ex) "/"
}

bool StringView::IsRelative() const
{
    return !IsAbsolute();
}

bool StringView::IsDirectory() const
{
    return !IsEmpty() && IsPathSeparator(Back());
}

bool StringView::IsRegularFile() const
{
    return !IsEmpty() && !IsPathSeparator(Back());
}

bool StringView::IsBasePathOf(
    const StringView _path)
{
    // todo..
    return false;
}

StringView StringView::GetParentPath() const
{
    const CharResult lastSep = FindLastPathSeparator();
    if (lastSep.IsFailed())
    {
        return {};
    }
    return SubView(0, lastSep.pos);
}

StringView StringView::GetFileName() const
{
    const CharResult lastSep = FindLastPathSeparator();
    if (lastSep.IsFailed())
    {
        return *this;   // aaa -> file name is "aaa"
    }

    return SubView(lastSep.pos + 1);
}

StringView StringView::GetBaseName() const
{
    const StringView fileName = GetFileName();
    const CharResult lastDot  = fileName.RFind('.', fileName.GetLength());
    if (lastDot.IsFailed())
    {
        return fileName;   // aaa -> base name is "aaa"
    }
    return fileName.SubView(0, lastDot.pos);
}

StringView StringView::GetExtension() const
{
    const StringView fileName = GetFileName();
    const CharResult lastDot  = fileName.RFind('.', fileName.GetLength());
    if (lastDot.IsFailed())
    {
        return {};   // aaa -> extension is ""
    }
    return fileName.SubView(lastDot.pos);
}

CharResult StringView::FindLastPathSeparator() const
{
    for (SizeT i = GetLength(); i > 0; --i)
    {
        if (IsPathSeparator(operator[](i - 1)))
        {
            return { i - 1 };
        }
    }
    return CharResult::Failed();
}

void String::Simplify()
{
    JAM_ASSERT(GetLength() < 1024, "Simplify: path length exceeds the limit of 1024 characters.");

    if (IsEmpty())
    {
        Set('.');
        return;
    }

    // cpy to local buffer since we need to read the original string while modifying it.
    char src[1024];
    std::memcpy(src, GetPtr(), GetLength());   // to do change to ScratchBuffer

    const SizeT srcLen = GetLength();
    Clear();

    SizeT idx    = 0;
    SizeT dotdot = 0;

    // drive letter, ex) "C:" or "D:"
    if (srcLen >= 2 && IsAlphabet(src[0]) && src[1] == ':')
    {
        Append(ToUpper(src[0]));
        Append(':');

        idx += 2;
        dotdot = GetLength();
    }

    const bool bIsRoot = (idx < srcLen) && IsPathSeparator(src[idx]);
    if (bIsRoot)
    {
        Append('/');
        ++idx;
        dotdot = GetLength();
    }

    bool trailingSlash = false;
    while (idx < srcLen)
    {
        if (IsPathSeparator(src[idx]))
        {
            ++idx;
            trailingSlash = (idx == srcLen);
            continue;
        }

        if (src[idx] == '.')
        {
            if (idx + 1 == srcLen || IsPathSeparator(src[idx + 1]))
            {
                ++idx;
                continue;
            }

            if ('.' == src[idx + 1] && (idx + 2 == srcLen || IsPathSeparator(src[idx + 2])))
            {
                idx += 2;

                if (dotdot < GetLength())
                {
                    SizeT i = GetLength() - 1;
                    for (; i > dotdot && !IsPathSeparator(operator[](i)); --i)
                    {
                    }
                    Resize(i);
                }
                else if (!bIsRoot)
                {
                    if (GetLength() > 0)
                    {
                        Append('/');
                    }

                    Append("..");
                    dotdot = GetLength();
                }

                continue;
            }
        }

        // default: actual name
        if ((bIsRoot && GetLength() > dotdot) || (!bIsRoot && GetLength() > 0))
        {
            Append('/');
        }

        for (; idx < srcLen && !IsPathSeparator(src[idx]); ++idx)
        {
            Append(src[idx]);
        }
    }

    if (IsEmpty())
    {
        Append('.');
    }
    else if (trailingSlash)
    {
        Append('/');
    }
}

void String::RelativeFrom(
    StringView _path)
{
}

void String::Absolutize()
{
}

bool String::IsAbsolute() const
{
}

bool String::IsRelative() const
{
}

bool String::IsDirectory() const
{
}

bool String::IsRegularFile() const
{
}

bool String::IsBasePathOf(StringView _path)
{
}

StringView String::GetParentPath() const
{
    return StringView { *this }.GetParentPath();
}

StringView String::GetFileName() const
{
    return StringView { *this }.GetFileName();
}

StringView String::GetBaseName() const
{
    return StringView { *this }.GetBaseName();
}

StringView String::GetExtension() const
{
    return StringView { *this }.GetExtension();
}

String String::CurrentPath()
{
}

String String::ExecutablePath()
{
}

String String::HomePath()
{
}

String String::TempPath()
{
}

void String::Join(
    const StringView _path)
{
    if (_path.IsEmpty())
    {
        return;
    }

    if (IsEmpty())
    {
        *this = _path;
        return;
    }

    if (!IsPathSeparator(operator[](GetLength() - 1)) && !IsPathSeparator(_path[0]))
    {
        Append('/');
    }

    Append(_path);
}

}   // namespace V2