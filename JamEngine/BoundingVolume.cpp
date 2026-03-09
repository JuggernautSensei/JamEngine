#include "pch.h"

#include "BoundingVolume.h"

namespace Jam
{

Aabb::Aabb(
    const Vector3 _min,
    const Vector3 _max)
    : min(_min)
      , max(_max)
{
}

Aabb Aabb::FromCenterExtents(
    const Vector3 _center,
    const Vector3 _ext)
{
    Aabb out = {};
    out.min  = _center - _ext;
    out.max  = _center + _ext;
    return out;
}

void Aabb::Transform(
    const Matrix4x4& _mtx)
{
    const Vector3 center = GetCenter();
    const Vector3 ext    = GetExtents();

    // transform center
    const Vector3 newCenter = Vector3::Transform(center, _mtx);

    // transform extents
    const Vector3 r0      = Abs({ _mtx._11, _mtx._12, _mtx._13 });
    const Vector3 r1      = Abs({ _mtx._21, _mtx._22, _mtx._23 });
    const Vector3 r2      = Abs({ _mtx._31, _mtx._32, _mtx._33 });
    const Vector3 nextExt = { Dot(r0, ext), Dot(r1, ext), Dot(r2, ext) };

    min = newCenter - nextExt;
    max = newCenter + nextExt;
}

Aabb Aabb::Transformed(
    const Matrix4x4& _mtx) const
{
    Aabb aabb = *this;
    aabb.Transform(_mtx);
    return aabb;
}

void Aabb::Merge(
    const Aabb& _aabb)
{
    min = Vector3 { Min(min.x, _aabb.min.x), Min(min.y, _aabb.min.y), Min(min.z, _aabb.min.z) };
    max = Vector3 { Max(max.x, _aabb.max.x), Max(max.y, _aabb.max.y), Max(max.z, _aabb.max.z) };
}

void Aabb::Merge(
    const Vector3 _p)
{
    min = Vector3 { Min(min.x, _p.x), Min(min.y, _p.y), Min(min.z, _p.z) };
    max = Vector3 { Max(max.x, _p.x), Max(max.y, _p.y), Max(max.z, _p.z) };
}

Vector3 Aabb::GetCenter() const
{
    return (min + max) * 0.5f;
}

Vector3 Aabb::GetExtents() const
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
    CornerPoints corners              = {};
    corners[eRegion::NearLeftTop]     = Vector3 { min.x, max.y, min.z };
    corners[eRegion::NearRightTop]    = Vector3 { max.x, max.y, min.z };
    corners[eRegion::NearLeftBottom]  = Vector3 { min.x, min.y, min.z };
    corners[eRegion::NearRightBottom] = Vector3 { max.x, min.y, min.z };
    corners[eRegion::FarLeftTop]      = Vector3 { min.x, max.y, max.z };
    corners[eRegion::FarRightTop]     = Vector3 { max.x, max.y, max.z };
    corners[eRegion::FarLeftBottom]   = Vector3 { min.x, min.y, max.z };
    corners[eRegion::FarRightBottom]  = Vector3 { max.x, min.y, max.z };
    return corners;
}

bool Aabb::Contain(
    const Vector3 _p) const
{
    return _p.x >= min.x && _p.x <= max.x
           && _p.y >= min.y && _p.y <= max.y
           && _p.z >= min.z && _p.z <= max.z;
}

bool Aabb::Contain(
    const Aabb& _other) const
{
    return _other.min.x >= min.x && _other.max.x <= max.x
           && _other.min.y >= min.y && _other.max.y <= max.y
           && _other.min.z >= min.z && _other.max.z <= max.z;
}

bool Aabb::Intersect(
    const Aabb& _other) const
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

RayCastResult Aabb::Intersect(
    const Ray& _ray) const
{
    const Vector3 invDir = {
        IsZeroApprox(_ray.direction.x) ? Limits<float>::k_max : 1.f / _ray.direction.x,
        IsZeroApprox(_ray.direction.y) ? Limits<float>::k_max : 1.f / _ray.direction.y,
        IsZeroApprox(_ray.direction.z) ? Limits<float>::k_max : 1.f / _ray.direction.z
    };

    const Vector3 t0 = (min - _ray.origin) * invDir;
    const Vector3 t1 = (max - _ray.origin) * invDir;

    const Vector3 tMin = { Min(t0.x, t1.x), Min(t0.y, t1.y), Min(t0.z, t1.z) };
    const Vector3 tMax = { Max(t0.x, t1.x), Max(t0.y, t1.y), Max(t0.z, t1.z) };

    const float tNear = Max(Max(tMin.x, tMin.y), tMin.z);
    const float tFar  = Min(Min(tMax.x, tMax.y), tMax.z);
    if (tNear > tFar || tFar < 0.f)
    {
        return {};
    }

    RayCastResult result = {};
    result.bHit          = true;
    result.distance      = tNear >= 0.f ? tNear : tFar;
    return result;
}

Obb::Obb(const Matrix4x4& _mtx)
    : mtx(_mtx)
{
}

