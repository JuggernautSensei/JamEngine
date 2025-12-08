#pragma once

namespace jam
{

struct Ray;

struct Corners
{
    union
    {
        struct
        {
            Vec3 nearLeftTop;
            Vec3 nearRightTop;
            Vec3 nearRightBottom;
            Vec3 nearLeftBottom;
            Vec3 farLeftTop;
            Vec3 farRightTop;
            Vec3 farRightBottom;
            Vec3 farLeftBottom;
        };
        std::array<Vec3, 8> points = {};
    };
};

struct RayIntersectResult
{
    bool  bHit     = false;
    float distance = 0.f;
};

struct Aabb
{
    [[nodiscard]] static Aabb FromCenterExtents(Vec3 _center, Vec3 _extents);
    static Aabb               Transform(const Aabb& _aabb, const Mat4& _mtx);

    void Transform(const Mat4& _mtx);
    void Merge(const Aabb& _aabb);
    void Merge(Vec3 _p);

    [[nodiscard]] Vec3    Center() const;
    [[nodiscard]] Vec3    Extents() const;
    [[nodiscard]] float   Width() const;
    [[nodiscard]] float   Height() const;
    [[nodiscard]] float   Depth() const;
    [[nodiscard]] Corners GetCorners() const;

    [[nodiscard]] bool               Contains(Vec3 _point) const;
    [[nodiscard]] bool               Contains(const Aabb& _other) const;
    [[nodiscard]] bool               Intersects(const Aabb& _other) const;
    [[nodiscard]] RayIntersectResult Intersects(const Ray& _ray) const;

    Vec3 min = Vec3(k_floatMax);
    Vec3 max = Vec3(k_floatMin);
};

struct Obb
{
    [[nodiscard]] static Obb FromAabb(const Aabb& _aabb);

    void               Transform(const Mat4& _mtx);
    [[nodiscard]] Aabb ToAABB() const;

    [[nodiscard]] Vec3    Center() const;
    [[nodiscard]] Vec3    Extents() const;
    [[nodiscard]] float   Width() const;
    [[nodiscard]] float   Height() const;
    [[nodiscard]] float   Depth() const;
    [[nodiscard]] Corners GetCorners() const;

    [[nodiscard]] bool               Contains(Vec3 _point) const;
    [[nodiscard]] RayIntersectResult Intersects(const Ray& _ray) const;

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

    [[nodiscard]] Vec3  Normal() const;
    [[nodiscard]] float D() const;

    void Normalize();
    void Transform(const Mat4& _mtx);

    [[nodiscard]] float              DistanceToPoint(Vec3 _point) const;
    [[nodiscard]] RayIntersectResult Intersects(const Ray& _ray) const;

    Vec4 vec = Vec4::Zero;
};

struct Frustum
{
    Frustum() = default;

    [[nodiscard]] static Frustum FromMatrix(const Mat4& _mtx);

    [[nodiscard]] Frustum SubFrustum(float _startDist, float _endDist) const;
    [[nodiscard]] Frustum Transform(const Mat4& _mtx) const;
    [[nodiscard]] Vec3    Center() const;
    [[nodiscard]] Corners GetCorners() const;

    [[nodiscard]] ePlaneIntersectType Intersects(const Aabb& _box) const;
    [[nodiscard]] RayIntersectResult  Intersects(const Ray& _ray) const;

    [[nodiscard]] Vec3  LookTo() const;
    [[nodiscard]] float Near() const;
    [[nodiscard]] float Far() const;

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
