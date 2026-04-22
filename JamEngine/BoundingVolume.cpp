#include "pch.h"
#include "BoundingVolume.h"

namespace Jam
{

eRegion CalcRegion(const bool _bRight, const bool _bTop, const bool _bFar)
{
    return static_cast<eRegion>((static_cast<int>(_bRight) << 2) | (static_cast<int>(_bTop) << 1) | static_cast<int>(_bFar));
}

bool IsRight(const eRegion _region)
{
    return (static_cast<int>(_region) & 0b100) != 0;
}

bool IsTop(const eRegion _region)
{
    return (static_cast<int>(_region) & 0b010) != 0;
}

bool IsFar(const eRegion _region)
{
    return (static_cast<int>(_region) & 0b001) != 0;
}

Aabb::Aabb(const Vec3 _center, const Vec3 _extents)
    : center(_center)
    , extents(_extents)
{
}

Aabb Aabb::FromMinMax(const Vec3 _min, const Vec3 _max)
{
    const Vec3 c = (_min + _max) * 0.5f;
    const Vec3 e = (_max - _min) * 0.5f;
    return Aabb { c, e };
}

Aabb Aabb::Transform(const Mtx4& _mtx) const
{
    const Vec3 nC = _mtx.Mult(center);

    const Vec3 ex = { Abs(_mtx.m[0][0]) * extents.x, Abs(_mtx.m[1][0]) * extents.x, Abs(_mtx.m[2][0]) * extents.x };
    const Vec3 ey = { Abs(_mtx.m[0][1]) * extents.y, Abs(_mtx.m[1][1]) * extents.y, Abs(_mtx.m[2][1]) * extents.y };
    const Vec3 ez = { Abs(_mtx.m[0][2]) * extents.z, Abs(_mtx.m[1][2]) * extents.z, Abs(_mtx.m[2][2]) * extents.z };
    const Vec3 nE = (ex + ey + ez);
    return { nC, nE };
}

void Aabb::Merge(const Aabb& _aabb)
{
    const Vec3 minA = center - extents;
    const Vec3 maxA = center + extents;
    const Vec3 minB = _aabb.center - _aabb.extents;
    const Vec3 maxB = _aabb.center + _aabb.extents;

    const Vec3 newMin = minA.Min(minB);
    const Vec3 newMax = maxA.Max(maxB);

    center  = (newMin + newMax) * 0.5f;
    extents = (newMax - newMin) * 0.5f;
}

void Aabb::Merge(const Vec3 _p)
{
    const Vec3 min = center - extents;
    const Vec3 max = center + extents;

    const Vec3 newMin = min.Min(_p);
    const Vec3 newMax = max.Max(_p);

    center  = (newMin + newMax) * 0.5f;
    extents = (newMax - newMin) * 0.5f;
}

Vec3 Aabb::GetMin() const
{
    return center - extents;
}

Vec3 Aabb::GetMax() const
{
    return center + extents;
}

float Aabb::GetWidth() const
{
    return extents.x * 2.f;
}

float Aabb::GetHeight() const
{
    return extents.y * 2.f;
}

float Aabb::GetDepth() const
{
    return extents.z * 2.f;
}

Corners Aabb::CalcCorners() const
{
    const Vec3 min = center - extents;
    const Vec3 max = center + extents;

    Corners corners                   = {};
    corners[eRegion::LeftTopNear]     = { min.x, max.y, min.z };
    corners[eRegion::RightTopNear]    = { max.x, max.y, min.z };
    corners[eRegion::LeftBottomNear]  = { min.x, min.y, min.z };
    corners[eRegion::RightBottomNear] = { max.x, min.y, min.z };
    corners[eRegion::LeftTopFar]      = { min.x, max.y, max.z };
    corners[eRegion::RightTopFar]     = { max.x, max.y, max.z };
    corners[eRegion::LeftBottomFar]   = { min.x, min.y, max.z };
    corners[eRegion::RightBottomFar]  = { max.x, min.y, max.z };
    return corners;
}

bool Aabb::Contains(const Vec3 _p) const
{
    const Vec3 v = (_p - center).Abs();
    return v.x <= extents.x && v.y <= extents.y && v.z <= extents.z;
}

bool Aabb::Contains(const Aabb& _other) const
{
    const Vec3 min0 = center - extents;
    const Vec3 max0 = center + extents;
    const Vec3 min1 = _other.center - _other.extents;
    const Vec3 max2 = _other.center + _other.extents;

    return min1.x >= min0.x && max2.x <= max0.x
        && min1.y >= min0.y && max2.y <= max0.y
        && min1.z >= min0.z && max2.z <= max0.z;
}

bool Aabb::Overlap(const Aabb& _other) const
{
    const Vec3 d = (center - _other.center).Abs();
    const Vec3 r = extents + _other.extents;
    return d.x <= r.x && d.y <= r.y && d.z <= r.z;
}

eIntersect Aabb::Intersects(const Aabb& _other) const
{
    const Vec3 d    = (center - _other.center).Abs();
    const Vec3 sumE = extents + _other.extents;

    if (d.x > sumE.x || d.y > sumE.y || d.z > sumE.z)
    {
        return eIntersect::Outside;
    }

    const Vec3 contain = d + _other.extents;
    if (contain.x <= extents.x && contain.y <= extents.y && contain.z <= extents.z)
    {
        return eIntersect::Inside;
    }

    return eIntersect::Overlab;
}

bool Aabb::RayCast(const Ray& _ray, Hit* _pHitOrNull) const
{
    const Vec3 min    = center - extents;
    const Vec3 max    = center + extents;
    const Vec3 invDir = _ray.dir.Rcp();

    const Vec3  t0    = (min - _ray.pos) * invDir;
    const Vec3  t1    = (max - _ray.pos) * invDir;
    const Vec3  tmin  = t0.Min(t1);
    const Vec3  tmax  = t0.Max(t1);
    const float tNear = Max(tmin.x, tmin.y, tmin.z);
    const float tFar  = Min(tmax.x, tmax.y, tmax.z);

    if (tNear > tFar || tFar < 0.f)
    {
        return false;
    }

    if (!_pHitOrNull)
    {
        return true;
    }

    Vec3 normal = {};
    if (tmin.x >= tmin.y && tmin.x >= tmin.z)
    {
        normal.x = (t0.x < t1.x) ? -1.f : 1.f;
    }
    else if (tmin.y >= tmin.z)
    {
        normal.y = (t0.y < t1.y) ? -1.f : 1.f;
    }
    else
    {
        normal.z = (t0.z < t1.z) ? -1.f : 1.f;
    }

    _pHitOrNull->point  = _ray.pos + _ray.dir * tNear;
    _pHitOrNull->normal = normal;
    _pHitOrNull->dist   = tNear;

    return true;
}

Obb::Obb(
    const Aabb& _aabb)
    : center(_aabb.center)
    , extents(_aabb.extents)
    , rotation(Quat::kIdentity)
{
}

Obb Obb::Transform(const Mtx4& _mtx) const
{
    Obb result    = {};
    result.center = _mtx.Mult(center);

    Vec3 s, t;
    Quat r;
    _mtx.DecomposeSRT(&s, &r, &t);

    result.rotation = r * rotation;
    result.extents  = extents * s;

    return result;
}

float Obb::GetWidth() const
{
    return extents.x * 2.f;
}

float Obb::GetHeight() const
{
    return extents.y * 2.f;
}

float Obb::GetDepth() const
{
    return extents.z * 2.f;
}

Corners Obb::GetCorners() const
{
    const Vec3 axisX = rotation.Rotate(Vec3::kRight) * extents.x;
    const Vec3 axisY = rotation.Rotate(Vec3::kUp) * extents.y;
    const Vec3 axisZ = rotation.Rotate(Vec3::kForward) * extents.z;

    Corners points                   = {};
    points[eRegion::LeftBottomFar]   = center - axisX - axisY + axisZ;
    points[eRegion::LeftBottomNear]  = center - axisX - axisY - axisZ;
    points[eRegion::LeftTopFar]      = center - axisX + axisY + axisZ;
    points[eRegion::LeftTopNear]     = center - axisX + axisY - axisZ;
    points[eRegion::RightBottomFar]  = center + axisX - axisY + axisZ;
    points[eRegion::RightBottomNear] = center + axisX - axisY - axisZ;
    points[eRegion::RightTopFar]     = center + axisX + axisY + axisZ;
    points[eRegion::RightTopNear]    = center + axisX + axisY - axisZ;

    return points;
}

bool Obb::Contains(Vec3 _point) const
{
    const Quat invRot = rotation.Inverse();
    const Vec3 pointL = invRot.Rotate(_point - center).Abs();
    return pointL.x <= extents.x && pointL.y <= extents.y && pointL.z <= extents.z;
}

bool Obb::Contains(const Obb& _other) const
{
    const Corners corners = _other.GetCorners();
    for (const Vec3& corner: corners)
    {
        if (!Contains(corner))
        {
            return false;
        }
    }
    return true;
}

bool Obb::Overlab(const Obb& _other) const
{
    Quat invQ = rotation.Inverse();
    Mtx4 rMtx = Mtx4::FromQuaternion(invQ * _other.rotation);
    Vec3 t    = invQ.Rotate(_other.center - center);
    Vec3 eA   = extents;
    Vec3 eB   = _other.extents;
    Vec3 ar0  = rMtx.r0.xyz;
    Vec3 ar1  = rMtx.r1.xyz;
    Vec3 ar2  = rMtx.r2.xyz;

    float ra, rb;
    ra = eA.x;
    rb = ar0.Dot(eB);
    if (Abs(t.x) > ra + rb)
    {
        return false;
    }

    ra = eA.y;
    rb = ar1.Dot(eB);
    if (Abs(t.y) > ra + rb)
    {
        return false;
    }

    ra = eA.z;
    rb = ar2.Dot(eB);
    if (Abs(t.z) > ra + rb)
    {
        return false;
    }

    Vec3 bu = { rMtx.m[0][0], rMtx.m[1][0], rMtx.m[2][0] };
    Vec3 bv = { rMtx.m[0][1], rMtx.m[1][1], rMtx.m[2][1] };
    Vec3 bw = { rMtx.m[0][2], rMtx.m[1][2], rMtx.m[2][2] };

    Vec3 abu = bu.Abs();
    Vec3 abv = bv.Abs();
    Vec3 abw = bw.Abs();

    ra = eA.Dot(abu);
    rb = eB.x;
    if (Abs(t.Dot(bu)) > ra + rb)
    {
        return false;
    }

    ra = eA.Dot(abv);
    rb = eB.y;
    if (Abs(t.Dot(bv)) > ra + rb)
    {
        return false;
    }

    ra = eA.Dot(abw);
    rb = eB.z;
    if (Abs(t.Dot(bw)) > ra + rb)
    {
        return false;
    }

    ra = eA.y * abu.z + eA.z * abu.y;
    rb = eB.y * ar0.z + eB.z * ar0.y;
    if (Abs(t.z * rMtx.m[1][0] - t.y * rMtx.m[2][0]) > ra + rb + kEpsilon)
    {
        return false;
    }

    ra = eA.y * abv.z + eA.z * abv.y;
    rb = eB.x * ar0.z + eB.z * ar0.x;
    if (Abs(t.z * rMtx.m[1][1] - t.y * rMtx.m[2][1]) > ra + rb + kEpsilon)
    {
        return false;
    }

    ra = eA.y * abw.z + eA.z * abw.y;
    rb = eB.x * ar0.y + eB.y * ar0.x;
    if (Abs(t.z * rMtx.m[1][2] - t.y * rMtx.m[2][2]) > ra + rb + kEpsilon)
    {
        return false;
    }

    ra = eA.x * abu.z + eA.z * abu.x;
    rb = eB.y * ar1.z + eB.z * ar1.y;
    if (Abs(t.x * rMtx.m[2][0] - t.z * rMtx.m[0][0]) > ra + rb + kEpsilon)
    {
        return false;
    }

    ra = eA.x * abv.z + eA.z * abv.x;
    rb = eB.x * ar1.z + eB.z * ar1.x;
    if (Abs(t.x * rMtx.m[2][1] - t.z * rMtx.m[0][1]) > ra + rb + kEpsilon)
    {
        return false;
    }

    ra = eA.x * abw.z + eA.z * abw.x;
    rb = eB.x * ar1.y + eB.y * ar1.x;
    if (Abs(t.x * rMtx.m[2][2] - t.z * rMtx.m[0][2]) > ra + rb + kEpsilon)
    {
        return false;
    }

    ra = eA.x * abu.y + eA.y * abu.x;
    rb = eB.y * ar2.z + eB.z * ar2.y;
    if (Abs(t.y * rMtx.m[0][0] - t.x * rMtx.m[1][0]) > ra + rb + kEpsilon)
    {
        return false;
    }

    ra = eA.x * abv.y + eA.y * abv.x;
    rb = eB.x * ar2.z + eB.z * ar2.x;
    if (Abs(t.y * rMtx.m[0][1] - t.x * rMtx.m[1][1]) > ra + rb + kEpsilon)
    {
        return false;
    }

    ra = eA.x * abw.y + eA.y * abw.x;
    rb = eB.x * ar2.y + eB.y * ar2.x;
    if (Abs(t.y * rMtx.m[0][2] - t.x * rMtx.m[1][2]) > ra + rb + kEpsilon)
    {
        return false;
    }

    return true;
}

bool Obb::RayCast(const Ray& _ray, Hit* _pHitOrNull) const
{
    const Quat invQ = rotation.Inverse();

    Ray rayL = {};
    rayL.pos = invQ.Rotate(_ray.pos - center);
    rayL.dir = invQ.Rotate(_ray.dir);

    const Aabb aabbL = { Vec3::kZero, extents };
    if (aabbL.RayCast(rayL, _pHitOrNull))
    {
        if (_pHitOrNull)
        {
            _pHitOrNull->point  = rotation.Rotate(_pHitOrNull->point) + center;
            _pHitOrNull->normal = rotation.Rotate(_pHitOrNull->normal);
            _pHitOrNull->dist   = _ray.pos.Distance(_pHitOrNull->point);
        }
        return true;
    }
    return false;
}

Plane::Plane(const float _a, const float _b, const float _c, const float _d, const bool _bNormalize)
    : vec(_a, _b, _c, _d)
{
    if (_bNormalize)
    {
        Normalize();
    }
}

Plane::Plane(const Vec4 _vec, const bool _bNormalize)
    : Plane(_vec.x, _vec.y, _vec.z, _vec.w, _bNormalize)
{}

Plane::Plane(const Vec3 _normal, const float _d, const bool _bNormalize)
    : Plane(_normal.x, _normal.y, _normal.z, _d, _bNormalize)
{}

Plane::Plane(const Vec3 _p1, const Vec3 _p2, const Vec3 _p3, const bool _bNormalize)
{
    const Vec3 v0 = _p2 - _p1;
    const Vec3 v1 = _p3 - _p1;
    normal        = v0.Cross(v1);
    d             = -normal.Dot(_p1);
    if (_bNormalize)
    {
        Normalize();
    }
}

Plane::Plane(const Vec3 _point, const Vec3 _normal, const bool _bNormalize)
{
    normal = _normal;
    d      = -normal.Dot(_point);
    if (_bNormalize)
    {
        Normalize();
    }
}

Vec3 Plane::IntersectOf3Planes(const Plane& p1, const Plane& p2, const Plane& p3)
{
    const Vec3 n1 = p1.normal;
    const Vec3 n2 = p2.normal;
    const Vec3 n3 = p3.normal;

    const Vec3  cross23 = n2.Cross(n3);
    const float denom   = n1.Dot(cross23);
    if (IsZeroApprox(denom))
    {
        return Vec3::kZero;
    }

    const Vec3 cross31 = n3.Cross(n1);
    const Vec3 cross12 = n1.Cross(n2);
    return (cross23 * -p1.d + cross31 * -p2.d + cross12 * -p3.d) / denom;
}

void Plane::Normalize()
{
    const float invLen = RSqrt(normal.LengthSq());
    normal             = normal * invLen;
    d *= invLen;
}

Plane Plane::Normalized() const
{
    Plane out = *this;
    out.Normalize();
    return out;
}

Plane Plane::Transform(const Mtx4& _invTransMtx, const bool _bNormalize) const
{
    Plane p = *this;
    p.vec   = _invTransMtx.Mult(vec);
    if (_bNormalize)
    {
        p.Normalize();
    }
    return p;
}

float Plane::DistanceToPoint(const Vec3 _point) const
{
    return normal.Dot(_point) + d;
}

bool Plane::RayCast(const Ray& _ray, const bool _bDoubleSided, Hit* _pHitOrNull) const
{
    const float dist  = DistanceToPoint(_ray.pos);
    const float nDotd = normal.Dot(_ray.dir);
    if (!_bDoubleSided && (dist < 0.f || nDotd > 0.f))
    {
        return false;
    }

    if (_pHitOrNull)
    {
        const float tt      = -dist / nDotd;
        _pHitOrNull->normal = normal;
        _pHitOrNull->point  = _ray.pos + _ray.dir * tt;
        _pHitOrNull->dist   = tt;
    }

    return true;
}

bool Plane::Overlap(const Aabb& _box) const
{
    const float d = DistanceToPoint(_box.center);
    const float r = normal.Abs().Dot(_box.extents);
    return Abs(d) <= r;
}

eIntersect Plane::Intersects(const Aabb& _box) const
{
    const float dist = DistanceToPoint(_box.center);
    const float r    = normal.Abs().Dot(_box.extents);
    return dist < -r ? eIntersect::Outside
         : dist > r  ? eIntersect::Inside
                     : eIntersect::Overlab;
}

Frustum::Frustum(
    const Plane _left,
    const Plane _right,
    const Plane _bottom,
    const Plane _top,
    const Plane _near,
    const Plane _far,
    const bool  _bNormalized)
{
    planes[eFrustumFace::Near]   = _near;
    planes[eFrustumFace::Far]    = _far;
    planes[eFrustumFace::Left]   = _left;
    planes[eFrustumFace::Right]  = _right;
    planes[eFrustumFace::Bottom] = _bottom;
    planes[eFrustumFace::Top]    = _top;

    if (_bNormalized)
    {
        Normalize();
    }
}

Frustum::Frustum(const Mtx4& _mtx, const bool _bNormalized)
    : Frustum(
          Plane { _mtx.m[0][3] + _mtx.m[0][0], _mtx.m[1][3] + _mtx.m[1][0], _mtx.m[2][3] + _mtx.m[2][0], _mtx.m[3][3] + _mtx.m[3][0], false },
          Plane { _mtx.m[0][3] - _mtx.m[0][0], _mtx.m[1][3] - _mtx.m[1][0], _mtx.m[2][3] - _mtx.m[2][0], _mtx.m[3][3] - _mtx.m[3][0], false },
          Plane { _mtx.m[0][3] + _mtx.m[0][1], _mtx.m[1][3] + _mtx.m[1][1], _mtx.m[2][3] + _mtx.m[2][1], _mtx.m[3][3] + _mtx.m[3][1], false },
          Plane { _mtx.m[0][3] - _mtx.m[0][1], _mtx.m[1][3] - _mtx.m[1][1], _mtx.m[2][3] - _mtx.m[2][1], _mtx.m[3][3] - _mtx.m[3][1], false },
          Plane { _mtx.m[0][2], _mtx.m[1][2], _mtx.m[2][2], _mtx.m[3][2], false },
          Plane { _mtx.m[0][3] - _mtx.m[0][2], _mtx.m[1][3] - _mtx.m[1][2], _mtx.m[2][3] - _mtx.m[2][2], _mtx.m[3][3] - _mtx.m[3][2], false },
          _bNormalized)
{
}

Frustum Frustum::Transform(const Mtx4& _invTransMtx, const bool _bNormalize) const
{
    Frustum out = *this;
    for (Plane& p: out.planes)
    {
        p = p.Transform(_invTransMtx, _bNormalize);
    }
    return out;
}

void Frustum::Normalize()
{
    for (Plane& p: planes)
    {
        p.Normalize();
    }
}

Frustum Frustum::Normalized() const
{
    Frustum out = *this;
    out.Normalize();
    return out;
}

Vec3 Frustum::CalcCenter() const
{
    const Corners corners = CalcCorners();
    Vec3          sum     = Vec3::kZero;
    for (const Vec3 p: corners)
    {
        sum = sum + p;
    }
    return sum / static_cast<float>(corners.Size());
}

Corners Frustum::CalcCorners() const
{
    const Plane& n = planes[eFrustumFace::Near];
    const Plane& f = planes[eFrustumFace::Far];
    const Plane& l = planes[eFrustumFace::Left];
    const Plane& r = planes[eFrustumFace::Right];
    const Plane& t = planes[eFrustumFace::Top];
    const Plane& b = planes[eFrustumFace::Bottom];

    Corners corners                   = {};
    corners[eRegion::LeftTopNear]     = Plane::IntersectOf3Planes(n, l, t);
    corners[eRegion::RightTopNear]    = Plane::IntersectOf3Planes(n, r, t);
    corners[eRegion::LeftBottomNear]  = Plane::IntersectOf3Planes(n, l, b);
    corners[eRegion::RightBottomNear] = Plane::IntersectOf3Planes(n, r, b);
    corners[eRegion::LeftTopFar]      = Plane::IntersectOf3Planes(f, l, t);
    corners[eRegion::RightTopFar]     = Plane::IntersectOf3Planes(f, r, t);
    corners[eRegion::LeftBottomFar]   = Plane::IntersectOf3Planes(f, l, b);
    corners[eRegion::RightBottomFar]  = Plane::IntersectOf3Planes(f, r, b);
    return corners;
}

bool Frustum::Overlap(const Aabb& _box) const
{
    for (const Plane& p: planes)
    {
        if (!p.Overlap(_box))
        {
            return false;
        }
    }
    return true;
}

eIntersect Frustum::Intersects(const Aabb& _box) const
{
    bool bAnyIntersect = false;
    for (const Plane p: planes)
    {
        const eIntersect ret = p.Intersects(_box);
        if (ret == eIntersect::Outside)
        {
            return eIntersect::Outside;
        }

        if (ret == eIntersect::Overlab)
        {
            bAnyIntersect = true;
        }
    }
    return bAnyIntersect ? eIntersect::Overlab : eIntersect::Inside;
}

bool Frustum::RayCast(const Ray& _ray, const bool _bDoubleSided, Hit* _pHitOrNull) const
{
    for (const Plane& p: planes)
    {
        if (p.RayCast(_ray, _bDoubleSided, _pHitOrNull))
        {
            return true;
        }
    }
    return false;
}

Ray::Ray(const Vec3 _origin, const Vec3 _direction, const bool _bNormalizeDir)
    : pos(_origin)
    , dir(_direction)
{
    if (_bNormalizeDir) Normalize();
}

void Ray::Normalize()
{
    dir.Normalize();
}

Ray Ray::Normalized() const
{
    Ray out = *this;
    out.Normalize();
    return out;
}

}   // namespace Jam