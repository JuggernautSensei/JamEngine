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

    [[nodiscard]] bool Contains(Vec3 _point) const;
    [[nodiscard]] bool Contains(const Aabb& _other) const;
    [[nodiscard]] bool Overlap(const Aabb& _other) const;
    [[nodiscard]] bool RayCast(const Ray& _ray, Hit* _pHitOrNull = nullptr) const;

    Vec3 center  = Vec3::kZero;
    Vec3 extents = Vec3::kOne;
};

struct Obb
{
    Obb() = default;
    explicit Obb(const Aabb& _aabb);

    [[nodiscard]] float   GetWidth() const;
    [[nodiscard]] float   GetHeight() const;
    [[nodiscard]] float   GetDepth() const;
    [[nodiscard]] Corners GetCorners() const;

    [[nodiscard]] Obb Transform(const Mtx4& _mtx) const;

    [[nodiscard]] bool Contains(Vec3 _point) const;
    [[nodiscard]] bool Contains(const Obb& _other) const;
    [[nodiscard]] bool Overlap(const Obb& _other) const;
    [[nodiscard]] bool RayCast(const Ray& _ray, Hit* _pHitOrNull = nullptr) const;

    Vec3 center   = Vec3::kZero;
    Vec3 extents  = Vec3::kOne;
    Quat rotation = Quat::kIdentity;
};

struct Plane
{
    Plane() {}
    Plane(float _a, float _b, float _c, float _d);
    explicit Plane(Vec4 _vec);
    Plane(Vec3 _normal, float _d);
    Plane(Vec3 _p1, Vec3 _p2, Vec3 _p3);
    Plane(Vec3 _point, Vec3 _normal);

    [[nodiscard]] static Vec3 IntersectOf3Planes(Plane _p0, Plane _p1, Plane _p2);
    [[nodiscard]] Plane       Transform(const Mtx4& _invTransMtx) const;

    void                Normalize();
    [[nodiscard]] Plane Normalized() const;

    // point - plane operations
    [[nodiscard]] float DistanceTo(Vec3 _point) const;
    [[nodiscard]] bool  IsFront(Vec3 _point) const;
    [[nodiscard]] bool  IsBehind(Vec3 _point) const;

    // aabb - plane operations
    [[nodiscard]] bool Overlap(const Aabb& _aabb) const;
    [[nodiscard]] bool IsFront(const Aabb& _aabb) const;
    [[nodiscard]] bool IsBehind(const Aabb& _aabb) const;

    // 충돌검사
    [[nodiscard]] bool RayCast(const Ray& _ray, bool _bDoubleSided, Hit* _pHitOrNull = nullptr) const;

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
        Vec4            vec;
        Array<float, 4> e = {};
    };
};

// 평면의 normal은 프러스텀의 안쪽을 향한다.
struct Frustum
{
    Frustum() = default;
    Frustum(Plane _left, Plane _right, Plane _bottom, Plane _top, Plane _near, Plane _far);
    explicit Frustum(const Mtx4& _mtx);

    [[nodiscard]] Frustum Transform(const Mtx4& _invTransMtx) const;

    void                  Normalize();
    [[nodiscard]] Frustum Normalized() const;

    [[nodiscard]] Vec3    CalcCenter() const;
    [[nodiscard]] Corners CalcCorners() const;

    [[nodiscard]] bool Overlap(const Aabb& _box) const;
    [[nodiscard]] bool RayCast(const Ray& _ray, bool _bDoubleSided, Hit* _pHitOrNull = nullptr) const;

    // 분할. 뷰 공간 프러스텀에서만 올바르게 동작함.
    void Split(Span<Frustum> _outFrustums, float _weight = 0.5f) const;

    union
    {
        struct
        {
            Plane leftp;
            Plane rightp;
            Plane bottomp;
            Plane topp;
            Plane nearp;
            Plane farp;
        };
        Array<Plane, 6> planes = {};
    };
};

struct Ray
{
    Ray() = default;
    Ray(Vec3 _origin, Vec3 _direction);
    Ray(float _x, float _y, const Mtx4& _invVP);   // NDC space -> world space

    [[nodiscard]] Vec3 GetPoint(float _t) const;

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