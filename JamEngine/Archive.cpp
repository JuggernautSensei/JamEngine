#include "pch.h"
#include "Archive.h"

namespace Jam
{

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
    for (Int32 i = 0; i < 16; ++i)
    {
        _writer.WriteElement(_value.m[i]);
    }
    _writer.EndArray();
}

Error ReadFromArchive(
    ArchiveReader& _reader,
    Mtx4&          _value)
{
    for (Int32 i = 0; i < 16; ++i)
    {
        if (const Error err = _reader.ReadAt(i, _value.m[i]))
        {
            return err;
        }
    }
    return Error::None;
}

}   // namespace Jam