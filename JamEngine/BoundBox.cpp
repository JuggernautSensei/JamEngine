#include "pch.h"

#include "BoundBox.h"

namespace jam
{

AABB AABB::FromCenterExtents(const Vec3& _center, const Vec3& _extents)
{
    AABB out;
    out.min = _center - _extents;
    out.max = _center + _extents;
    return out;
}

AABB AABB::Transform(const AABB& _box, const Mat4& _mat)
{
    using namespace DirectX;

    AABB     out;
    XMVECTOR vMin      = XMLoadFloat3(&_box.min);
    XMVECTOR vMax      = XMLoadFloat3(&_box.max);
    XMVECTOR vCenter   = XMVectorScale(XMVectorAdd(vMin, vMax), 0.5f);
    XMVECTOR vExtents  = XMVectorScale(XMVectorSubtract(vMax, vMin), 0.5f);
    XMMATRIX M         = XMLoadFloat4x4(&_mat);
    XMVECTOR r0        = M.r[0];
    XMVECTOR r1        = M.r[1];
    XMVECTOR r2        = M.r[2];
    XMVECTOR newCenter = XMVector3Transform(vCenter, M);
    r0                 = XMVectorAbs(r0);
    r1                 = XMVectorAbs(r1);
    r2                 = XMVectorAbs(r2);
    XMVECTOR ex        = XMVector3Dot(r0, vExtents);
    XMVECTOR ey        = XMVector3Dot(r1, vExtents);
    XMVECTOR ez        = XMVector3Dot(r2, vExtents);
    XMVECTOR newExt    = XMVectorSet(XMVectorGetX(ex), XMVectorGetX(ey), XMVectorGetX(ez), 0.0f);
    XMVECTOR newMin    = XMVectorSubtract(newCenter, newExt);
    XMVECTOR newMax    = XMVectorAdd(newCenter, newExt);
    XMStoreFloat3(&out.min, newMin);
    XMStoreFloat3(&out.max, newMax);
    return out;
}

Vec3 AABB::Center() const
{
    return (min + max) * 0.5f;
}

Vec3 AABB::Extents() const
{
    return (max - min) * 0.5f;
}

float AABB::Width() const
{
    return max.x - min.x;
}

float AABB::Height() const
{
    return max.y - min.y;
}

float AABB::Depth() const
{
    return max.z - min.z;
}

bool AABB::Intersects(const AABB& _other) const
{
    if (max.x < _other.min.x || min.x > _other.max.x) return false;
    if (max.y < _other.min.y || min.y > _other.max.y) return false;
    if (max.z < _other.min.z || min.z > _other.max.z) return false;
    return true;
}

bool AABB::Contains(const Vec3& _p) const
{
    return _p.x >= min.x
        && _p.x <= max.x
        && _p.y >= min.y
        && _p.y <= max.y
        && _p.z >= min.z
        && _p.z <= max.z;
}

OBB OBB::Transform(const OBB& _obb, const Mat4& _mat)
{
    OBB out;
    out.mtx = _obb.mtx * _mat;
    return out;
}

AABB OBB::ToAABB(const OBB& _obb)
{
    AABB aabb;
    aabb.min = Vec3 { -1.f, -1.f, -1.f };
    aabb.max = Vec3 { 1.f, 1.f, 1.f };
    return AABB::Transform(aabb, _obb.mtx);
}

Vec3 OBB::Center() const
{
    return mtx.Translation();
}

Vec3 OBB::Extents()
{
    Vec3 s;
    Quat q;
    Vec3 t;
    mtx.Decompose(s, q, t);
    return Absolute(s) * 0.5f;
}

AABB AABB::Merge(const AABB& _a, const AABB& _b)
{
    AABB out;
    out.min.x = Min(_a.min.x, _b.min.x);
    out.min.y = Min(_a.min.y, _b.min.y);
    out.min.z = Min(_a.min.z, _b.min.z);
    out.max.x = Max(_a.max.x, _b.max.x);
    out.max.y = Max(_a.max.y, _b.max.y);
    out.max.z = Max(_a.max.z, _b.max.z);
    return out;
}

AABB AABB::Merge(const AABB& _a, const Vec3& _pointA)
{
    AABB out;
    out.min.x = Min(_a.min.x, _pointA.x);
    out.min.y = Min(_a.min.y, _pointA.y);
    out.min.z = Min(_a.min.z, _pointA.z);
    out.max.x = Max(_a.max.x, _pointA.x);
    out.max.y = Max(_a.max.y, _pointA.y);
    out.max.z = Max(_a.max.z, _pointA.z);
    return out;
}

Plane::Plane(const Vec3& _normal, const float _d)
    : normal(_normal)
    , d(_d)
{
    Normalize();
}

Plane::Plane(const Vec3& _p1, const Vec3& _p2, const Vec3& _p3)
{
    using namespace DirectX;
    XMPlaneFromPoints(XMLoadFloat3(&_p1), XMLoadFloat3(&_p2), XMLoadFloat3(&_p3));
    Normalize();
}

Plane::Plane(const Vec3& _p, const Vec3& _n)
{
    using namespace DirectX;
    XMPlaneFromPointNormal(XMLoadFloat3(&_p), XMLoadFloat3(&_n));
    XMPlaneNormalize(vec);
}

Plane Plane::Transform(const Plane& _plane, const Mat4& _mat)
{
    using namespace DirectX;
    Plane out;
    out.vec = XMPlaneTransform(_plane.vec, _mat);
    out.Normalize();
    return out;
}

Vec3 Plane::IntersectOf3Planes(const Plane& p1, const Plane& p2, const Plane& p3)
{
    using namespace DirectX;

    XMVECTOR n1 = XMLoadFloat3(&p1.normal);
    XMVECTOR n2 = XMLoadFloat3(&p2.normal);
    XMVECTOR n3 = XMLoadFloat3(&p3.normal);

    XMVECTOR crossDot = XMVector3Dot(n1, XMVector3Cross(n2, n3));
    float    denom    = XMVectorGetX(crossDot);
    if (Absolute(denom) < k_epsilon)
        return Vec3::Zero;

    XMVECTOR term1 = XMVectorScale(XMVector3Cross(n2, n3), -p1.d);
    XMVECTOR term2 = XMVectorScale(XMVector3Cross(n3, n1), -p2.d);
    XMVECTOR term3 = XMVectorScale(XMVector3Cross(n1, n2), -p3.d);
    return XMVectorScale(XMVectorAdd(XMVectorAdd(term1, term2), term3), 1.0f / denom);
}

void Plane::Normalize()
{
    using namespace DirectX;
    vec = XMPlaneNormalize(vec);
}

float Plane::DistanceToPoint(const Vec3& _point) const
{
    using namespace DirectX;
    XMVECTOR p = XMLoadFloat3(&_point);
    p          = XMVectorSetW(p, 1.0f);
    return XMVectorGetX(XMPlaneDotCoord(vec, p));   // ax+by+cz+d
}

Frustum::Frustum(const Mat4& _viewProj)
{
    using namespace DirectX;
    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, XMLoadFloat4x4(&_viewProj));

