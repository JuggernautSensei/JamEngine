#include "pch.h"
#include "Archive.h"

namespace Jam
{

void WriteToArchive(
    ArchiveWriter& _writer,
    const Vec3     _value)
{
    _writer.PushBack(_value.x);
    _writer.PushBack(_value.y);
    _writer.PushBack(_value.z);
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
    _writer.PushBack(_value.x);
    _writer.PushBack(_value.y);
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
    for (Int32 row = 0; row < 4; ++row)
    {
        for (Int32 col = 0; col < 4; ++col)
        {
            _writer.PushBack(_value(row, col));
        }
    }
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