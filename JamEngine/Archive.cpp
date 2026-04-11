#include "pch.h"
#include "Archive.h"

namespace Jam
{

void WriteToArchive(ArchiveWriter& _writer, const String& _value)
{
    _writer.Write(_value.ToStdStringView());
}

Error ReadFromArchive(ArchiveReader& _reader, String& _value)
{
    std::string_view str = {};
    if (const Error err = _reader.Read(str))
    {
        return err;
    }

    _value.Set(str.data(), str.size());
    return Error::None;
}

void WriteToArchive(ArchiveWriter& _writer, const StringView _value)
{
    _writer.Write(_value.ToStdStringView());
}

Error ReadFromArchive(ArchiveReader& _reader, StringView& _value)
{
    std::string_view str = {};
    if (const Error err = _reader.Read(str))
    {
        return err;
    }

    _value = { str.data(), NarrowCast<SizeT>(str.size()) };
    return Error::None;
}

void WriteToArchive(
    ArchiveWriter& _writer,
    const Vec3     _value)
{
    _writer.BeginArray();
    _writer.WriteElement(_value.x);
    _writer.WriteElement(_value.y);
    _writer.WriteElement(_value.z);
    _writer.EndArray();
}

Error ReadFromArchive(
    ArchiveReader& _reader,
    Vec3&          _value)
{
    if (const Error err = _reader.ReadAt(0, _value.x))
    {
        return err;
    }

    if (const Error err = _reader.ReadAt(1, _value.y))
    {
        return err;
    }

    if (const Error err = _reader.ReadAt(2, _value.z))
    {
        return err;
    }

    return Error::None;
}

void WriteToArchive(
    ArchiveWriter& _writer,
    const Vec2     _value)
{
    _writer.BeginArray();
    _writer.WriteElement(_value.x);
    _writer.WriteElement(_value.y);
    _writer.EndArray();
}

Error ReadFromArchive(
    ArchiveReader& _reader,
    Vec2&          _value)
{
    if (const Error err = _reader.ReadAt(0, _value.x))
    {
        return err;
    }

    if (const Error err = _reader.ReadAt(1, _value.y))
    {
        return err;
    }

    return Error::None;
}

void WriteToArchive(
    ArchiveWriter& _writer,
    const Mtx4&    _value)
{
    _writer.BeginArray();
    for (Int32 row = 0; row < 4; ++row)
    {
        for (Int32 col = 0; col < 4; ++col)
        {
            _writer.WriteElement(_value(row, col));
        }
    }
    _writer.EndArray();
}

Error ReadFromArchive(
    ArchiveReader& _reader,
    Mtx4&          _value)
{
    for (Int32 row = 0; row < 4; ++row)
    {
        for (Int32 col = 0; col < 4; ++col)
        {
            if (const Error err = _reader.ReadAt(row * 4 + col, _value(row, col)))
            {
                return err;
            }
        }
    }
    return Error::None;
}

}   // namespace Jam