Obb Obb::FromAabb(
    const Aabb& _aabb)
{
    const Vector3 center = _aabb.GetCenter();
    const Vector3 ext    = _aabb.GetExtents();

    Obb obb = {};
    obb.mtx = Matrix4x4::CreateScale(ext * 2.f) * Matrix4x4::CreateTranslation(center);
    return obb;
}

void Obb::Transform(
    const Matrix4x4& _mtx)
{
    mtx *= _mtx;
}

Obb Obb::Transformed(
    const Matrix4x4& _mtx) const
{
    Obb obb = *this;
    obb.Transform(_mtx);
    return obb;
}

Aabb Obb::ToAabb() const
{
    Aabb aabb = {};
    aabb.min  = Vector3 { -1.f, -1.f, -1.f };
    aabb.max  = Vector3 { 1.f, 1.f, 1.f };
    aabb.Transform(mtx);
    return aabb;
}

Vector3 Obb::GetCenter() const
{
    return mtx.Translation();
}

Vector3 Obb::GetExtents() const
{
    const Vector3  r   = mtx.Right();
    const Vector3  u   = mtx.Up();
    const Vector3  f   = mtx.Forward();
    const float hsx = Length(r) * 0.5f;
    const float hsy = Length(u) * 0.5f;
    const float hsz = Length(f) * 0.5f;
    return { hsx, hsy, hsz };
}

float Obb::GetWidth() const
{
    const Vector3  r  = mtx.Right();
    const float sx = Length(r);
    return sx;
}

float Obb::GetHeight() const
{
    const Vector3  u  = mtx.Up();
    const float sy = Length(u);
    return sy;
}

float Obb::GetDepth() const
{
    const Vector3  f  = mtx.Forward();
    const float sz = Length(f);
    return sz;
}

CornerPoints Obb::GetCornerPoints() const
{
    const Aabb aabb = Aabb::FromCenterExtents(Vec3::k_zero, { 0.5f, 0.5f, 0.5f });

    CornerPoints corners = aabb.GetCornerPoints();
    for (Vector3& p: corners)
    {
        p = Vector3::Transform(p, mtx);
    }
    return corners;
}

bool Obb::Contain(const Vector3 _point) const
{
    // world point to local point
    const Matrix4x4 invMtx     = mtx.Invert();
    const Vector3 localPoint = Vector3::Transform(_point, invMtx);

    // test local point vs tmp unit local aabb
    Aabb localBox = {};
    localBox.min  = Vector3 { -0.5f, -0.5f, -0.5f };
    localBox.max  = Vector3 { 0.5f, 0.5f, 0.5f };

    return localBox.Contain(localPoint);
}

