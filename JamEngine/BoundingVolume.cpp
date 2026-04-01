#include "pch.h"

#include "BoundingVolume.h"

namespace Jam
{

namespace
{

    ePlaneIntersect IntersectPlaneAabb_(
        const Vec3  _center,
        const Vec3  _ext,
        const Vec3  _normal,
        const float _d)
    {
        const float dist = _normal.x * _center.x + _normal.y * _center.y + _normal.z * _center.z + _d;
        const float r    = Abs(_normal.x) * _ext.x + Abs(_normal.y) * _ext.y + Abs(_normal.z) * _ext.z;

        if (dist > r)
        {
            return ePlaneIntersect::Outside;
        }
        else if (dist < -r)
        {
            return ePlaneIntersect::Inside;
        }
        else
        {
            return ePlaneIntersect::Intersects;
        }
    }

}   // namespace

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

Aabb::Aabb(
    const Vec3 _min,
    const Vec3 _max)
    : min(_min)
    , max(_max)
{
}

Aabb Aabb::FromCenterExtents(
    const Vec3 _center,
    const Vec3 _ext)
{
    Aabb aabb = {};
    aabb.min  = _center - _ext;
    aabb.max  = _center + _ext;
    return aabb;
}

void Aabb::Transform(
    const Mtx4& _mtx)
{
    const Vec3 c    = GetCenter();
    const Vec3 e    = GetExtents();
    const Vec3 newC = Vec3::Transform(c, _mtx);
    const Vec3 r0   = Abs({ _mtx._11, _mtx._12, _mtx._13 });
    const Vec3 r1   = Abs({ _mtx._21, _mtx._22, _mtx._23 });
    const Vec3 r2   = Abs({ _mtx._31, _mtx._32, _mtx._33 });
    const Vec3 newE = { Dot(r0, e), Dot(r1, e), Dot(r2, e) };
    min             = newC - newE;
    max             = newC + newE;
}

Aabb Aabb::Transformed(
    const Mtx4& _mtx) const
{
    Aabb aabb = *this;
    aabb.Transform(_mtx);
    return aabb;
}

void Aabb::Merge(
    const Aabb& _aabb)
{
    min = Vec3 { Min(min.x, _aabb.min.x), Min(min.y, _aabb.min.y), Min(min.z, _aabb.min.z) };
    max = Vec3 { Max(max.x, _aabb.max.x), Max(max.y, _aabb.max.y), Max(max.z, _aabb.max.z) };
}

void Aabb::Merge(
    const Vec3 _p)
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
    CornerPoints corners              = {};
    corners[eRegion::LeftTopNear]     = Vec3 { min.x, max.y, min.z };
    corners[eRegion::RightTopNear]    = Vec3 { max.x, max.y, min.z };
    corners[eRegion::LeftBottomNear]  = Vec3 { min.x, min.y, min.z };
    corners[eRegion::RightBottomNear] = Vec3 { max.x, min.y, min.z };
    corners[eRegion::LeftTopFar]      = Vec3 { min.x, max.y, max.z };
    corners[eRegion::RightTopFar]     = Vec3 { max.x, max.y, max.z };
    corners[eRegion::LeftBottomFar]   = Vec3 { min.x, min.y, max.z };
    corners[eRegion::RightBottomFar]  = Vec3 { max.x, min.y, max.z };
    return corners;
}

bool Aabb::Contains(
    const Vec3 _p) const
{
    return _p.x >= min.x && _p.x <= max.x
        && _p.y >= min.y && _p.y <= max.y
        && _p.z >= min.z && _p.z <= max.z;
}

bool Aabb::Contains(
    const Aabb& _other) const
{
    return _other.min.x >= min.x && _other.max.x <= max.x
        && _other.min.y >= min.y && _other.max.y <= max.y
        && _other.min.z >= min.z && _other.max.z <= max.z;
}

