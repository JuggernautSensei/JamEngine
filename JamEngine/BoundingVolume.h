#pragma once
#include "EnumArray.h"

namespace Jam
{

struct Ray;

enum class eRegion
{
    NearLeftTop = 0,
    NearRightTop,
    NearLeftBottom,
    NearRightBottom,
    FarLeftTop,
    FarRightTop,
    FarLeftBottom,
    FarRightBottom,
};
using CornerPoints = EnumArray<eRegion, Vector3>;

struct RayCastResult
{
    bool  bHit     = false;
    float distance = 0.f;
};

struct Aabb
{
    Aabb() = default;
    explicit Aabb(Vector3 _min, Vector3 _max);

    [[nodiscard]] static Aabb FromCenterExtents(Vector3 _center, Vector3 _ext);

    void               Transform(const Matrix4x4& _mtx);
    [[nodiscard]] Aabb Transformed(const Matrix4x4& _mtx) const;

    void Merge(const Aabb& _aabb);
    void Merge(Vector3 _p);

    [[nodiscard]] Vector3         GetCenter() const;
    [[nodiscard]] Vector3         GetExtents() const;
    [[nodiscard]] float        GetWidth() const;
    [[nodiscard]] float        GetHeight() const;
    [[nodiscard]] float        GetDepth() const;
    [[nodiscard]] CornerPoints GetCornerPoints() const;

    [[nodiscard]] bool          Contain(Vector3 _point) const;
    [[nodiscard]] bool          Contain(const Aabb& _other) const;
    [[nodiscard]] bool          Intersect(const Aabb& _other) const;
    [[nodiscard]] RayCastResult Intersect(const Ray& _ray) const;

    Vector3 min = Vector3 { Limits<float>::k_max };
    Vector3 max = Vector3 { -Limits<float>::k_max };
};

struct Obb
{
    Obb() = default;
    explicit Obb(const Matrix4x4& _mtx);

    [[nodiscard]] static Obb FromAabb(const Aabb& _aabb);

    void              Transform(const Matrix4x4& _mtx);
    [[nodiscard]] Obb Transformed(const Matrix4x4& _mtx) const;

    [[nodiscard]] Aabb         ToAabb() const;
    [[nodiscard]] Vector3         GetCenter() const;
    [[nodiscard]] Vector3         GetExtents() const;
    [[nodiscard]] float        GetWidth() const;
    [[nodiscard]] float        GetHeight() const;
    [[nodiscard]] float        GetDepth() const;
    [[nodiscard]] CornerPoints GetCornerPoints() const;

    [[nodiscard]] bool          Contain(Vector3 _point) const;
    [[nodiscard]] RayCastResult Intersect(const Ray& _ray) const;

    Matrix4x4 mtx = Mtx4::k_identity;
};

enum class ePlaneIntersectType
{
    Outside,
    Intersect,
    Inside
};

struct Plane
{
    Plane() = default;
    explicit Plane(float _a, float _b, float _c, float _d);
    explicit Plane(Vector4 _vec);
    explicit Plane(Vector3 _normal, float _d);
    explicit Plane(Vector3 _p1, Vector3 _p2, Vector3 _p3);
    explicit Plane(Vector3 _point, Vector3 _normal);

    [[nodiscard]] static Vector3 IntersectOf3Planes(const Plane& p1, const Plane& p2, const Plane& p3);
    [[nodiscard]] Vector3        GetNormal() const;
    [[nodiscard]] float       GetD() const;

    void                Normalize();
    [[nodiscard]] Plane Normalized() const;

    void                Transform(const Matrix4x4& _mtx);
    [[nodiscard]] Plane Transformed(const Matrix4x4& _mtx) const;

    [[nodiscard]] float         DistanceToPoint(Vector3 _point) const;
    [[nodiscard]] RayCastResult Intersect(const Ray& _ray) const;

    Vector4 vec = Vec4::k_zero;
};

struct Frustum
{
    Frustum() = default;
    explicit Frustum(Plane _near, Plane _far, Plane _left, Plane _right, Plane _top, Plane _bottom);
    explicit Frustum(const Matrix4x4& _mtx);

    void                  Transform(const Matrix4x4& _mtx);
    [[nodiscard]] Frustum Transformed(const Matrix4x4& _mtx) const;

    [[nodiscard]] Frustum      SplitFrustum(float _startDist, float _endDist) const;
    [[nodiscard]] Vector3         GetCenter() const;
    [[nodiscard]] CornerPoints GetCornerPoints() const;

    [[nodiscard]] ePlaneIntersectType Intersect(const Aabb& _box) const;
    [[nodiscard]] RayCastResult       Intersect(const Ray& _ray) const;

    union
    {
        struct
        {
            Plane nearPlane;
            Plane farPlane;
            Plane leftPlane;
            Plane rightPlane;
            Plane topPlane;
            Plane bottomPlane;
        };

        Array<Plane, 6> planes = {};
    };
};

struct Ray
{
    Vector3 origin    = Vec3::k_zero;
    Vector3 direction = Vec3::k_zero;
};

}   // namespace Jam
