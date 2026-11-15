#include "pch.h"
#include "BoundingVolume.h"

namespace Jam
{

eRegion CalcRegion(
    const bool _bRight,
    const bool _bTop,
    const bool _bFar)
{
    return static_cast<eRegion>((static_cast<int>(_bRight) << 2)
                                | (static_cast<int>(_bTop) << 1)
                                | static_cast<int>(_bFar));
}

bool IsRight(
    const eRegion _region)
{
    return (static_cast<int>(_region) & 0b100) != 0;
}

bool IsTop(
    const eRegion _region)
{
    return (static_cast<int>(_region) & 0b010) != 0;
}

bool IsFar(
    const eRegion _region)
{
    return (static_cast<int>(_region) & 0b001) != 0;
}

Ray::Ray(Vec3 _dir, Vec3 _origin)
    : dir(_dir)
    , pos(_origin)
{
    JAM_ASSERT(dir.IsNormalized(), "Ray direction must be normalized.");
}

Ray::Ray(
    const float _ndcX,
    const float _ndcY,
    const Mtx4& _invMtx)
{
    Vec3 n = { _ndcX, _ndcY, 0.f };
    n      = _invMtx.MultPoint(n);

    Vec3 f = { _ndcX, _ndcY, 1.f };
    f      = _invMtx.MultPoint(f);

    dir = (f - n).Normalized();
    pos = n;
}

Ray::Ray(
    const Int32 _screenX,
    const Int32 _screenY,
    const Int32 _screenWidth,
    const Int32 _screenHeight,
    const Mtx4& _invMtx)
{
    const float fscreenX = static_cast<float>(_screenX) + 0.5f;   // Center of the pixel
    const float fscreenY = static_cast<float>(_screenY) + 0.5f;   // Center of the pixel
    const float ndcX     = (fscreenX / static_cast<float>(_screenWidth)) * 2.f - 1.f;
    const float ndcY     = 1.f - (fscreenY / static_cast<float>(_screenHeight)) * 2.f;

    Vec3 n = { ndcX, ndcY, 0.f };
    n      = _invMtx.MultPoint(n);

    Vec3 f = { ndcX, ndcY, 1.f };
    f      = _invMtx.MultPoint(f);

    dir = (f - n).Normalized();
    pos = n;
}

Aabb::Aabb(
    const Vec3 _center,
    const Vec3 _extents)
    : center(_center)
    , extents(_extents)
{
}

Aabb Aabb::FromMinMax(
    const Vec3 _min,
    const Vec3 _max)
{
    const Vec3 c = (_min + _max) * 0.5f;
    const Vec3 e = (_max - _min) * 0.5f;
    return Aabb { c, e };
}

Aabb Aabb::Transform(
    const Mtx4& _mtx) const
{
    const Vec3 nc = _mtx.MultPoint(center);
    const Vec3 ex = _mtx.r0.xyz.Abs() * extents;
    const Vec3 ey = _mtx.r1.xyz.Abs() * extents;
    const Vec3 ez = _mtx.r2.xyz.Abs() * extents;
    const Vec3 ne = ex + ey + ez;
    return { nc, ne };
}

void Aabb::Merge(
    const Aabb& _aabb)
{
    const Vec3 mina = center - extents;
    const Vec3 maxa = center + extents;
    const Vec3 minb = _aabb.center - _aabb.extents;
    const Vec3 maxb = _aabb.center + _aabb.extents;
    const Vec3 nmin = mina.Min(minb);
    const Vec3 nmax = maxa.Max(maxb);

    center  = (nmin + nmax) * 0.5f;
    extents = (nmax - nmin) * 0.5f;
}

void Aabb::Merge(
    const Vec3 _p)
{
    const Vec3 min  = center - extents;
    const Vec3 max  = center + extents;
    const Vec3 nmin = min.Min(_p);
    const Vec3 nmax = max.Max(_p);

    center  = (nmin + nmax) * 0.5f;
    extents = (nmax - nmin) * 0.5f;
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

bool Aabb::Contains(
    const Vec3 _p) const
{
    const Vec3 v = (_p - center).Abs();
    return v.x <= extents.x && v.y <= extents.y && v.z <= extents.z;
}

bool Aabb::Contains(
    const Aabb& _other) const
{
    const Vec3 min0 = center - extents;
    const Vec3 max0 = center + extents;
    const Vec3 min1 = _other.center - _other.extents;
    const Vec3 max2 = _other.center + _other.extents;

    return min1.x >= min0.x && max2.x <= max0.x
        && min1.y >= min0.y && max2.y <= max0.y
        && min1.z >= min0.z && max2.z <= max0.z;
}

bool Aabb::Overlap(
    const Aabb& _other) const
{
    const Vec3 d = (center - _other.center).Abs();
    const Vec3 r = extents + _other.extents;
    return d.x <= r.x && d.y <= r.y && d.z <= r.z;
}

Optional<Hit> Aabb::RayCast(
    const Ray& _ray) const
{
    JAM_ASSERT(_ray.dir.IsNormalized(), "RayCast requires normalized ray direction.");

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
        return kNullopt;
    }

    Vec3 normal = Vec3::kZero;
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

    const Vec3 point = _ray.pos + _ray.dir * tNear;
    return Hit { normal, point, tNear };
}

Obb::Obb(
    const Vec3 _center,
    const Vec3 _extents,
    const Quat _rotation)
    : center(_center)
    , extents(_extents)
    , rotation(_rotation)
{
    JAM_ASSERT(rotation.IsNormalized(), "Obb rotation must be normalized.");
}

Obb::Obb(
    const Aabb& _aabb)
    : center(_aabb.center)
    , extents(_aabb.extents)
    , rotation(Quat::kIdentity)
{
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

Obb Obb::Transform(
    const Mtx4& _mtx) const
{
    Vec3 s, t;
    Quat r;
    _mtx.DecomposeSRT(&s, &r, &t);

    const Vec3 c = _mtx.MultPoint(center);
    const Vec3 e = extents * s;
    const Quat q = r * rotation;
    return { c, e, q };
}

bool Obb::Contains(
    const Vec3 _point) const
{
    const Quat invRot = rotation.Inverse();
    const Vec3 pointL = invRot.Rotate(_point - center).Abs();
    return pointL.x <= extents.x && pointL.y <= extents.y && pointL.z <= extents.z;
}

bool Obb::Contains(
    const Obb& _other) const
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

bool Obb::Overlap(
    const Obb& _other) const
{
    const Quat invQ = rotation.Inverse();
    const Mtx4 rMtx = Mtx4::FromQuaternion(invQ * _other.rotation);
    const Vec3 t    = invQ.Rotate(_other.center - center);
    const Vec3 eA   = extents;
    const Vec3 eB   = _other.extents;

    const Vec3 ar0 = rMtx.r0.xyz;
    const Vec3 ar1 = rMtx.r1.xyz;
    const Vec3 ar2 = rMtx.r2.xyz;

    const Vec3 abu = { Abs(rMtx.m00), Abs(rMtx.m10), Abs(rMtx.m20) };
    const Vec3 abv = { Abs(rMtx.m01), Abs(rMtx.m11), Abs(rMtx.m21) };
    const Vec3 abw = { Abs(rMtx.m02), Abs(rMtx.m12), Abs(rMtx.m22) };

    const Vec3 aar0 = ar0.Abs();
    const Vec3 aar1 = ar1.Abs();
    const Vec3 aar2 = ar2.Abs();

    float ra, rb;

    ra = eA.x;
    rb = abu.Dot(eB);
    if (Abs(t.x) > ra + rb)
    {
        return false;
    }

    ra = eA.y;
    rb = abv.Dot(eB);
    if (Abs(t.y) > ra + rb)
    {
        return false;
    }

    ra = eA.z;
    rb = abw.Dot(eB);
    if (Abs(t.z) > ra + rb)
    {
        return false;
    }

    ra = eA.Dot(aar0);
    rb = eB.x;
    if (Abs(t.Dot(ar0)) > ra + rb)
    {
        return false;
    }

    ra = eA.Dot(aar1);
    rb = eB.y;
    if (Abs(t.Dot(ar1)) > ra + rb)
    {
        return false;
    }

    ra = eA.Dot(aar2);
    rb = eB.z;
    if (Abs(t.Dot(ar2)) > ra + rb)
    {
        return false;
    }

    ra = eA.y * aar0.z + eA.z * aar0.y;
    rb = eB.y * abu.z + eB.z * abu.y;
    if (Abs(t.z * ar0.y - t.y * ar0.z) > ra + rb + kEpsilon)
    {
        return false;
    }

    ra = eA.y * aar1.z + eA.z * aar1.y;
    rb = eB.x * abu.z + eB.z * abu.x;
    if (Abs(t.z * ar1.y - t.y * ar1.z) > ra + rb + kEpsilon)
    {
        return false;
    }

    ra = eA.y * aar2.z + eA.z * aar2.y;
    rb = eB.x * abu.y + eB.y * abu.x;
    if (Abs(t.z * ar2.y - t.y * ar2.z) > ra + rb + kEpsilon)
    {
        return false;
    }

    ra = eA.x * aar0.z + eA.z * aar0.x;
    rb = eB.y * abv.z + eB.z * abv.y;
    if (Abs(t.x * ar0.z - t.z * ar0.x) > ra + rb + kEpsilon)
    {
        return false;
    }

    ra = eA.x * aar1.z + eA.z * aar1.x;
    rb = eB.x * abv.z + eB.z * abv.x;
    if (Abs(t.x * ar1.z - t.z * ar1.x) > ra + rb + kEpsilon)
    {
        return false;
    }

    ra = eA.x * aar2.z + eA.z * aar2.x;
    rb = eB.x * abv.y + eB.y * abv.x;
    if (Abs(t.x * ar2.z - t.z * ar2.x) > ra + rb + kEpsilon)
    {
        return false;
    }

    ra = eA.x * aar0.y + eA.y * aar0.x;
    rb = eB.y * abw.z + eB.z * abw.y;
    if (Abs(t.y * ar0.x - t.x * ar0.y) > ra + rb + kEpsilon)
    {
        return false;
    }

    ra = eA.x * aar1.y + eA.y * aar1.x;
    rb = eB.x * abw.z + eB.z * abw.x;
    if (Abs(t.y * ar1.x - t.x * ar1.y) > ra + rb + kEpsilon)
    {
        return false;
    }

    ra = eA.x * aar2.y + eA.y * aar2.x;
    rb = eB.x * abw.y + eB.y * abw.x;
    if (Abs(t.y * ar2.x - t.x * ar2.y) > ra + rb + kEpsilon)
    {
        return false;
    }

    return true;
}

Optional<Hit> Obb::RayCast(const Ray& _ray) const
{
    JAM_ASSERT(_ray.dir.IsNormalized(), "RayCast requires normalized ray direction.");

    const Quat invQ = rotation.Inverse();
    const Vec3 pos  = invQ.Rotate(_ray.pos - center);
    const Vec3 dir  = invQ.Rotate(_ray.dir);
    const Ray  rayL = { pos, dir };

    const Aabb    aabbL  = { Vec3::kZero, extents };
    Optional<Hit> result = aabbL.RayCast(rayL);
    if (result)
    {
        Hit& hit   = result.GetValue();
        hit.point  = rotation.Rotate(hit.point) + center;
        hit.normal = rotation.Rotate(hit.normal);
        hit.dist   = _ray.pos.Distance(hit.point);
        return result;
    }
    return kNullopt;
}

Plane::Plane(
    const float _a,
    const float _b,
    const float _c,
    const float _d)
    : vec(_a, _b, _c, _d)
{
}

Plane::Plane(
    const Vec4 _vec)
    : vec(_vec)
{}

Plane::Plane(
    const Vec3  _normal,
    const float _d)
    : vec(_normal.x, _normal.y, _normal.z, _d)
{
}

Plane::Plane(
    const Vec3 _p1,
    const Vec3 _p2,
    const Vec3 _p3)
{
    const Vec3 v0 = _p2 - _p1;
    const Vec3 v1 = _p3 - _p1;
    normal        = v0.Cross(v1);
    d             = -normal.Dot(_p1);
}

Plane::Plane(
    const Vec3 _point,
    const Vec3 _normal)
    : normal(_normal)
    , d(-normal.Dot(_point))
{
}

Vec3 Plane::IntersectOf3Planes(
    const Plane _p0,
    const Plane _p1,
    const Plane _p2)
{
    const Vec3 n1 = _p0.normal;
    const Vec3 n2 = _p1.normal;
    const Vec3 n3 = _p2.normal;

    const Vec3  cross23 = n2.Cross(n3);
    const float denom   = n1.Dot(cross23);
    if (IsZeroApprox(denom))
    {
        return Vec3::kZero;
    }

    const Vec3 cross31 = n3.Cross(n1);
    const Vec3 cross12 = n1.Cross(n2);
    return (cross23 * -_p0.d + cross31 * -_p1.d + cross12 * -_p2.d) / denom;
}

Plane Plane::Transform(
    const Mtx4& _invTransMtx) const
{
    Plane p = *this;
    p.vec   = _invTransMtx.Mult(vec);
    return p;
}

void Plane::Normalize()
{
    const float lenSq = normal.LengthSq();
    if (lenSq == 0.f)
    {
        normal = Vec3::kZero;
        d      = 0.f;
        return;
    }

    const float invLen = RSqrt(lenSq);
    normal             = normal * invLen;
    d *= invLen;
}

Plane Plane::Normalized() const
{
    Plane out = *this;
    out.Normalize();
    return out;
}

bool Plane::IsNormalized() const
{
    return normal.IsNormalized();
}

float Plane::DistanceTo(
    const Vec3 _point) const
{
    JAM_ASSERT(IsNormalized(), "Plane must be normalized to calculate distance.");
    return normal.Dot(_point) + d;
}

bool Plane::IsFront(
    const Vec3 _point) const
{
    return DistanceTo(_point) > 0.f;
}

bool Plane::IsBehind(
    const Vec3 _point) const
{
    return DistanceTo(_point) < 0.f;
}

bool Plane::Overlap(
    const Aabb& _aabb) const
{
    const float d = DistanceTo(_aabb.center);          // aabb의 중심과 평면 사이의 거리
    const float r = normal.Abs().Dot(_aabb.extents);   // aabb가 평면에 걸치는 최대 거리
    return Abs(d) <= r;
}

bool Plane::IsFront(
    const Aabb& _aabb) const
{
    const float d = DistanceTo(_aabb.center);
    const float r = normal.Abs().Dot(_aabb.extents);
    return d > r;
}

bool Plane::IsBehind(
    const Aabb& _aabb) const
{
    const float d = DistanceTo(_aabb.center);
    const float r = normal.Abs().Dot(_aabb.extents);
    return d < -r;
}

Optional<Hit> Plane::RayCast(
    const Ray& _ray,
    const bool _bDoubleSided) const
{
    JAM_ASSERT(_ray.dir.IsNormalized(), "Ray direction must be normalized to perform raycast against plane.");

    const float dist  = DistanceTo(_ray.pos);
    const float nDotd = normal.Dot(_ray.dir);
    if (!_bDoubleSided && (dist < 0.f || nDotd > 0.f))
    {
        return kNullopt;
    }

    if (IsZeroApprox(nDotd))
    {
        return kNullopt;
    }

    const float tt    = -dist / nDotd;
    const Vec3  point = _ray.pos + _ray.dir * tt;
    return Hit { normal, point, tt };
}

Frustum::Frustum(
    const Plane _left,
    const Plane _right,
    const Plane _bottom,
    const Plane _top,
    const Plane _near,
    const Plane _far)
    : leftp(_left)
    , rightp(_right)
    , bottomp(_bottom)
    , topp(_top)
    , nearp(_near)
    , farp(_far)
{
}

Frustum::Frustum(
    const Mtx4& _mtx)
    : leftp(_mtx.m03 + _mtx.m00, _mtx.m13 + _mtx.m10, _mtx.m23 + _mtx.m20, _mtx.m33 + _mtx.m30)
    , rightp(_mtx.m03 - _mtx.m00, _mtx.m13 - _mtx.m10, _mtx.m23 - _mtx.m20, _mtx.m33 - _mtx.m30)
    , bottomp(_mtx.m03 + _mtx.m01, _mtx.m13 + _mtx.m11, _mtx.m23 + _mtx.m21, _mtx.m33 + _mtx.m31)
    , topp(_mtx.m03 - _mtx.m01, _mtx.m13 - _mtx.m11, _mtx.m23 - _mtx.m21, _mtx.m33 - _mtx.m31)
    , nearp(_mtx.m02, _mtx.m12, _mtx.m22, _mtx.m32)
    , farp(_mtx.m03 - _mtx.m02, _mtx.m13 - _mtx.m12, _mtx.m23 - _mtx.m22, _mtx.m33 - _mtx.m32)
{
}

Frustum Frustum::Transform(
    const Mtx4& _invTransMtx) const
{
    Frustum out = *this;
    for (Plane& p: out.planes)
    {
        p = p.Transform(_invTransMtx);
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

Corners Frustum::CalcCorners() const
{
    Corners corners                   = {};
    corners[eRegion::LeftTopNear]     = Plane::IntersectOf3Planes(nearp, leftp, topp);
    corners[eRegion::RightTopNear]    = Plane::IntersectOf3Planes(nearp, rightp, topp);
    corners[eRegion::LeftBottomNear]  = Plane::IntersectOf3Planes(nearp, leftp, bottomp);
    corners[eRegion::RightBottomNear] = Plane::IntersectOf3Planes(nearp, rightp, bottomp);
    corners[eRegion::LeftTopFar]      = Plane::IntersectOf3Planes(farp, leftp, topp);
    corners[eRegion::RightTopFar]     = Plane::IntersectOf3Planes(farp, rightp, topp);
    corners[eRegion::LeftBottomFar]   = Plane::IntersectOf3Planes(farp, leftp, bottomp);
    corners[eRegion::RightBottomFar]  = Plane::IntersectOf3Planes(farp, rightp, bottomp);
    return corners;
}

Corners Frustum::CalcCorners(
    const Mtx4& _invMtx)
{
    Corners corners                   = {};
    corners[eRegion::LeftTopNear]     = _invMtx.MultPoint(Vec3 { -1.f, 1.f, 0.f });
    corners[eRegion::RightTopNear]    = _invMtx.MultPoint(Vec3 { 1.f, 1.f, 0.f });
    corners[eRegion::LeftBottomNear]  = _invMtx.MultPoint(Vec3 { -1.f, -1.f, 0.f });
    corners[eRegion::RightBottomNear] = _invMtx.MultPoint(Vec3 { 1.f, -1.f, 0.f });
    corners[eRegion::LeftTopFar]      = _invMtx.MultPoint(Vec3 { -1.f, 1.f, 1.f });
    corners[eRegion::RightTopFar]     = _invMtx.MultPoint(Vec3 { 1.f, 1.f, 1.f });
    corners[eRegion::LeftBottomFar]   = _invMtx.MultPoint(Vec3 { -1.f, -1.f, 1.f });
    corners[eRegion::RightBottomFar]  = _invMtx.MultPoint(Vec3 { 1.f, -1.f, 1.f });
    return corners;
}

Corners Frustum::CalcCornersInViewSpace(
    const float _fovRad,
    const float _aspectRatio,
    const float _nearZ,
    const float _farZ)
{
    // z = 1
    const float tanHFov = Tan(_fovRad * 0.5f);
    const float h       = tanHFov;
    const float w       = h * _aspectRatio;

    // scala factor
    const float nw = w * _nearZ;
    const float nh = h * _nearZ;
    const float fw = w * _farZ;
    const float fh = h * _farZ;

    Corners corners                   = {};
    corners[eRegion::LeftTopNear]     = { -nw, nh, _nearZ };
    corners[eRegion::RightTopNear]    = { nw, nh, _nearZ };
    corners[eRegion::LeftBottomNear]  = { -nw, -nh, _nearZ };
    corners[eRegion::RightBottomNear] = { nw, -nh, _nearZ };
    corners[eRegion::LeftTopFar]      = { -fw, fh, _farZ };
    corners[eRegion::RightTopFar]     = { fw, fh, _farZ };
    corners[eRegion::LeftBottomFar]   = { -fw, -fh, _farZ };
    corners[eRegion::RightBottomFar]  = { fw, -fh, _farZ };
    return corners;
}

bool Frustum::Overlap(
    const Aabb& _box) const
{
    for (const Plane p: planes)
    {
        if (p.IsBehind(_box))
        {
            return false;
        }
    }
    return true;
}

eContainment Frustum::Classify(
    const Aabb& _box) const
{
    bool bIntersects = false;
    for (const Plane& p: planes)
    {
        const float d = p.x * _box.center.x + p.y * _box.center.y + p.z * _box.center.z + p.w;
        const float r = Abs(p.x) * _box.extents.x + Abs(p.y) * _box.extents.y + Abs(p.z) * _box.extents.z;
        if (d < -r)
        {
            return eContainment::Outside;
        }
        bIntersects |= d < r;
    }
    return bIntersects ? eContainment::Intersects : eContainment::Inside;
}

void Frustum::CalcCascadeSplitsInViewSpace(
    const Span<CascadeSplit> _outDs,
    const float              _weight) const
{
    // practical split scheme
    // https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-10-parallel-split-shadow-maps-programmable-gpus

    JAM_ASSERT(_weight >= 0.f && _weight <= 1.f, "weight must be in [0, 1]");
    JAM_ASSERT(nearp.IsNormalized() && farp.IsNormalized(), "near and far planes must be normalized to calculate cascade splits.");

    const Int32 numFrustums = static_cast<Int32>(_outDs.size());
    const float l           = _weight;
    const float n           = -nearp.d;   // near plane의 d는 음수이므로 양수로 변환
    const float f           = farp.d;
    const float ratio       = f / n;
    const float range       = f - n;
    const float invNumF     = 1.f / static_cast<float>(numFrustums);

    float nn = n;
    for (Int32 i = 0; i < numFrustums; ++i)
    {
        const float pctFar = static_cast<float>(i + 1) * invNumF;
        const float logf   = n * Pow(ratio, pctFar);
        const float unif   = n + range * pctFar;
        const float ff     = l * logf + (1.f - l) * unif;

        _outDs[i].nearZ = nn;
        _outDs[i].farZ  = (i < numFrustums - 1) ? ff * 1.005f : f;

        nn = ff;
    }
}

}   // namespace Jam