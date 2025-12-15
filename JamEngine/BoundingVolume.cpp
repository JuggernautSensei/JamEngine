#include "pch.h"

#include "BoundingVolume.h"

namespace jam
{

Aabb Aabb::FromCenterExtents(const Vec3 _center, const Vec3 _extents)
{
    Aabb out;
    out.min = _center - _extents;
    out.max = _center + _extents;
    return out;
}

Aabb Aabb::Transformed(const Mat4& _mtx) const
{
    Aabb aabb = *this;
    aabb.Transform(_mtx);
    return aabb;
}

void Aabb::Transform(const Mat4& _mtx)
{
    const Vec3 center = GetCenter();
    const Vec3 ext    = GetExtents();

    // transform center
    const Vec3 newCenter = Vec3::Transform(center, _mtx);

    // transform extents
    const Vec3 r0      = Abs({ _mtx._11, _mtx._12, _mtx._13 });
    const Vec3 r1      = Abs({ _mtx._21, _mtx._22, _mtx._23 });
    const Vec3 r2      = Abs({ _mtx._31, _mtx._32, _mtx._33 });
    const Vec3 nextExt = { Dot(r0, ext), Dot(r1, ext), Dot(r2, ext) };

    min = newCenter - nextExt;
    max = newCenter + nextExt;
}

void Aabb::Merge(const Aabb& _aabb)
{
    min = Vec3 { Min(min.x, _aabb.min.x), Min(min.y, _aabb.min.y), Min(min.z, _aabb.min.z) };
    max = Vec3 { Max(max.x, _aabb.max.x), Max(max.y, _aabb.max.y), Max(max.z, _aabb.max.z) };
}

void Aabb::Merge(const Vec3 _p)
{
    min = Vec3 { Min(min.x, _p.x), Min(min.y, _p.y), Min(min.z, _p.z) };
    max = Vec3 { Max(max.x, _p.x), Max(max.y, _p.y), Max(max.z, _p.z) };
}

Vec3 Aabb::GetCenter() const
{
    return (min + max) * 0.5f;
}

Vec3 Aabb::GetExtents() const
{
    return (max - min) * 0.5f;
}

float Aabb::GetWidth() const
{
    return max.x - min.x;
}

float Aabb::GetHeight() const
{
    return max.y - min.y;
}

float Aabb::GetDepth() const
{
    return max.z - min.z;
}

CornerPoints Aabb::GetCornerPoints() const
{
    CornerPoints corners;
    corners.nearLeftTop     = Vec3(min.x, max.y, min.z);
    corners.nearRightTop    = Vec3(max.x, max.y, min.z);
    corners.nearLeftBottom  = Vec3(min.x, min.y, min.z);
    corners.nearRightBottom = Vec3(max.x, min.y, min.z);
    corners.farLeftTop      = Vec3(min.x, max.y, max.z);
    corners.farRightTop     = Vec3(max.x, max.y, max.z);
    corners.farLeftBottom   = Vec3(min.x, min.y, max.z);
    corners.farRightBottom  = Vec3(max.x, min.y, max.z);
    return corners;
}

bool Aabb::Contain(const Vec3 _p) const
{
    return _p.x >= min.x && _p.x <= max.x
        && _p.y >= min.y && _p.y <= max.y
        && _p.z >= min.z && _p.z <= max.z;
}

bool Aabb::Contain(const Aabb& _other) const
{
    return _other.min.x >= min.x && _other.max.x <= max.x
        && _other.min.y >= min.y && _other.max.y <= max.y
        && _other.min.z >= min.z && _other.max.z <= max.z;
}

bool Aabb::Intersect(const Aabb& _other) const
{
    if (max.x < _other.min.x || min.x > _other.max.x)
    {
        return false;
    }

    if (max.y < _other.min.y || min.y > _other.max.y)
    {
        return false;
    }

    if (max.z < _other.min.z || min.z > _other.max.z)
    {
        return false;
    }

    return true;
}

RayCastResult Aabb::Intersect(const Ray& _ray) const
{
    const Vec3 invDir = Vec3(
        IsZeroApprox(_ray.direction.x) ? k_floatMax : 1.f / _ray.direction.x,
        IsZeroApprox(_ray.direction.y) ? k_floatMax : 1.f / _ray.direction.y,
        IsZeroApprox(_ray.direction.z) ? k_floatMax : 1.f / _ray.direction.z);

    const Vec3 t0 = (min - _ray.origin) * invDir;
    const Vec3 t1 = (max - _ray.origin) * invDir;

    const Vec3 tMin = Vec3(Min(t0.x, t1.x), Min(t0.y, t1.y), Min(t0.z, t1.z));
    const Vec3 tMax = Vec3(Max(t0.x, t1.x), Max(t0.y, t1.y), Max(t0.z, t1.z));

    const float tNear = Max(Max(tMin.x, tMin.y), tMin.z);
    const float tFar  = Min(Min(tMax.x, tMax.y), tMax.z);
    if (tNear > tFar || tFar < 0.f)
    {
        return {};
    }

    RayCastResult result;
    result.bHit     = true;
    result.distance = tNear >= 0.f ? tNear : tFar;
    return result;
}

void Obb::Transform(const Mat4& _mtx)
{
    mtx *= _mtx;
}

Aabb Obb::ToAabb() const
{
    Aabb aabb;
    aabb.min = Vec3(-1.f, -1.f, -1.f);
    aabb.max = Vec3(1.f, 1.f, 1.f);
    aabb.Transform(mtx);
    return aabb;
}

Obb Obb::FromAabb(const Aabb& _aabb)
{
    const Vec3 center  = _aabb.GetCenter();
    const Vec3 extents = _aabb.GetExtents();

    Obb obb;
    obb.mtx = Mat4::CreateScale(extents * 2.f) * Mat4::CreateTranslation(center);
    return obb;
}

Obb Obb::Transformed(const Mat4& _mtx) const
{
    Obb obb = *this;
    obb.Transform(_mtx);
    return obb;
}

Vec3 Obb::GetCenter() const
{
    return mtx.Translation();
}

Vec3 Obb::GetExtents() const
{
    const Vec3  r   = mtx.Right();
    const Vec3  u   = mtx.Up();
    const Vec3  f   = mtx.Forward();
    const float hsx = Length(r) * 0.5f;
    const float hsy = Length(u) * 0.5f;
    const float hsz = Length(f) * 0.5f;
    return { hsx, hsy, hsz };
}

float Obb::GetWidth() const
{
    const Vec3  r  = mtx.Right();
    const float sx = Length(r);
    return sx;
}

float Obb::GetHeight() const
{
    const Vec3  u  = mtx.Up();
    const float sy = Length(u);
    return sy;
}

float Obb::GetDepth() const
{
    const Vec3  f  = mtx.Forward();
    const float sz = Length(f);
    return sz;
}

CornerPoints Obb::GetCornerPoints() const
{
    Aabb aabb = Aabb::FromCenterExtents(Vec3::Zero, Vec3(0.5f));

    CornerPoints corners = aabb.GetCornerPoints();
    for (size_t i = 0; i < 8; ++i)
    {
        corners.points[i] = Vec3::Transform(corners.points[i], mtx);
    }
    return corners;
}

bool Obb::Contain(const Vec3 _point) const
{
    // world point to local point
    const Mat4 invMtx     = mtx.Invert();
    const Vec3 localPoint = Vec3::Transform(_point, invMtx);

    // test local point vs tmp unit local aabb
    Aabb localBox;
    localBox.min = Vec3(-0.5f, -0.5f, -0.5f);
    localBox.max = Vec3(0.5f, 0.5f, 0.5f);

    return localBox.Contain(localPoint);
}

RayCastResult Obb::Intersect(const Ray& _ray) const
{
    // world ray to local ray
    const Mat4 invMtx         = mtx.Invert();
    const Vec3 localOrigin    = Vec3::Transform(_ray.origin, invMtx);
    const Vec3 localDirection = Vec3::TransformNormal(_ray.direction, invMtx);

    // tmp unit local aabb
    Aabb localBox;
    localBox.min = Vec3(-0.5f, -0.5f, -0.5f);
    localBox.max = Vec3(0.5f, 0.5f, 0.5f);

    // test local ray vs local aabb
    Ray localRay;
    localRay.origin    = localOrigin;
    localRay.direction = localDirection;

    const RayCastResult localResult = localBox.Intersect(localRay);
    if (localResult.bHit)
    {
        return localResult;
    }
    else
    {
        return {};
    }
}

Plane Plane::FromNormalD(const Vec3 _normal, const float _d)
{
    Plane plane;
    plane.vec.x = _normal.x;
    plane.vec.y = _normal.y;
    plane.vec.z = _normal.z;
    plane.vec.w = _d;
    plane.Normalize();
    return plane;
}

Plane Plane::FromPoints(const Vec3 _p1, const Vec3 _p2, const Vec3 _p3)
{
    using namespace DirectX;
    Plane plane;
    plane.vec = XMPlaneFromPoints(_p1, _p2, _p3);
    plane.Normalize();
    return plane;
}

Plane Plane::FromPointNormal(const Vec3 _point, const Vec3 _normal)
{
    using namespace DirectX;
    Plane plane;
    plane.vec = XMPlaneFromPointNormal(_point, _normal);
    plane.Normalize();
    return plane;
}

Vec3 Plane::GetNormal() const
{
    return { vec.x, vec.y, vec.z };
}

float Plane::GetD() const
{
    return vec.w;
}

Plane Plane::Normalized() const
{
    Plane out = *this;
    out.Normalize();
    return out;
}

Plane Plane::Transformed(const Mat4& _mtx) const
{
    Plane out = *this;
    out.Transform(_mtx);
    return out;
}

void Plane::Transform(const Mat4& _mtx)
{
    vec = DirectX::XMPlaneTransform(vec, _mtx);
    Normalize();
}

Vec3 Plane::IntersectOf3Planes(const Plane& p1, const Plane& p2, const Plane& p3)
{
    using namespace DirectX;

    const Vec3 n1 = p1.GetNormal();
    const Vec3 n2 = p2.GetNormal();
    const Vec3 n3 = p3.GetNormal();

    const float d1 = p1.GetD();
    const float d2 = p2.GetD();
    const float d3 = p3.GetD();

    const float crossDot = Dot(n1, Cross(n2, n3));
    if (IsZeroApprox(crossDot))
    {
        return Vec3::Zero;
    }

    const Vec3 term1 = Cross(n2, n3) * -d1;
    const Vec3 term2 = Cross(n3, n1) * -d2;
    const Vec3 term3 = Cross(n1, n2) * -d3;
    const Vec3 sum   = term1 + term2 + term3;

    return sum / crossDot;
}

void Plane::Normalize()
{
    using namespace DirectX;
    vec = XMPlaneNormalize(vec);
}

float Plane::DistanceToPoint(const Vec3 _point) const
{
    using namespace DirectX;
    return DirectX::XMVectorGetX(XMPlaneDotCoord(vec, _point));
}

RayCastResult Plane::Intersect(const Ray& _ray) const
{
    const Vec3  n     = GetNormal();
    const float d     = GetD();
    const float denom = Dot(n, _ray.direction);
    if (IsZeroApprox(denom))
    {
        return {};
    }

    const float t = -(Dot(n, _ray.origin) + d) / denom;
    if (t < 0.f)
    {
        return {};
    }

    RayCastResult result;
    result.bHit     = true;
    result.distance = t;
    return result;
}

Frustum Frustum::FromMatrix(const Mat4& _mtx)
{
    Frustum out;

    out.leftPlane.vec = Vec4(_mtx._14 + _mtx._11, _mtx._24 + _mtx._21, _mtx._34 + _mtx._31, _mtx._44 + _mtx._41);
    out.leftPlane.Normalize();

    out.rightPlane.vec = Vec4(_mtx._14 - _mtx._11, _mtx._24 - _mtx._21, _mtx._34 - _mtx._31, _mtx._44 - _mtx._41);
    out.rightPlane.Normalize();

    out.bottomPlane.vec = Vec4(_mtx._14 + _mtx._12, _mtx._24 + _mtx._22, _mtx._34 + _mtx._32, _mtx._44 + _mtx._42);
    out.bottomPlane.Normalize();

    out.topPlane.vec = Vec4(_mtx._14 - _mtx._12, _mtx._24 - _mtx._22, _mtx._34 - _mtx._32, _mtx._44 - _mtx._42);
    out.topPlane.Normalize();

    out.nearPlane.vec = Vec4(_mtx._13, _mtx._23, _mtx._33, _mtx._43);
    out.nearPlane.Normalize();

    out.farPlane.vec = Vec4(_mtx._14 - _mtx._13, _mtx._24 - _mtx._23, _mtx._34 - _mtx._33, _mtx._44 - _mtx._43);
    out.farPlane.Normalize();

    return out;
}

Frustum Frustum::SplitFrustum(const float _startDist, const float _endDist) const
{
    const float n     = -nearPlane.GetD();
    const float f     = -farPlane.GetD();
    const float dist0 = Max(n, _startDist);
    const float dist1 = Min(f, _endDist);

    Frustum out         = *this;
    out.nearPlane.vec.w = -dist0;
    out.farPlane.vec.w  = -dist1;
    return out;
}

void Frustum::Transform(const Mat4& _mtx)
{
    for (Plane& p: planes)
    {
        p.Transform(_mtx);
    }
}

Frustum Frustum::Transformed(const Mat4& _mtx) const
{
    Frustum out = *this;
    out.Transform(_mtx);
    return out;
}

ePlaneIntersectType Frustum::Intersect(const Aabb& _box) const
{
    for (const Plane& p: planes)
    {
        Vec3 n = p.GetNormal();

        const Vec3 vmin(
            n.x >= 0 ? _box.min.x : _box.max.x,
            n.y >= 0 ? _box.min.y : _box.max.y,
            n.z >= 0 ? _box.min.z : _box.max.z);

        const Vec3 vmax(
            n.x >= 0 ? _box.max.x : _box.min.x,
            n.y >= 0 ? _box.max.y : _box.min.y,
            n.z >= 0 ? _box.max.z : _box.min.z);

        if (p.DistanceToPoint(vmin) > 0.0f)
        {
            return ePlaneIntersectType::Outside;
        }

        if (p.DistanceToPoint(vmax) >= 0.0f)
        {
            return ePlaneIntersectType::Intersect;
        }
    }
    return ePlaneIntersectType::Inside;
}

RayCastResult Frustum::Intersect(const Ray& _ray) const
{
    float tNear = k_floatMin;
    float tFar  = k_floatMax;

    for (const Plane& p: planes)
    {
        const Vec3  n     = p.GetNormal();
        const float d     = p.GetD();
        const float denom = Dot(n, _ray.direction);
        const float numer = -(Dot(n, _ray.origin) + d);

        if (IsZeroApprox(denom))
        {
            if (numer < 0.f)
            {
                return {};
            }
            else
            {
                continue;
            }
        }

        const float t = numer / denom;
        if (denom < 0.f)
        {
            tNear = Max(t, tNear);
        }
        else
        {
            tFar = Min(t, tFar);
        }

        if (tNear > tFar)
        {
            return {};
        }

        if (tFar < 0.f)
        {
            return {};
        }
    }

    RayCastResult result;
    result.bHit     = true;
    result.distance = tNear >= 0.f ? tNear : tFar;
    return result;
}

Vec3 Frustum::GetCenter() const
{
    const Vec3 nearCenter = nearPlane.GetNormal() * -nearPlane.GetD();
    const Vec3 farCenter  = farPlane.GetNormal() * -farPlane.GetD();

    return (nearCenter + farCenter) * 0.5f;
}

CornerPoints Frustum::GetCornerPoints() const
{
    CornerPoints corners;
    corners.nearLeftTop     = Plane::IntersectOf3Planes(nearPlane, leftPlane, topPlane);
    corners.nearRightTop    = Plane::IntersectOf3Planes(nearPlane, rightPlane, topPlane);
    corners.nearLeftBottom  = Plane::IntersectOf3Planes(nearPlane, leftPlane, bottomPlane);
    corners.nearRightBottom = Plane::IntersectOf3Planes(nearPlane, rightPlane, bottomPlane);
    corners.farLeftTop      = Plane::IntersectOf3Planes(farPlane, leftPlane, topPlane);
    corners.farRightTop     = Plane::IntersectOf3Planes(farPlane, rightPlane, topPlane);
    corners.farLeftBottom   = Plane::IntersectOf3Planes(farPlane, leftPlane, bottomPlane);
    corners.farRightBottom  = Plane::IntersectOf3Planes(farPlane, rightPlane, bottomPlane);
    return corners;
}

}   // namespace jam