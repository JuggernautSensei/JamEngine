#include "pch.h"
#include "AssetSaveLoader.h"

#include "BoundingVolume.h"

namespace Jam
{

namespace Dto
{
    StringView Wrap(
        const Jam::StringView _str)
    {
        return { _str.GetPtr(), static_cast<StringView::msize_t>(_str.GetLength()) };
    }

    Jam::StringView Unwrap(
        const StringView _str)
    {
        return { _str.data(), _str.size() };
    }

    Vec3 Wrap(
        const Jam::Vec3& _vec)
    {
        return { _vec.x, _vec.y, _vec.z };
    }

    Jam::Vec3 Unwrap(
        const Vec3& _vec)
    {
        return { _vec.x, _vec.y, _vec.z };
    }

    Matrix Wrap(
        const Jam::Mtx4& _mtx)
    {
        return {
            _mtx[0][0],
            _mtx[0][1],
            _mtx[0][2],
            _mtx[0][3],

            _mtx[1][0],
            _mtx[1][1],
            _mtx[1][2],
            _mtx[1][3],

            _mtx[2][0],
            _mtx[2][1],
            _mtx[2][2],
            _mtx[2][3],

            _mtx[3][0],
            _mtx[3][1],
            _mtx[3][2],
            _mtx[3][3],
        };
    }

    Jam::Mtx4 Unwrap(
        const Matrix& _mtx)
    {
        return {
            _mtx.m[0],
            _mtx.m[1],
            _mtx.m[2],
            _mtx.m[3],

            _mtx.m[4],
            _mtx.m[5],
            _mtx.m[6],
            _mtx.m[7],

            _mtx.m[8],
            _mtx.m[9],
            _mtx.m[10],
            _mtx.m[11],

            _mtx.m[12],
            _mtx.m[13],
            _mtx.m[14],
            _mtx.m[15],
        };
    }

    Aabb Wrap(
        const Jam::Aabb& _aabb)
    {
        return { Wrap(_aabb.center), Wrap(_aabb.extents) };
    }

    Jam::Aabb Unwrap(
        const Aabb& _aabb)
    {
        return { Unwrap(_aabb.center), Unwrap(_aabb.extents) };
    }
}   // namespace Dto

}   // namespace Jam