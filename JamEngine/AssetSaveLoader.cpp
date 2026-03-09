#include "pch.h"
#include "AssetSaveLoader.h"

#include "BoundingVolume.h"

namespace Jam
{

String StringFrom(
    const StringViewDto _str)
{
    return { _str.data(), _str.size() };
}

StringView StringViewFrom(
    const StringViewDto _str)
{
    return { _str.data(), _str.size() };
}

StringViewDto StringViewDtoFrom(
    const StringView _str)
{
    return { _str.GetPtr(), NumericCast<StringViewDto::msize_t>(_str.GetLength()) };
}

Vec3Dto Vec3Dto::FromVec3(const Vector3& _v)
{
    Vec3Dto vec = {};
    vec.vec[0]  = _v.x;
    vec.vec[1]  = _v.y;
    vec.vec[2]  = _v.z;
    return vec;
}

Vector3 Vec3Dto::ToVec3() const
{
    return { vec[0], vec[1], vec[2] };
}

Mtx4Dto Mtx4Dto::FromMat4(
    Matrix4x4 _mtx)
{
    Mtx4Dto mtx = {};
    mtx.mtx[0]  = _mtx._11;
    mtx.mtx[1]  = _mtx._12;
    mtx.mtx[2]  = _mtx._13;
    mtx.mtx[3]  = _mtx._14;
    mtx.mtx[4]  = _mtx._21;
    mtx.mtx[5]  = _mtx._22;
    mtx.mtx[6]  = _mtx._23;
    mtx.mtx[7]  = _mtx._24;
    mtx.mtx[8]  = _mtx._31;
    mtx.mtx[9]  = _mtx._32;
    mtx.mtx[10] = _mtx._33;
    mtx.mtx[11] = _mtx._34;
    mtx.mtx[12] = _mtx._41;
    mtx.mtx[13] = _mtx._42;
    mtx.mtx[14] = _mtx._43;
    mtx.mtx[15] = _mtx._44;
    return mtx;
}

Matrix4x4 Mtx4Dto::ToMtx4() const
{
    return { mtx[0],
             mtx[1],
             mtx[2],
             mtx[3],
             mtx[4],
             mtx[5],
             mtx[6],
             mtx[7],
             mtx[8],
             mtx[9],
             mtx[10],
             mtx[11],
             mtx[12],
             mtx[13],
             mtx[14],
             mtx[15] };
}

AabbDto AabbDto::FromAabb(
    const Aabb& _aabb)
{
    AabbDto aabb = {};
    aabb.min     = Vec3Dto::FromVec3(_aabb.min);
    aabb.max     = Vec3Dto::FromVec3(_aabb.max);
    return aabb;
}

Aabb AabbDto::ToAabb() const
{
    return Aabb { min.ToVec3(), max.ToVec3() };
}

}   // namespace Jam