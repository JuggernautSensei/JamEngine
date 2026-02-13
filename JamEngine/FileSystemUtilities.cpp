#include "pch.h"

#include "FileSystemUtilities.h"

#include "Archive.h"

namespace Jam
{

bool IsBasePathOf(
    const Path& _base,
    const Path& _path)
{
    if (_path.empty() || _base.empty())
    {
        return false;
    }

    if (_base == _path)
    {
        return true;
    }

    const Path base     = fs::weakly_canonical(_base);
    const Path path     = fs::weakly_canonical(_path);
    StringView baseView = base.native();
    StringView pathView = path.native();

    // base가 path보다 더 길 수 없음
    if (pathView.GetLength() < baseView.GetLength())
    {
        return false;
    }

    for (Int32 i = 0; i < baseView.GetLength(); ++i)
    {
        if (baseView[i] != pathView[i])
        {
            return false;
        }
    }

    // "/apple"이 "/app"의 베이스가 되는 것을 방지
    // base의 마지막 문자가 구분자가 아니라면, path의 그 다음 문자가 구분자여야 함
    if (baseView.Back() != k_preferedPathSeparator)
    {
        if (pathView[baseView.GetLength()] != k_preferedPathSeparator)
        {
            return false;
        }
    }

    return true;
}

StringView ParentPathViewOf(
    const Path& _path)
{
    if (_path.empty())
    {
        return {};
    }

    const StringView path = _path.native();
    const CharResult ret  = path.RFind(k_preferedPathSeparator);
    return ret.IsSucceeded() ? path.SubView(0, ret.pos) : StringView {};
}

StringView BaseNameViewOf(
    const Path& _path)
{
    const StringView filename = FileNameViewOf(_path);
    const CharResult ret      = filename.RFind('.');
    return ret.IsSucceeded() ? filename.SubView(0, ret.pos) : filename;
}

StringView FileNameViewOf(
    const Path& _path)
{
    if (_path.empty())
    {
        return {};
    }

    const StringView path = _path.native();
    const CharResult ret  = path.RFind(k_preferedPathSeparator);
    return ret.IsSucceeded() ? path.SubView(ret.pos + 1) : path;
}

StringView ExtensionViewOf(
    const Path& _path)
{
    const StringView filename = FileNameViewOf(_path);
    const CharResult result   = filename.RFind('.');
    return result.IsSucceeded() ? filename.SubView(result.pos) : StringView {};
}

String ParentPathOf(
    const Path& _path)
{
    return ParentPathViewOf(_path).ToString();
}

String BaseNameOf(
    const Path& _path)
{
    return BaseNameViewOf(_path).ToString();
}

String FileNameOf(const Path& _path)
{
    return FileNameViewOf(_path).ToString();
}

String ExtensionOf(const Path& _path)
{
    return ExtensionViewOf(_path).ToString();
}

Path PathFrom(
    const StringView _str)
{
    return Path { _str.ToStdString() };
}

bool MatchesExtension(
    const Path&      _path,
    const StringView _extension)
{
    JAM_ASSERT(_extension.IsEmpty() == false, "Extension to match cannot be empty.");

    StringView pathExt = ExtensionViewOf(_path);
    if (pathExt.IsEmpty())
    {
        return false;
    }
    pathExt = pathExt.SubView(1);   // skip '.'

    // skip '.'
    const StringView ext = _extension.StartsWith('.') ? _extension.SubView(1) : _extension;
    return pathExt.CompareIgnoreCase(ext);
}

void ReplaceExtension(
    Path&            _path,
    const StringView _extension)
{
    _path.replace_extension(_extension.ToStdString());
}

Path ReplacedExtension(
    const Path&      _path,
    const StringView _extension)
{
    Path path = _path;
    ReplaceExtension(path, _extension);
    return path;
}

void JoinPath(
    Path&            _base,
    const StringView _path)
{
    _base /= _path.ToStdString();
}

Path JoinedPath(
    const Path&      _base,
    const StringView _path)
{
    return _base / _path.ToStdString();
}

void WriteToArchive(
    ArchiveWriter& _writer,
    const Path&    _path)
{
    _writer.Write(_path.native());
}

Error ReadFromArchive(
    ArchiveReader& _reader,
    Path&          _path)
{
    StringView str = {};
    if (const Error err = _reader.Read(str))
    {
        return err;
    }

    _path = Path { str.ToStdString() };
    return Error::None;
}

}   // namespace Jam