bool Aabb::Intersects(
    const Aabb& _other) const
{
    // 접함도 교차로 간주
    if (max.x < _other.min.x || min.x > _other.max.x
        || max.y < _other.min.y || min.y > _other.max.y
        || max.z < _other.min.z || min.z > _other.max.z)
    {
        return false;
    }

    return true;
}

RayCastResult Aabb::Intersects(
    const Ray& _ray) const
{
    JAM_ASSERT(_ray.IsDirectionNormalized(), "Ray direction must be normalized to get correct intersection result.");

    // zero divide 여도 올바르게 계산됨.
    const Vec3 invDir = { 1.f / _ray.direction.x, 1.f / _ray.direction.y, 1.f / _ray.direction.z };

    const Vec3 t0 = (min - _ray.origin) * invDir;
    const Vec3 t1 = (max - _ray.origin) * invDir;

    const Vec3 tMin = { Min(t0.x, t1.x), Min(t0.y, t1.y), Min(t0.z, t1.z) };
    const Vec3 tMax = { Max(t0.x, t1.x), Max(t0.y, t1.y), Max(t0.z, t1.z) };

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

Obb::Obb(const Mtx4& _mtx)
    : mtx(_mtx)
{
}

Obb Obb::FromAabb(
    const Aabb& _aabb)
{
    const Vec3 center = _aabb.GetCenter();
    const Vec3 ext    = _aabb.GetExtents();

    Obb obb = {};
    obb.mtx = Mtx4::CreateScale(ext * 2.f) * Mtx4::CreateTranslation(center);
    return obb;
}

void Obb::Transform(
    const Mtx4& _mtx)
{
    mtx *= _mtx;
}

Obb Obb::Transformed(
    const Mtx4& _mtx) const
{
    Obb obb = *this;
    obb.Transform(_mtx);
    return obb;
}

Aabb Obb::ToAabb() const
{
    Aabb aabb = {};
    aabb.min  = Vec3 { -0.5f, -0.5f, -0.5f };
    aabb.max  = Vec3 { 0.5f, 0.5f, 0.5f };
    aabb.Transform(mtx);
    return aabb;
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
    return Length(mtx.Right());
}

float Obb::GetHeight() const
{
    return Length(mtx.Up());
}

float Obb::GetDepth() const
{
    return Length(mtx.Forward());
}

CornerPoints Obb::GetCornerPoints() const
{
    const Aabb aabb = Aabb::FromCenterExtents(Numbers<Vec3>::k_zero, { 0.5f, 0.5f, 0.5f });

    CornerPoints corners = aabb.GetCornerPoints();
    for (Vec3& p: corners)
    {
        p = Vec3::Transform(p, mtx);
    }

    return corners;
}

bool Obb::Contains(const Vec3 _point) const
{
    const Mtx4 invMtx = mtx.Invert();
    const Vec3 pointL = Vec3::Transform(_point, invMtx);

    Aabb boxL = {};
    boxL.min  = Vec3 { -0.5f, -0.5f, -0.5f };
    boxL.max  = Vec3 { 0.5f, 0.5f, 0.5f };

    return boxL.Contains(pointL);
}

RayCastResult Obb::Intersects(
    const Ray& _ray) const
{
    JAM_ASSERT(_ray.IsDirectionNormalized(), "Ray direction must be normalized to get correct intersection result.");

    const Mtx4 invMtx  = mtx.Invert();
    const Vec3 originL = Vec3::Transform(_ray.origin, invMtx);
    const Vec3 dirL    = Vec3::TransformNormal(_ray.direction, invMtx);

    const float dirLLen = Length(dirL);
    if (IsZeroApprox(dirLLen))
    {
        return {};
    }

    const Vec3 nDirL = dirL / dirLLen;
    const Ray  rayL  = { originL, nDirL };

    Aabb boxL = {};
    boxL.min  = Vec3 { -0.5f, -0.5f, -0.5f };
    boxL.max  = Vec3 { 0.5f, 0.5f, 0.5f };

    RayCastResult ret = boxL.Intersects(rayL);
    if (!ret.bHit)
    {
        return {};
    }

    ret.distance /= dirLLen;
    return ret;
}

Plane::Plane(
    const float _a,
    const float _b,
    const float _c,
    const float _d,
    const bool  _bNormalize)
    : normal(_a, _b, _c)
    , d(_d)
{
    if (_bNormalize)
    {
        Normalize();
    }
}

Plane::Plane(
    const Vec4 _vec,
    const bool _bNormalize)
    : Plane(_vec.x, _vec.y, _vec.z, _vec.w, _bNormalize)
{
}

Plane::Plane(
    const Vec3  _normal,
    const float _d,
    const bool  _bNormalize)
    : Plane(_normal.x, _normal.y, _normal.z, _d, _bNormalize)
{
}

Plane::Plane(
    const Vec3 _p1,
    const Vec3 _p2,
    const Vec3 _p3,
    const bool _bNormalize)
{
    const Vec3 v0 = _p2 - _p1;
    const Vec3 v1 = _p3 - _p1;
    normal        = Cross(v0, v1);
    d             = -Dot(normal, _p1);

    if (_bNormalize)
    {
        Normalize();
    }
}

Plane::Plane(
    const Vec3 _point,
    const Vec3 _normal,
    const bool _bNormalize)
{
    normal = _normal;
    d      = -Dot(_normal, _point);

    if (_bNormalize)
    {
        Normalize();
    }
}

Vec3 Plane::IntersectOf3Planes(
    const Plane& p1,
    const Plane& p2,
    const Plane& p3)
{
    const Vec3 n1 = p1.GetNormal();
    const Vec3 n2 = p2.GetNormal();
    const Vec3 n3 = p3.GetNormal();

    const float d1 = p1.GetD();
    const float d2 = p2.GetD();
    const float d3 = p3.GetD();

    const float crossDot = Dot(n1, Cross(n2, n3));
    if (IsZeroApprox(crossDot))
    {
        return Numbers<Vec3>::k_zero;
    }

    const Vec3 term1 = Cross(n2, n3) * -d1;
    const Vec3 term2 = Cross(n3, n1) * -d2;
    const Vec3 term3 = Cross(n1, n2) * -d3;

    return (term1 + term2 + term3) / crossDot;
}

Vec3 Plane::GetNormal() const
{
    return normal;   // AI
}

float Plane::GetD() const
{
    return d;   // AI
}

void Plane::Normalize()
{
    const float len = Length(normal);
    normal /= len;
    d /= len;
}

Plane Plane::Normalized() const
{
    Plane out = *this;
    out.Normalize();
    return out;
}

bool Plane::IsNormalized() const
{
    return Jam::IsNormalized(GetNormal());
}

void Plane::Transform(
    const Mtx4& _mtx,
    const bool  _normalize)
{
    const Vec4 plane = DirectX::XMPlaneTransform(Vec4From(normal, d), _mtx.Invert().Transpose());
    normal           = Vec3From(plane);
    d                = plane.w;

    if (_normalize)
    {
        Normalize();
    }
}

Plane Plane::Transformed(
    const Mtx4& _mtx,
    const bool  _normalize) const
{
    Plane out = *this;
    out.Transform(_mtx, _normalize);
    return out;
}

float Plane::DistanceToPoint(
    const Vec3 _point) const
{
    JAM_ASSERT(IsNormalized(), "Plane must be normalized to get correct distance.");
    return normal.x * _point.x + normal.y * _point.y + normal.z * _point.z + d;
}

RayCastResult Plane::Intersects(
    const Ray& _ray) const
{
    JAM_ASSERT(_ray.IsDirectionNormalized(), "Ray direction must be normalized to get correct intersection result.");

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

    RayCastResult result = {};
    result.bHit          = true;
    result.distance      = t;
    return result;
}

ePlaneIntersect Plane::Intersects(const Aabb& _box) const
{
    JAM_ASSERT(IsNormalized(), "Plane must be normalized to get correct intersection result.");

    // 정석적인 방법
    // 1. 평면의 노말을 기준으로 하여, 평면 기준 가장 먼점과 가까운 점을 찾는다. 이를 각각 n, f 라고 한다.
    // 2. n과 평면의 거리가 양수라면 Outside
    // 3. f과 평면의 거리가 음수라면 Inside
    // 4. 그 외에는 Intersects

    // const Vec3 n   = GetNormal();
    // const Vec3 pos = { n.x >= 0.f ? _box.max.x : _box.min.x, n.y >= 0.f ? _box.max.y : _box.min.y, n.z >= 0.f ? _box.max.z : _box.min.z };
    // const Vec3 neg = { n.x >= 0.f ? _box.min.x : _box.max.x, n.y >= 0.f ? _box.min.y : _box.max.y, n.z >= 0.f ? _box.min.z : _box.max.z };

    // if (Dot(n, neg) + GetD() > 0.f)
    //{
    //     return ePlaneIntersect::Outside;
    // }

    // if (Dot(n, pos) + GetD() < 0.f)
    //{
    //     return ePlaneIntersect::Inside;
    // }

    // return ePlaneIntersect::Intersects;

    // 최적화된 방법
    // 1. aabb는 원점에서부터 모든 방향으로 일정하게 떨어져있다.
    //    그러므로 extents를 평면의 노멀에 투영하여 최대거리를 구할 수 있다.
    //    여기서, e는 모두 양수이기 때문에, 평면의 노말도 양수로 바꿔서 계산한다.
    // 2. 평면의 노멀과 aabb의 중심을 이용하여, 평면과 aabb의 중심 사이의 거리를 구한다.
    // 3. 2에서 구한 거리와 1에서 구한 최대거리를 비교하여, 평면과 aabb의 관계를 구한다.

    const Vec3 c = _box.GetCenter();
    const Vec3 e = _box.GetExtents();
    return IntersectPlaneAabb_(c, e, GetNormal(), GetD());
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
    planes[eFrustumFace::Left]   = _left;
    planes[eFrustumFace::Right]  = _right;
    planes[eFrustumFace::Bottom] = _bottom;
    planes[eFrustumFace::Top]    = _top;
    planes[eFrustumFace::Near]   = _near;
    planes[eFrustumFace::Far]    = _far;

    if (_bNormalized)
    {
        Normalize();
    }
}

Frustum::Frustum(
    const Mtx4& _mtx,
    const bool  _bNormalized)
    : Frustum(
          Plane { -(_mtx._14 + _mtx._11), -(_mtx._24 + _mtx._21), -(_mtx._34 + _mtx._31), -(_mtx._44 + _mtx._41), false },
          Plane { -(_mtx._14 - _mtx._11), -(_mtx._24 - _mtx._21), -(_mtx._34 - _mtx._31), -(_mtx._44 - _mtx._41), false },
          Plane { -(_mtx._14 + _mtx._12), -(_mtx._24 + _mtx._22), -(_mtx._34 + _mtx._32), -(_mtx._44 + _mtx._42), false },
          Plane { -(_mtx._14 - _mtx._12), -(_mtx._24 - _mtx._22), -(_mtx._34 - _mtx._32), -(_mtx._44 - _mtx._42), false },
          Plane { -_mtx._13, -_mtx._23, -_mtx._33, -_mtx._43, false },
          Plane { -(_mtx._14 - _mtx._13), -(_mtx._24 - _mtx._23), -(_mtx._34 - _mtx._33), -(_mtx._44 - _mtx._43), false },
          _bNormalized)
{
}

void Frustum::Transform(
    const Mtx4& _mtx,
    const bool  _bNormalize)
{
    const Mtx4 invTransMtx = _mtx.Invert().Transpose();
    for (Plane& p: planes)
    {
        const Vec4 plane = DirectX::XMPlaneTransform(Vec4From(p.GetNormal(), p.GetD()), invTransMtx);
        p.normal         = Vec3From(plane);
        p.d              = plane.w;
    }

    if (_bNormalize)
    {
        Normalize();
    }
}

Frustum Frustum::Transformed(
    const Mtx4& _mtx,
    const bool  _bNormalize) const
{
    Frustum out = *this;
    out.Transform(_mtx, _bNormalize);
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

bool Frustum::IsNormalized() const
{
    for (const Plane& p: planes)
    {
        if (!p.IsNormalized())
        {
            return false;
        }
    }
    return true;
}

Vec3 Frustum::GetCenter() const
{
    const CornerPoints corners = GetCornerPoints();
    Vec3               sum     = Numbers<Vec3>::k_zero;
    for (const Vec3& p: corners)
    {
        sum = sum + p;
    }
    return sum / static_cast<float>(corners.size());
}

CornerPoints Frustum::GetCornerPoints() const
{
    const Plane n = planes[eFrustumFace::Near];
    const Plane f = planes[eFrustumFace::Far];
    const Plane l = planes[eFrustumFace::Left];
    const Plane r = planes[eFrustumFace::Right];
    const Plane t = planes[eFrustumFace::Top];
    const Plane b = planes[eFrustumFace::Bottom];

    CornerPoints corners              = {};
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

ePlaneIntersect Frustum::Intersects(
    const Aabb& _box) const
{
    bool       bAnyIntersect = false;
    const Vec3 c             = _box.GetCenter();
    const Vec3 e             = _box.GetExtents();

    for (const Plane& p: planes)
    {
        const ePlaneIntersect result = IntersectPlaneAabb_(c, e, p.GetNormal(), p.GetD());
        if (result == ePlaneIntersect::Outside)
        {
            return ePlaneIntersect::Outside;
        }
        else if (result == ePlaneIntersect::Intersects)
        {
            bAnyIntersect = true;
        }
    }
    return bAnyIntersect ? ePlaneIntersect::Intersects : ePlaneIntersect::Inside;
}

RayCastResult Frustum::Intersects(const Ray& _ray) const
{
    JAM_ASSERT(_ray.IsDirectionNormalized(), "Ray direction must be normalized to get correct intersection result.");

    float tNear = -Limits<float>::k_max;
    float tFar  = Limits<float>::k_max;

    for (const Plane& p: planes)
    {
        const Vec3  n     = p.GetNormal();
        const float d     = p.GetD();
        const float denom = Dot(n, _ray.direction);
        const float numer = -(Dot(n, _ray.origin) + d);

        if (IsZeroApprox(denom))
        {
            if (p.DistanceToPoint(_ray.origin) > 0.f)
            {
                return {};
            }
            continue;
        }

        const float t = numer / denom;
        if (denom < 0.f)
        {
            tNear = Max(tNear, t);
        }
        else
        {
            tFar = Min(tFar, t);
        }

        if (tNear > tFar)
        {
            return {};
        }
    }

    if (tFar < 0.f)
    {
        return {};
    }

    RayCastResult result = {};
    result.bHit          = true;
    result.distance      = tNear >= 0.f ? tNear : tFar;
    return result;
}

Ray::Ray(
    const Vec3 _origin,
    const Vec3 _direction,
    const bool _bNormalizeDir)
    : origin(_origin)
    , direction(_direction)
{
    if (_bNormalizeDir)
    {
        NormalizeDirection();
    }
}

void Ray::NormalizeDirection()
{
    direction = Normalized(direction);
}

Ray Ray::NormalizedDirection() const
{
    Ray out = *this;
    out.NormalizeDirection();
    return out;
}

bool Ray::IsDirectionNormalized() const
{
    return IsNormalized(direction);
}

}   // namespace Jam