    leftPlane.normal   = Vec3 { m._14 + m._11, m._24 + m._21, m._34 + m._31 };
    leftPlane.d        = m._44 + m._41;   // Left
    rightPlane.normal  = Vec3 { m._14 - m._11, m._24 - m._21, m._34 - m._31 };
    rightPlane.d       = m._44 - m._41;   // Right
    bottomPlane.normal = Vec3 { m._14 + m._12, m._24 + m._22, m._34 + m._32 };
    bottomPlane.d      = m._44 + m._42;   // Bottom
    topPlane.normal    = Vec3 { m._14 - m._12, m._24 - m._22, m._34 - m._32 };
    topPlane.d         = m._44 - m._42;   // Top
    nearPlane.normal   = Vec3 { m._13, m._23, m._33 };
    nearPlane.d        = m._43;   // Near
    farPlane.normal    = Vec3 { m._14 - m._13, m._24 - m._23, m._34 - m._33 };
    farPlane.d         = m._44 - m._43;   // Far

    for (Plane& p: planes)
        p.Normalize();

    position = Plane::IntersectOf3Planes(leftPlane, rightPlane, topPlane);
}

Frustum Frustum::CreateSubFrustum(const Frustum& _frustum, const float _startWeight, const float _endWeight)
{
    JAM_ASSERT(_startWeight <= _endWeight, "Start weight must be less than or equal to end weight");
    JAM_ASSERT(_startWeight >= 0.0f && _startWeight <= 1.0f, "Start weight must be in range [0.0, 1.0]");
    JAM_ASSERT(_endWeight >= 0.0f && _endWeight <= 1.0f, "End weight must be in range [0.0, 1.0]");

    Frustum subFrustum     = _frustum;
    float   nearDist       = _frustum.GetNear();
    float   farDist        = _frustum.GetFar();
    float   range          = farDist - nearDist;
    float   newNearDist    = nearDist + range * _startWeight;
    float   newFarDist     = nearDist + range * _endWeight;
    subFrustum.nearPlane.d = -newNearDist;
    subFrustum.farPlane.d  = -newFarDist;
    subFrustum.planes[0]   = subFrustum.nearPlane;
    subFrustum.planes[1]   = subFrustum.farPlane;
    return subFrustum;
}

Frustum Frustum::Transform(const Frustum& _frustum, const Mat4& _mat)
{
    Frustum out = {};
    for (size_t i = 0; i < 6; ++i)
        out.planes[i] = Plane::Transform(_frustum.planes[i], _mat);
    out.position = Vec3::Transform(_frustum.position, _mat);
    return out;
}

ePlaneIntersectType Frustum::Intersects(const AABB& _box) const
{
    for (const Plane& p: planes)
    {
        Vec3 vmin(
            p.normal.x >= 0 ? _box.min.x : _box.max.x,
            p.normal.y >= 0 ? _box.min.y : _box.max.y,
            p.normal.z >= 0 ? _box.min.z : _box.max.z);

        Vec3 vmax(
            p.normal.x >= 0 ? _box.max.x : _box.min.x,
            p.normal.y >= 0 ? _box.max.y : _box.min.y,
            p.normal.z >= 0 ? _box.max.z : _box.min.z);

        if (p.DistanceToPoint(vmin) > 0.0f)
            return ePlaneIntersectType::Outside;

        if (p.DistanceToPoint(vmax) >= 0.0f)
            return ePlaneIntersectType::Intersect;
    }
    return ePlaneIntersectType::Inside;
}

bool Frustum::Intersects(const Sphere& _sphere) const
{
    for (const Plane& p: planes)
    {
        float dist = p.DistanceToPoint(_sphere.center);
        if (dist > _sphere.radius)
            return false;
    }
    return true;
}

FrustumCorners Frustum::GetCorners() const
{
    FrustumCorners corners;
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

Vec3 Frustum::GetCenter() const
{
    FrustumCorners corners = GetCorners();
    Vec3           center  = Vec3::Zero;
    for (const Vec3& point: corners.points)
    {
        center += point;
    }
    center /= 8.f;
    return center;
}

Vec3 Frustum::ViewLookTo() const
{
    return -nearPlane.normal;
}

float Frustum::GetNear() const
{
    return -nearPlane.d;
}

float Frustum::GetFar() const
{
    return -farPlane.d;
}

}   // namespace jam