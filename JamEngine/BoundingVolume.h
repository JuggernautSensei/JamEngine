#pragma once

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

struct CornerPoints
{
    [[nodiscard]] Vec3& At(
        const eRegion region)
    {
        return points[UnderlyingFrom(region)];
    }

    [[nodiscard]] const Vec3& At(
        const eRegion region) const
    {
        return points[UnderlyingFrom(region)];
    }

    [[nodiscard]] Vec3& operator[](
        const eRegion region)
    {
        return points[UnderlyingFrom(region)];
    }

    [[nodiscard]] const Vec3& operator[](
        const eRegion region) const
    {
        return points[UnderlyingFrom(region)];
    }

    Array<Vec3, CountOf<eRegion>()> points = {};
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

    [[nodiscard]] bool          Contain(Vec3 _point) const;
    [[nodiscard]] bool          Contain(const Aabb& _other) const;
    [[nodiscard]] bool          Intersect(const Aabb& _other) const;
    [[nodiscard]] RayCastResult Intersect(const Ray& _ray) const;

    Vec3 min = Vec3 { Limits<float>::k_max };
    Vec3 max = Vec3 { -Limits<float>::k_max };
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

    [[nodiscard]] bool          Contain(Vec3 _point) const;
    [[nodiscard]] RayCastResult Intersect(const Ray& _ray) const;

    Mtx4 mtx = mtx4::k_identity;
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
    explicit Plane(Vec4 _vec);
    explicit Plane(Vec3 _normal, float _d);
    explicit Plane(Vec3 _p1, Vec3 _p2, Vec3 _p3);
    explicit Plane(Vec3 _point, Vec3 _normal);

    [[nodiscard]] static Vec3 IntersectOf3Planes(const Plane& p1, const Plane& p2, const Plane& p3);
    [[nodiscard]] Vec3        GetNormal() const;
    [[nodiscard]] float       GetD() const;

    void                Normalize();
    [[nodiscard]] Plane Normalized() const;

    void                Transform(const Mtx4& _mtx);
    [[nodiscard]] Plane Transformed(const Mtx4& _mtx) const;

    [[nodiscard]] float         DistanceToPoint(Vec3 _point) const;
    [[nodiscard]] RayCastResult Intersect(const Ray& _ray) const;

    Vec4 vec = vec4::k_zero;
};

struct Frustum
{
    Frustum() = default;
    explicit Frustum(Plane _near, Plane _far, Plane _left, Plane _right, Plane _top, Plane _bottom);
    explicit Frustum(const Mtx4& _mtx);

    void                  Transform(const Mtx4& _mtx);
    [[nodiscard]] Frustum Transformed(const Mtx4& _mtx) const;

    [[nodiscard]] Frustum      SplitFrustum(float _startDist, float _endDist) const;
    [[nodiscard]] Vec3         GetCenter() const;
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
    Vec3 origin    = vec3::k_zero;
    Vec3 direction = vec3::k_zero;
};

}   // namespace Jam
