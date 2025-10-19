#pragma once

namespace jam
{

struct AABB
{
    JAM_NODISCARD static AABB FromCenterExtents(const Vec3& _center, const Vec3& _extents);
    JAM_NODISCARD static AABB Transform(const AABB& _box, const Mat4& _mat);
    JAM_NODISCARD static AABB Merge(const AABB& _a, const AABB& _b);
    JAM_NODISCARD static AABB Merge(const AABB& _a, const Vec3& _pointA);

    JAM_NODISCARD Vec3 Center() const;
    JAM_NODISCARD Vec3 Extents() const;
    JAM_NODISCARD float Width() const;
    JAM_NODISCARD float Height() const;
    JAM_NODISCARD float Depth() const;
    JAM_NODISCARD bool Intersects(const AABB& _other) const;
    JAM_NODISCARD bool Contains(const Vec3& _point) const;

    Vec3 min = Vec3(FloatLimits::max());
    Vec3 max = Vec3(FloatLimits::lowest());
};

struct OBB
{
    JAM_NODISCARD static OBB  Transform(const OBB& _obb, const Mat4& _mat);
    JAM_NODISCARD static AABB ToAABB(const OBB& _obb);

    JAM_NODISCARD Vec3 Center() const;
    JAM_NODISCARD Vec3 Extents();

    Mat4 mtx = Mat4::Identity;
};

struct Sphere
{
    Vec3  center = Vec3::Zero;
    float radius = 0.f;
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
    explicit Frustum(const Mat4& _viewProj);

    JAM_NODISCARD static Frustum      CreateSubFrustum(const Frustum& _frustum, float _startWeight, float _endWeight);
    JAM_NODISCARD static Frustum      Transform(const Frustum& _frustum, const Mat4& _mat);
    JAM_NODISCARD ePlaneIntersectType Intersects(const AABB& _box) const;
    JAM_NODISCARD bool                Intersects(const Sphere& _sphere) const;
    JAM_NODISCARD FrustumCorners      GetCorners() const;
    JAM_NODISCARD Vec3                GetCenter() const;

    JAM_NODISCARD Vec3  ViewLookTo() const;
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