RayCastResult Obb::Intersect(const Ray& _ray) const
{
    // world ray to local ray
    const Matrix4x4 invMtx         = mtx.Invert();
    const Vector3 localOrigin    = Vector3::Transform(_ray.origin, invMtx);
    const Vector3 localDirection = Vector3::TransformNormal(_ray.direction, invMtx);

    // tmp unit local aabb
    Aabb localBox = {};
    localBox.min  = Vector3 { -0.5f, -0.5f, -0.5f };
    localBox.max  = Vector3 { 0.5f, 0.5f, 0.5f };

    // test local ray vs local aabb
    Ray localRay       = {};
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

Plane::Plane(
    const float _a,
    const float _b,
    const float _c,
    const float _d)
    : vec(_a, _b, _c, _d)
{
}

Plane::Plane(
    const Vector4 _vec)
    : vec(_vec)
{
}

Plane::Plane(
    const Vector3  _normal,
    const float _d)
    : vec(_normal.x, _normal.y, _normal.z, _d)
{
    Normalize();
}

Plane::Plane(
    const Vector3 _p1,
    const Vector3 _p2,
    const Vector3 _p3)
{
    vec = DirectX::XMPlaneFromPoints(_p1, _p2, _p3);
    Normalize();
}

Plane::Plane(
    const Vector3 _point,
    const Vector3 _normal)
{
    vec = DirectX::XMPlaneFromPointNormal(_point, _normal);
    Normalize();
}

Vector3 Plane::IntersectOf3Planes(const Plane& p1, const Plane& p2, const Plane& p3)
{
    using namespace DirectX;

    const Vector3 n1 = p1.GetNormal();
    const Vector3 n2 = p2.GetNormal();
    const Vector3 n3 = p3.GetNormal();

    const float d1 = p1.GetD();
    const float d2 = p2.GetD();
    const float d3 = p3.GetD();

    const float crossDot = Dot(n1, Cross(n2, n3));
    if (IsZeroApprox(crossDot))
    {
        return Vec3::k_zero;
    }

    const Vector3 term1 = Cross(n2, n3) * -d1;
    const Vector3 term2 = Cross(n3, n1) * -d2;
    const Vector3 term3 = Cross(n1, n2) * -d3;
    const Vector3 sum   = term1 + term2 + term3;

    return sum / crossDot;
}

Vector3 Plane::GetNormal() const
{
    return { vec.x, vec.y, vec.z };
}

float Plane::GetD() const
{
    return vec.w;
}

void Plane::Normalize()
{
    using namespace DirectX;
    vec = XMPlaneNormalize(vec);
}

Plane Plane::Normalized() const
{
    Plane out = *this;
    out.Normalize();
    return out;
}

void Plane::Transform(const Matrix4x4& _mtx)
{
    vec = DirectX::XMPlaneTransform(vec, _mtx);
    Normalize();
}

Plane Plane::Transformed(const Matrix4x4& _mtx) const
{
    Plane out = *this;
    out.Transform(_mtx);
    return out;
}

float Plane::DistanceToPoint(const Vector3 _point) const
{
    using namespace DirectX;
    return DirectX::XMVectorGetX(XMPlaneDotCoord(vec, _point));
}

RayCastResult Plane::Intersect(const Ray& _ray) const
{
    const Vector3  n     = GetNormal();
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

    RayCastResult result = {};
    result.bHit          = true;
    result.distance      = t;
    return result;
}

Frustum::Frustum(
    Plane _near,
    Plane _far,
    Plane _left,
    Plane _right,
    Plane _top,
    Plane _bottom)
    : nearPlane(_near)
      , farPlane(_far)
      , leftPlane(_left)
      , rightPlane(_right)
      , topPlane(_top)
      , bottomPlane(_bottom)
{
}

Frustum::Frustum(
    const Matrix4x4& _mtx)
    : nearPlane({ _mtx._13, _mtx._23, _mtx._33, _mtx._43 })
      , farPlane({ _mtx._14 - _mtx._13, _mtx._24 - _mtx._23, _mtx._34 - _mtx._33, _mtx._44 - _mtx._43 })
      , leftPlane({ _mtx._14 + _mtx._11, _mtx._24 + _mtx._21, _mtx._34 + _mtx._31, _mtx._44 + _mtx._41 })
      , rightPlane({ _mtx._14 - _mtx._11, _mtx._24 - _mtx._21, _mtx._34 - _mtx._31, _mtx._44 - _mtx._41 })
      , topPlane({ _mtx._14 - _mtx._12, _mtx._24 - _mtx._22, _mtx._34 - _mtx._32, _mtx._44 - _mtx._42 })
      , bottomPlane({ _mtx._14 + _mtx._12, _mtx._24 + _mtx._22, _mtx._34 + _mtx._32, _mtx._44 + _mtx._42 })

{
}

void Frustum::Transform(
    const Matrix4x4& _mtx)
{
    for (Plane& p: planes)
    {
        p.Transform(_mtx);
    }
}

Frustum Frustum::Transformed(
    const Matrix4x4& _mtx) const
{
    Frustum out = *this;
    out.Transform(_mtx);
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

Vector3 Frustum::GetCenter() const
{
    // normalized plane need
    // this is approximate center
    // maybe practically use
    const Vector3 nearCenter = nearPlane.GetNormal() * -nearPlane.GetD();
    const Vector3 farCenter  = farPlane.GetNormal() * -farPlane.GetD();
    return (nearCenter + farCenter) * 0.5f;
}

CornerPoints Frustum::GetCornerPoints() const
{
    CornerPoints corners              = {};
    corners[eRegion::NearLeftTop]     = Plane::IntersectOf3Planes(nearPlane, leftPlane, topPlane);
    corners[eRegion::NearRightTop]    = Plane::IntersectOf3Planes(nearPlane, rightPlane, topPlane);
    corners[eRegion::NearLeftBottom]  = Plane::IntersectOf3Planes(nearPlane, leftPlane, bottomPlane);
    corners[eRegion::NearRightBottom] = Plane::IntersectOf3Planes(nearPlane, rightPlane, bottomPlane);
    corners[eRegion::FarLeftTop]      = Plane::IntersectOf3Planes(farPlane, leftPlane, topPlane);
    corners[eRegion::FarRightTop]     = Plane::IntersectOf3Planes(farPlane, rightPlane, topPlane);
    corners[eRegion::FarLeftBottom]   = Plane::IntersectOf3Planes(farPlane, leftPlane, bottomPlane);
    corners[eRegion::FarRightBottom]  = Plane::IntersectOf3Planes(farPlane, rightPlane, bottomPlane);
    return corners;
}

ePlaneIntersectType Frustum::Intersect(
    const Aabb& _box) const
{
    for (const Plane& p: planes)
    {
        const Vector3 n = p.GetNormal();

        const Vector3 vmin {
            n.x >= 0 ? _box.min.x : _box.max.x,
            n.y >= 0 ? _box.min.y : _box.max.y,
            n.z >= 0 ? _box.min.z : _box.max.z
        };

        const Vector3 vmax {
            n.x >= 0 ? _box.max.x : _box.min.x,
            n.y >= 0 ? _box.max.y : _box.min.y,
            n.z >= 0 ? _box.max.z : _box.min.z
        };

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
    float tNear = -Limits<float>::k_max;
    float tFar  = Limits<float>::k_max;

    for (const Plane& p: planes)
    {
        const Vector3  n     = p.GetNormal();
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

    RayCastResult result = {};
    result.bHit          = true;
    result.distance      = tNear >= 0.f ? tNear : tFar;
    return result;
}

}   // namespace Jam