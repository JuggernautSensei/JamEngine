#pragma once

namespace jam
{

struct Ray;

enum class eRegion
{
    NearLeftTop,
    NearRightTop,
    NearLeftBottom,
    NearRightBottom,
    FarLeftTop,
    FarRightTop,
    FarLeftBottom,
    FarRightBottom
};

struct CornerPoints
{
    [[nodiscard]] Vec3& At(const eRegion region)
    {
        return points[IntFrom(region)];
    }

    [[nodiscard]] const Vec3& At(const eRegion region) const
    {
        return points[IntFrom(region)];
    }

    [[nodiscard]] Vec3& operator[](const eRegion region)
    {
        return points[IntFrom(region)];
    }

    [[nodiscard]] const Vec3& operator[](const eRegion region) const
    {
        return points[IntFrom(region)];
    }

    Array<Vec3, NumEnums<eRegion>()> points = {};
};

struct RayCastResult
{
    bool  bHit     = false;
    float distance = 0.f;
};

struct Aabb
{
    [[nodiscard]] static Aabb FromCenterExtents(Vec3 _center, Vec3 _ext);

    void               Transform(const Mat4& _mtx);
    [[nodiscard]] Aabb Transformed(const Mat4& _mtx) const;

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

    Vec3 min = Vec3(k_floatMax);
    Vec3 max = Vec3(k_floatMin);
};

struct Obb
{
    [[nodiscard]] static Obb FromAabb(const Aabb& _aabb);

    void              Transform(const Mat4& _mtx);
    [[nodiscard]] Obb Transformed(const Mat4& _mtx) const;

    [[nodiscard]] Aabb         ToAabb() const;
    [[nodiscard]] Vec3         GetCenter() const;
    [[nodiscard]] Vec3         GetExtents() const;
    [[nodiscard]] float        GetWidth() const;
    [[nodiscard]] float        GetHeight() const;
    [[nodiscard]] float        GetDepth() const;
    [[nodiscard]] CornerPoints GetCornerPoints() const;

    [[nodiscard]] bool          Contain(Vec3 _point) const;
    [[nodiscard]] RayCastResult Intersect(const Ray& _ray) const;

    Mat4 mtx = Mat4::Identity;
};

enum class ePlaneIntersectType
{
    Outside,
    Intersect,
    Inside
};

struct Plane
{
    [[nodiscard]] static Plane FromNormalD(Vec3 _normal, float _d);
    [[nodiscard]] static Plane FromPoints(Vec3 _p1, Vec3 _p2, Vec3 _p3);
    [[nodiscard]] static Plane FromPointNormal(Vec3 _point, Vec3 _normal);
    [[nodiscard]] static Vec3  IntersectOf3Planes(const Plane& p1, const Plane& p2, const Plane& p3);

    [[nodiscard]] Vec3  GetNormal() const;
    [[nodiscard]] float GetD() const;

    void                Normalize();
    [[nodiscard]] Plane Normalized() const;

    void                Transform(const Mat4& _mtx);
    [[nodiscard]] Plane Transformed(const Mat4& _mtx) const;

    [[nodiscard]] float         DistanceToPoint(Vec3 _point) const;
    [[nodiscard]] RayCastResult Intersect(const Ray& _ray) const;

    Vec4 vec = Vec4::Zero;
};

struct Frustum
{
    Frustum() = default;

    [[nodiscard]] static Frustum FromMatrix(const Mat4& _mtx);

    void                  Transform(const Mat4& _mtx);
    [[nodiscard]] Frustum Transformed(const Mat4& _mtx) const;

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
        Plane planes[6] = {};
    };
};

struct Ray
{
    Vec3 origin;
    Vec3 direction;
};

}   // namespace jam
