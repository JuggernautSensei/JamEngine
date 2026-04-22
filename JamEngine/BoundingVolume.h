// ReSharper disable CppClangTidyClangDiagnosticNestedAnonTypes
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

using Corners = EnumArray<eRegion, Vec3>;

enum class eFrustumFace
{
    Near,
    Far,
    Left,
    Right,
    Bottom,
    Top,
};

enum class eIntersect
{
    Outside,
    Overlab,
    Inside
};

struct Hit;

struct Aabb
{
    Aabb() = default;
    Aabb(Vec3 _center, Vec3 _extents);
    [[nodiscard]] static Aabb FromMinMax(Vec3 _min, Vec3 _max);

    [[nodiscard]] Aabb Transform(const Mtx4& _mtx) const;

    void Merge(const Aabb& _aabb);
    void Merge(Vec3 _p);

    [[nodiscard]] Vec3    GetMin() const;
    [[nodiscard]] Vec3    GetMax() const;
    [[nodiscard]] float   GetWidth() const;
    [[nodiscard]] float   GetHeight() const;
    [[nodiscard]] float   GetDepth() const;
    [[nodiscard]] Corners CalcCorners() const;

    [[nodiscard]] bool       Contains(Vec3 _point) const;
    [[nodiscard]] bool       Contains(const Aabb& _other) const;
    [[nodiscard]] bool       Overlap(const Aabb& _other) const;
    [[nodiscard]] eIntersect Intersects(const Aabb& _other) const;
    [[nodiscard]] bool       RayCast(const Ray& _ray, Hit* _pHitOrNull = nullptr) const;

    Vec3 center  = Vec3::kZero;
    Vec3 extents = Vec3::kOne;
};

struct alignas(16) Obb
{
    Obb() = default;
    explicit Obb(const Aabb& _aabb);

    [[nodiscard]] Obb Transform(const Mtx4& _mtx) const;

    [[nodiscard]] float   GetWidth() const;
    [[nodiscard]] float   GetHeight() const;
    [[nodiscard]] float   GetDepth() const;
    [[nodiscard]] Corners GetCorners() const;

    [[nodiscard]] bool Contains(Vec3 _point) const;
    [[nodiscard]] bool Contains(const Obb& _other) const;
    [[nodiscard]] bool Overlab(const Obb& _other) const;
    [[nodiscard]] bool RayCast(const Ray& _ray, Hit* _pHitOrNull = nullptr) const;

    Vec3 center   = Vec3::kZero;
    Vec3 extents  = Vec3::kOne;
    Quat rotation = Quat::kIdentity;
};

struct Plane
{
    Plane() {}
    Plane(float _a, float _b, float _c, float _d, bool _bNormalize = true);
    explicit Plane(Vec4 _vec, bool _bNormalize = true);
    Plane(Vec3 _normal, float _d, bool _bNormalize = true);
    Plane(Vec3 _p1, Vec3 _p2, Vec3 _p3, bool _bNormalize = true);
    Plane(Vec3 _point, Vec3 _normal, bool _bNormalize = true);

    [[nodiscard]] Plane Transform(const Mtx4& _invTransMtx, bool _bNormalize = true) const;

    void                Normalize();
    [[nodiscard]] Plane Normalized() const;

    [[nodiscard]] static Vec3 IntersectOf3Planes(const Plane& p1, const Plane& p2, const Plane& p3);
    [[nodiscard]] float       DistanceToPoint(Vec3 _point) const;

    [[nodiscard]] bool       Overlap(const Aabb& _box) const;
    [[nodiscard]] eIntersect Intersects(const Aabb& _box) const;
    [[nodiscard]] bool       RayCast(const Ray& _ray, bool _bDoubleSided, Hit* _pHitOrNull = nullptr) const;

    union
    {
        struct
        {
            float x, y, z, w;
        };
        struct
        {
            Vec3  normal;
            float d;
        };
        Vec4  vec;
        float e[4] = {};
    };
};

struct Frustum
{
    Frustum() = default;
    Frustum(Plane _left, Plane _right, Plane _bottom, Plane _top, Plane _near, Plane _far, bool _bNormalized = true);
    explicit Frustum(const Mtx4& _mtx, bool _bNormalized = true);

    [[nodiscard]] Frustum Transform(const Mtx4& _invTransMtx, bool _bNormalize = true) const;

    void                  Normalize();
    [[nodiscard]] Frustum Normalized() const;

    [[nodiscard]] Vec3    CalcCenter() const;
    [[nodiscard]] Corners CalcCorners() const;

    [[nodiscard]] bool       Overlap(const Aabb& _box) const;
    [[nodiscard]] eIntersect Intersects(const Aabb& _box) const;
    [[nodiscard]] bool       RayCast(const Ray& _ray, bool _bDoubleSided, Hit* _pHitOrNull = nullptr) const;

    EnumArray<eFrustumFace, Plane> planes = {};
};

struct Ray
{
    Ray() = default;
    Ray(Vec3 _origin, Vec3 _direction, bool _bNormalizeDir = true);

    void              Normalize();
    [[nodiscard]] Ray Normalized() const;

    Vec3 pos = Vec3::kZero;
    Vec3 dir = Vec3::kZero;
};

struct Hit
{
    Vec3  normal = Vec3::kZero;
    Vec3  point  = Vec3::kZero;
    float dist   = 0;
};

}   // namespace Jam