#pragma once

namespace jam
{

struct AABB
{
    JAM_NODISCARD static AABB Transform(const AABB& _box, const Mat4& _mat);
    JAM_NODISCARD Vec3        Center() const;
    JAM_NODISCARD Vec3        Extents() const;
    JAM_NODISCARD bool        Intersects(const AABB& _other) const;
    JAM_NODISCARD bool        Contains(const Vec3& _point) const;
    static AABB               Merge(const AABB& _a, const AABB& _b);

    Vec3 min = Vec3::Zero;
    Vec3 max = Vec3::Zero;
};

enum class ePlaneIntersactType
{
    Outside,
    Intersect,
    Inside
};

struct Plane
{
    Plane() = default;
    Plane(const Vec3& _normal, float _d);
    Plane(const Vec3& _p1, const Vec3& _p2, const Vec3& _p3);
    Plane(const Vec3& _point, const Vec3& _normal);

    JAM_NODISCARD static Plane Transform(const Plane& _plane, const Mat4& _mat);
    JAM_NODISCARD static Vec3  IntersectOf3Planes(const Plane& p1, const Plane& p2, const Plane& p3);

    void                Normalize();
    JAM_NODISCARD float DistanceToPoint(const Vec3& _point) const;

    union
    {
        struct
        {
            Vec3  normal;
            float d;
        };
        Vec4 vec = Vec4::Zero;
    };
};

struct FrustumCorners
{
    FrustumCorners() = default;

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

struct Frustum
{
    Frustum() = default;
    explicit Frustum(const Mat4& _projMat);

    JAM_NODISCARD static Frustum      Transform(const Frustum& _frustum, const Mat4& _mat);
    JAM_NODISCARD ePlaneIntersactType Intersects(const AABB& _box) const;
    JAM_NODISCARD FrustumCorners      GetCorners() const;

    JAM_NODISCARD Vec3 GetLookTo() const;
    JAM_NODISCARD float GetNear() const;
    JAM_NODISCARD float GetFar() const;

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
    Vec3 position = Vec3::Zero;
};

}   // namespace jam
