#include "pch.h"
#include "Archive.h"

namespace Jam
{

void WriteToArchive(ArchiveWriter& _ar, Vec2 _v)
{
    _ar.BeginArray();
    _ar.Write(_v.x);
    _ar.Write(_v.y);
    _ar.EndArray();
}

void WriteToArchive(ArchiveWriter& _ar, Vec3 _v)
{
    _ar.BeginArray();
    _ar.Write(_v.x);
    _ar.Write(_v.y);
    _ar.Write(_v.z);
    _ar.EndArray();
}

void WriteToArchive(ArchiveWriter& _ar, Vec4 _v)
{
    _ar.BeginArray();
    _ar.Write(_v.x);
    _ar.Write(_v.y);
    _ar.Write(_v.z);
    _ar.Write(_v.w);
    _ar.EndArray();
}

void WriteToArchive(ArchiveWriter& _ar, Vec2i _v)
{
    _ar.BeginArray();
    _ar.Write(_v.x);
    _ar.Write(_v.y);
    _ar.EndArray();
}

void WriteToArchive(ArchiveWriter& _ar, Vec3i _v)
{
    _ar.BeginArray();
    _ar.Write(_v.x);
    _ar.Write(_v.y);
    _ar.Write(_v.z);
    _ar.EndArray();
}

void WriteToArchive(ArchiveWriter& _ar, Vec4i _v)
{
    _ar.BeginArray();
    _ar.Write(_v.x);
    _ar.Write(_v.y);
    _ar.Write(_v.z);
    _ar.Write(_v.w);
    _ar.EndArray();
}

void WriteToArchive(ArchiveWriter& _ar, Quat _q)
{
    _ar.BeginArray();
    _ar.Write(_q.x);
    _ar.Write(_q.y);
    _ar.Write(_q.z);
    _ar.Write(_q.w);
    _ar.EndArray();
}

void WriteToArchive(ArchiveWriter& _ar, const Mtx4& _m)
{
    _ar.BeginArray();
    _ar.Write(_m.m00);
    _ar.Write(_m.m01);
    _ar.Write(_m.m02);
    _ar.Write(_m.m03);
    _ar.Write(_m.m10);
    _ar.Write(_m.m11);
    _ar.Write(_m.m12);
    _ar.Write(_m.m13);
    _ar.Write(_m.m20);
    _ar.Write(_m.m21);
    _ar.Write(_m.m22);
    _ar.Write(_m.m23);
    _ar.Write(_m.m30);
    _ar.Write(_m.m31);
    _ar.Write(_m.m32);
    _ar.Write(_m.m33);
    _ar.EndArray();
}

Error ReadFromArchive(ArchiveReader& _ar, Vec2& _v)
{
    if (const Error err = _ar.ReadAt(0, _v.x))
    {
        return err;
    }

    if (const Error err = _ar.ReadAt(1, _v.y))
    {
        return err;
    }

    return Error::None;
}

Error ReadFromArchive(ArchiveReader& _ar, Vec3& _v)
{
    if (const Error err = _ar.ReadAt(0, _v.x))
    {
        return err;
    }

    if (const Error err = _ar.ReadAt(1, _v.y))
    {
        return err;
    }

    if (const Error err = _ar.ReadAt(2, _v.z))
    {
        return err;
    }

    return Error::None;
}

Error ReadFromArchive(ArchiveReader& _ar, Vec4& _v)
{
    if (const Error err = _ar.ReadAt(0, _v.x))
    {
        return err;
    }

    if (const Error err = _ar.ReadAt(1, _v.y))
    {
        return err;
    }

    if (const Error err = _ar.ReadAt(2, _v.z))
    {
        return err;
    }

    if (const Error err = _ar.ReadAt(3, _v.w))
    {
        return err;
    }

    return Error::None;
}

Error ReadFromArchive(ArchiveReader& _ar, Vec2i& _v)
{
    if (const Error err = _ar.ReadAt(0, _v.x))
    {
        return err;
    }

    if (const Error err = _ar.ReadAt(1, _v.y))
    {
        return err;
    }

    return Error::None;
}

Error ReadFromArchive(ArchiveReader& _ar, Vec3i& _v)
{
    if (const Error err = _ar.ReadAt(0, _v.x))
    {
        return err;
    }

    if (const Error err = _ar.ReadAt(1, _v.y))
    {
        return err;
    }

    if (const Error err = _ar.ReadAt(2, _v.z))
    {
        return err;
    }

    return Error::None;
}

Error ReadFromArchive(ArchiveReader& _ar, Vec4i& _v)
{
    if (const Error err = _ar.ReadAt(0, _v.x))
    {
        return err;
    }

    if (const Error err = _ar.ReadAt(1, _v.y))
    {
        return err;
    }

    if (const Error err = _ar.ReadAt(2, _v.z))
    {
        return err;
    }

    if (const Error err = _ar.ReadAt(3, _v.w))
    {
        return err;
    }

    return Error::None;
}

Error ReadFromArchive(ArchiveReader& _ar, Quat& _q)
{
    if (const Error err = _ar.ReadAt(0, _q.x))
    {
        return err;
    }

    if (const Error err = _ar.ReadAt(1, _q.y))
    {
        return err;
    }

    if (const Error err = _ar.ReadAt(2, _q.z))
    {
        return err;
    }

    if (const Error err = _ar.ReadAt(3, _q.w))
    {
        return err;
    }

    return Error::None;
}

Error ReadFromArchive(ArchiveReader& _ar, Mtx4& _m)
{

#define READAT_OR_RETURN(_index, _elem)              \
    if (const Error err = _ar.ReadAt(_index, _elem)) \
    return err

    READAT_OR_RETURN(0, _m.m00);
    READAT_OR_RETURN(1, _m.m01);
    READAT_OR_RETURN(2, _m.m02);
    READAT_OR_RETURN(3, _m.m03);
    READAT_OR_RETURN(4, _m.m10);
    READAT_OR_RETURN(5, _m.m11);
    READAT_OR_RETURN(6, _m.m12);
    READAT_OR_RETURN(7, _m.m13);
    READAT_OR_RETURN(8, _m.m20);
    READAT_OR_RETURN(9, _m.m21);
    READAT_OR_RETURN(10, _m.m22);
    READAT_OR_RETURN(11, _m.m23);
    READAT_OR_RETURN(12, _m.m30);
    READAT_OR_RETURN(13, _m.m31);
    READAT_OR_RETURN(14, _m.m32);
    READAT_OR_RETURN(15, _m.m33);

#undef READAT_OR_RETURN

    return Error::None;
}

}   // namespace Jam