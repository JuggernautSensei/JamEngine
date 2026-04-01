#pragma once
#include "EnumArray.h"

namespace Jam
{

struct Ray;

enum class eRegion
{
    //                    x y z
    LeftBottomNear,    // 0 0 0
    LeftBottomFar,     // 0 0 1
    LeftTopNear,       // 0 1 0
    LeftTopFar,        // 0 1 1
    RightBottomNear,   // 1 0 0
    RightBottomFar,    // 1 0 1
    RightTopNear,      // 1 1 0
    RightTopFar        // 1 1 1
};

[[nodiscard]] eRegion CalcRegion(bool _bRight, bool _bTop, bool _bFar);
[[nodiscard]] bool    IsRight(eRegion _region);
[[nodiscard]] bool    IsTop(eRegion _region);
[[nodiscard]] bool    IsFar(eRegion _region);

using CornerPoints = EnumArray<eRegion, Vec3>;

enum class eFrustumFace
{
    Near,   // frustum culling 시 near far에서 early out 위해 near far을 앞에 배치
    Far,
    Left,
    Right,
    Bottom,
    Top,
};

struct RayCastResult
{
    bool  bHit     = false;
    float distance = 0.f;
};

struct Aabb
{
    Aabb() = default;
    explicit Aabb(Vec3 _min, Vec3 _max);
    [[nodiscard]] static Aabb FromCenterExtents(Vec3 _center, Vec3 _ext);

    void               Transform(const Mtx4& _mtx);
    [[nodiscard]] Aabb Transformed(const Mtx4& _mtx) const;

    void Merge(const Aabb& _aabb);
    void Merge(Vec3 _p);

    [[nodiscard]] Vec3         GetCenter() const;
    [[nodiscard]] Vec3         GetExtents() const;
    [[nodiscard]] float        GetWidth() const;
    [[nodiscard]] float        GetHeight() const;
    [[nodiscard]] float        GetDepth() const;
    [[nodiscard]] CornerPoints GetCornerPoints() const;

    [[nodiscard]] bool          Contains(Vec3 _point) const;
    [[nodiscard]] bool          Contains(const Aabb& _other) const;
    [[nodiscard]] bool          Intersects(const Aabb& _other) const;
    [[nodiscard]] RayCastResult Intersects(const Ray& _ray) const;

    Vec3 min = Numbers<Vec3>::k_max;
    Vec3 max = Numbers<Vec3>::k_min;
};

struct Obb
{
    Obb() = default;
    explicit Obb(const Mtx4& _mtx);
    [[nodiscard]] static Obb FromAabb(const Aabb& _aabb);

    void              Transform(const Mtx4& _mtx);
    [[nodiscard]] Obb Transformed(const Mtx4& _mtx) const;

    [[nodiscard]] Aabb         ToAabb() const;
    [[nodiscard]] Vec3         GetCenter() const;
    [[nodiscard]] Vec3         GetExtents() const;
    [[nodiscard]] float        GetWidth() const;
    [[nodiscard]] float        GetHeight() const;
    [[nodiscard]] float        GetDepth() const;
    [[nodiscard]] CornerPoints GetCornerPoints() const;

    [[nodiscard]] bool          Contains(Vec3 _point) const;
    [[nodiscard]] RayCastResult Intersects(const Ray& _ray) const;

    Mtx4 mtx = Numbers<Mtx4>::k_identity;
};

enum class ePlaneIntersect
{
    Outside,
    Intersects,
    Inside
};

// ax + by + cz + d = 0
// d = -np
//
// normalized plane
// ax + by + cz + d = 0
// n = (a, b, c) is a unit vector
// d = -np
// -d의 의미 = 원점 ~ 평면까지의 최단거리.
// d < 0 : 원점이 평면의 법선 방향(외부)에 있음.
// d > 0 : 원점이 평면의 법선 반대 방향(내부)에 있음.
// dot(n, p) < d : 점 p가 평면의 법선 방향(외부)에 있음.
// dot(n, p) > d : 점 p가 평면의 법선 반대 방향(내부)에 있음.
struct Plane
{
    Plane() = default;
    explicit Plane(float _a, float _b, float _c, float _d, bool _bNormalize = true);
    explicit Plane(Vec4 _vec, bool _bNormalize = true);
    explicit Plane(Vec3 _normal, float _d, bool _bNormalize = true);
    explicit Plane(Vec3 _p1, Vec3 _p2, Vec3 _p3, bool _bNormalize = true);
    explicit Plane(Vec3 _point, Vec3 _normal, bool _bNormalize = true);

    [[nodiscard]] static Vec3 IntersectOf3Planes(const Plane& p1, const Plane& p2, const Plane& p3);
    [[nodiscard]] Vec3        GetNormal() const;
    [[nodiscard]] float       GetD() const;

    void                Normalize();
    [[nodiscard]] Plane Normalized() const;
    [[nodiscard]] bool  IsNormalized() const;

    void                Transform(const Mtx4& _mtx, bool _normalize = true);
    [[nodiscard]] Plane Transformed(const Mtx4& _mtx, bool _normalize = true) const;

    [[nodiscard]] float               DistanceToPoint(Vec3 _point) const;
    [[nodiscard]] RayCastResult       Intersects(const Ray& _ray) const;
    [[nodiscard]] ePlaneIntersect Intersects(const Aabb& _box) const;

    Vec3  normal = Numbers<Vec3>::k_zero;
    float d      = 0.f;
};

struct Frustum
{
    Frustum() = default;
    explicit Frustum(Plane _left, Plane _right, Plane _bottom, Plane _top, Plane _near, Plane _far, bool _bNormalized = true);
    explicit Frustum(const Mtx4& _mtx, bool _bNormalized = true);

    void                  Transform(const Mtx4& _mtx, bool _bNormalize = true);
    [[nodiscard]] Frustum Transformed(const Mtx4& _mtx, bool _bNormalize = true) const;

    void                  Normalize();
    [[nodiscard]] Frustum Normalized() const;
    [[nodiscard]] bool    IsNormalized() const;

    [[nodiscard]] Vec3         GetCenter() const;
    [[nodiscard]] CornerPoints GetCornerPoints() const;

    [[nodiscard]] ePlaneIntersect Intersects(const Aabb& _box) const;
    [[nodiscard]] RayCastResult       Intersects(const Ray& _ray) const;

    EnumArray<eFrustumFace, Plane> planes = {};
};

struct Ray
{
    Ray() = default;
    Ray(Vec3 _origin, Vec3 _direction, bool _bNormalizeDir = true);

    void               NormalizeDirection();
    [[nodiscard]] Ray  NormalizedDirection() const;
    [[nodiscard]] bool IsDirectionNormalized() const;

    Vec3 origin    = Numbers<Vec3>::k_zero;
    Vec3 direction = Numbers<Vec3>::k_zero;
};

}   // namespace Jam
