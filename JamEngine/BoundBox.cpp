#include "pch.h"

#include "BoundBox.h"

namespace jam
{

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

Plane::Plane(const Vec3& _normal, const float _d)
    : normal(_normal)
    , d(_d)
{
}

Plane::Plane(const Vec3& _p1, const Vec3& _p2, const Vec3& _p3)
{
    using namespace DirectX;
    XMPlaneFromPoints(XMLoadFloat3(&_p1), XMLoadFloat3(&_p2), XMLoadFloat3(&_p3));
}

Plane::Plane(const Vec3& _p, const Vec3& _n)
{
    using namespace DirectX;
    XMPlaneFromPointNormal(XMLoadFloat3(&_p), XMLoadFloat3(&_n));
}

Plane Plane::Transform(const Plane& _plane, const Mat4& _mat)
{
    using namespace DirectX;
    Plane out;
    out.vec = XMPlaneTransform(_plane.vec, _mat);
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
    {
        return Vec3::Zero;
    }
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

Frustum::Frustum(const Mat4& _projMat)
{
    using namespace DirectX;
    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, XMLoadFloat4x4(&_projMat));

    planes[0].normal = Vec3 { m._14 + m._11, m._24 + m._21, m._34 + m._31 };
    planes[0].d      = m._44 + m._41;   // Left
    planes[1].normal = Vec3 { m._14 - m._11, m._24 - m._21, m._34 - m._31 };
    planes[1].d      = m._44 - m._41;   // Right
    planes[2].normal = Vec3 { m._14 + m._12, m._24 + m._22, m._34 + m._32 };
    planes[2].d      = m._44 + m._42;   // Bottom
    planes[3].normal = Vec3 { m._14 - m._12, m._24 - m._22, m._34 - m._32 };
    planes[3].d      = m._44 - m._42;   // Top
    planes[4].normal = Vec3 { m._13, m._23, m._33 };
    planes[4].d      = m._43;   // Near
    planes[5].normal = Vec3 { m._14 - m._13, m._24 - m._23, m._34 - m._33 };
    planes[5].d      = m._44 - m._43;   // Far

    for (Plane& p: planes)
        p.Normalize();

    position = Vec3::Zero;
}

Frustum Frustum::Transform(const Frustum& _frustum, const Mat4& _mat)
{
    Frustum out = {};
    for (size_t i = 0; i < 6; ++i)
    {
        out.planes[i] = Plane::Transform(_frustum.planes[i], _mat);
    }
    out.position = Vec3::Transform(_frustum.position, _mat);
    return out;
}

ePlaneIntersactType Frustum::Intersects(const AABB& _box) const
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
            return ePlaneIntersactType::Outside;

        if (p.DistanceToPoint(vmax) >= 0.0f)
            return ePlaneIntersactType::Intersect;
    }
    return ePlaneIntersactType::Inside;
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

Vec3 Frustum::GetLookTo() const
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