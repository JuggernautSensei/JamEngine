#include "pch.h"
#include <gtest/gtest.h>

namespace Jam
{
namespace
{

    ///////////////////////////////////////////////////////////////////////////
    // Region Helper Tests
    ///////////////////////////////////////////////////////////////////////////

    TEST(BoundingVolumeTest, Region_AllCombinations)
    {
        EXPECT_EQ(CalcRegion(false, false, false), eRegion::LeftBottomNear);
        EXPECT_EQ(CalcRegion(false, false, true), eRegion::LeftBottomFar);
        EXPECT_EQ(CalcRegion(false, true, false), eRegion::LeftTopNear);
        EXPECT_EQ(CalcRegion(false, true, true), eRegion::LeftTopFar);
        EXPECT_EQ(CalcRegion(true, false, false), eRegion::RightBottomNear);
        EXPECT_EQ(CalcRegion(true, false, true), eRegion::RightBottomFar);
        EXPECT_EQ(CalcRegion(true, true, false), eRegion::RightTopNear);
        EXPECT_EQ(CalcRegion(true, true, true), eRegion::RightTopFar);
    }

    TEST(BoundingVolumeTest, Region_IsRightIsolated)
    {
        for (bool top: { false, true })
        {
            for (bool f: { false, true })
            {
                EXPECT_TRUE(IsRight(CalcRegion(true, top, f)));
                EXPECT_FALSE(IsRight(CalcRegion(false, top, f)));
            }
        }
    }

    TEST(BoundingVolumeTest, Region_IsTopIsolated)
    {
        for (bool right: { false, true })
        {
            for (bool f: { false, true })
            {
                EXPECT_TRUE(IsTop(CalcRegion(right, true, f)));
                EXPECT_FALSE(IsTop(CalcRegion(right, false, f)));
            }
        }
    }

    TEST(BoundingVolumeTest, Region_IsFarIsolated)
    {
        for (bool right: { false, true })
        {
            for (bool top: { false, true })
            {
                EXPECT_TRUE(IsFar(CalcRegion(right, top, true)));
                EXPECT_FALSE(IsFar(CalcRegion(right, top, false)));
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // AABB Construction & Accessors
    ///////////////////////////////////////////////////////////////////////////

    TEST(BoundingVolumeTest, Aabb_FromBounds)
    {
        // ✅ center/extents 기반 생성
        const Aabb aabb = Aabb(Vec3 { 1.f, 2.f, 3.f }, Vec3 { 1.f, 1.f, 1.f });
        EXPECT_FLOAT_EQ(aabb.center.x, 1.f);
        EXPECT_FLOAT_EQ(aabb.center.y, 2.f);
        EXPECT_FLOAT_EQ(aabb.center.z, 3.f);
        EXPECT_FLOAT_EQ(aabb.extents.x, 1.f);
        EXPECT_FLOAT_EQ(aabb.extents.y, 1.f);
        EXPECT_FLOAT_EQ(aabb.extents.z, 1.f);
    }

    TEST(BoundingVolumeTest, Aabb_FromMinMax)
    {
        // ✅ min/max 기반 생성 (호환성)
        const Aabb aabb = Aabb::FromMinMax(Vec3 { 0.f, 1.f, 2.f }, Vec3 { 2.f, 3.f, 4.f });
        EXPECT_FLOAT_EQ(aabb.center.x, 1.f);
        EXPECT_FLOAT_EQ(aabb.center.y, 2.f);
        EXPECT_FLOAT_EQ(aabb.center.z, 3.f);
        EXPECT_FLOAT_EQ(aabb.extents.x, 1.f);
        EXPECT_FLOAT_EQ(aabb.extents.y, 1.f);
        EXPECT_FLOAT_EQ(aabb.extents.z, 1.f);
    }

    TEST(BoundingVolumeTest, Aabb_GetMinMax)
    {
        // ✅ min/max 조회
        const Aabb aabb = Aabb(Vec3 { 3.f, 0.f, -5.f }, Vec3 { 2.f, 1.f, 3.f });
        const Vec3 min  = aabb.GetMin();
        const Vec3 max  = aabb.GetMax();

        EXPECT_FLOAT_EQ(min.x, 1.f);
        EXPECT_FLOAT_EQ(min.y, -1.f);
        EXPECT_FLOAT_EQ(min.z, -8.f);
        EXPECT_FLOAT_EQ(max.x, 5.f);
        EXPECT_FLOAT_EQ(max.y, 1.f);
        EXPECT_FLOAT_EQ(max.z, -2.f);
    }

    TEST(BoundingVolumeTest, Aabb_CenterAndExtents)
    {
        const Aabb aabb = Aabb(Vec3 { 3.f, 0.f, -5.f }, Vec3 { 2.f, 1.f, 3.f });
        const Vec3 c    = aabb.center;
        const Vec3 e    = aabb.extents;

        EXPECT_FLOAT_EQ(c.x, 3.f);
        EXPECT_FLOAT_EQ(c.y, 0.f);
        EXPECT_FLOAT_EQ(c.z, -5.f);
        EXPECT_FLOAT_EQ(e.x, 2.f);
        EXPECT_FLOAT_EQ(e.y, 1.f);
        EXPECT_FLOAT_EQ(e.z, 3.f);
    }

    TEST(BoundingVolumeTest, Aabb_WidthHeightDepth)
    {
        const Aabb aabb = Aabb(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 3.f, 5.f, 7.f });
        EXPECT_FLOAT_EQ(aabb.GetWidth(), 6.f);
        EXPECT_FLOAT_EQ(aabb.GetHeight(), 10.f);
        EXPECT_FLOAT_EQ(aabb.GetDepth(), 14.f);
    }

    TEST(BoundingVolumeTest, Aabb_CornerPoints_Count)
    {
        const Aabb         aabb    = Aabb(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 1.f, 1.f, 1.f });
        const Corners corners = aabb.CalcCorners();
        EXPECT_EQ(corners.Size(), 8u);
    }

    TEST(BoundingVolumeTest, Aabb_CornerPoints_Values)
    {
        const Aabb         aabb    = Aabb(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 1.f, 1.f, 1.f });
        const Corners corners = aabb.CalcCorners();

        EXPECT_FLOAT_EQ(corners[eRegion::LeftBottomNear].x, -1.f);
        EXPECT_FLOAT_EQ(corners[eRegion::LeftBottomNear].y, -1.f);
        EXPECT_FLOAT_EQ(corners[eRegion::LeftBottomNear].z, -1.f);
        EXPECT_FLOAT_EQ(corners[eRegion::RightTopFar].x, 1.f);
        EXPECT_FLOAT_EQ(corners[eRegion::RightTopFar].y, 1.f);
        EXPECT_FLOAT_EQ(corners[eRegion::RightTopFar].z, 1.f);
    }

    ///////////////////////////////////////////////////////////////////////////
    // AABB Contains
    ///////////////////////////////////////////////////////////////////////////

    TEST(BoundingVolumeTest, Aabb_ContainsPoint_OnBoundary)
    {
        const Aabb aabb = Aabb(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 1.f, 1.f, 1.f });
        EXPECT_TRUE(aabb.Contains(Vec3 { 1.f, 0.f, 0.f }));
        EXPECT_TRUE(aabb.Contains(Vec3 { -1.f, 0.f, 0.f }));
        EXPECT_TRUE(aabb.Contains(Vec3 { 0.f, 1.f, 0.f }));
        EXPECT_TRUE(aabb.Contains(Vec3 { 0.f, -1.f, 0.f }));
    }

    TEST(BoundingVolumeTest, Aabb_ContainsPoint_JustOutside)
    {
        const Aabb aabb = Aabb(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 1.f, 1.f, 1.f });
        EXPECT_FALSE(aabb.Contains(Vec3 { 1.001f, 0.f, 0.f }));
        EXPECT_FALSE(aabb.Contains(Vec3 { 0.f, 0.f, 1.001f }));
        EXPECT_FALSE(aabb.Contains(Vec3 { 0.f, -1.001f, 0.f }));
    }

    TEST(BoundingVolumeTest, Aabb_ContainsAabb_IdenticalIsContained)
    {
        const Aabb aabb = Aabb(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 1.f, 1.f, 1.f });
        EXPECT_TRUE(aabb.Contains(aabb));
    }

    TEST(BoundingVolumeTest, Aabb_ContainsAabb_PartialOverlapNotContained)
    {
        const Aabb a = Aabb(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 1.f, 1.f, 1.f });
        const Aabb b = Aabb(Vec3 { 0.5f, 0.f, 0.f }, Vec3 { 1.f, 1.f, 1.f });
        EXPECT_FALSE(a.Contains(b));
        EXPECT_FALSE(b.Contains(a));
    }

    ///////////////////////////////////////////////////////////////////////////
    // AABB Overlap / Intersects
    ///////////////////////////////////////////////////////////////////////////

    TEST(BoundingVolumeTest, Aabb_Overlap_TouchingEdge)
    {
        const Aabb a = Aabb(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 1.f, 1.f, 1.f });
        const Aabb b = Aabb(Vec3 { 2.f, 0.f, 0.f }, Vec3 { 1.f, 1.f, 1.f });
        EXPECT_TRUE(a.Overlap(b));
    }

    TEST(BoundingVolumeTest, Aabb_Intersects_InsideResult)
    {
        const Aabb a = Aabb(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 2.f, 2.f, 2.f });
        const Aabb b = Aabb(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 0.5f, 0.5f, 0.5f });
        EXPECT_EQ(a.Intersects(b), eIntersect::Inside);
    }

    TEST(BoundingVolumeTest, Aabb_Intersects_OverlapResult)
    {
        const Aabb a = Aabb(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 1.f, 1.f, 1.f });
        const Aabb b = Aabb(Vec3 { 0.5f, 0.f, 0.f }, Vec3 { 1.f, 1.f, 1.f });
        EXPECT_EQ(a.Intersects(b), eIntersect::Overlab);
    }

    TEST(BoundingVolumeTest, Aabb_Intersects_OutsideResult)
    {
        const Aabb a = Aabb(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 1.f, 1.f, 1.f });
        const Aabb b = Aabb(Vec3 { 10.f, 0.f, 0.f }, Vec3 { 1.f, 1.f, 1.f });
        EXPECT_EQ(a.Intersects(b), eIntersect::Outside);
    }

    ///////////////////////////////////////////////////////////////////////////
    // AABB Merge
    ///////////////////////////////////////////////////////////////////////////

    TEST(BoundingVolumeTest, Aabb_MergePoint_ExpandsCorrectly)
    {
        Aabb aabb = Aabb(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 1.f, 1.f, 1.f });
        aabb.Merge(Vec3 { 5.f, -3.f, 0.f });

        const Vec3 max = aabb.GetMax();
        const Vec3 min = aabb.GetMin();
        EXPECT_FLOAT_EQ(max.x, 5.f);
        EXPECT_FLOAT_EQ(min.y, -3.f);
        EXPECT_FLOAT_EQ(min.x, -1.f);
    }

    TEST(BoundingVolumeTest, Aabb_MergePoint_InsideNoChange)
    {
        Aabb       aabb   = Aabb(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 5.f, 5.f, 5.f });
        const Vec3 oldMin = aabb.GetMin();
        const Vec3 oldMax = aabb.GetMax();

        aabb.Merge(Vec3 { 1.f, 1.f, 1.f });
        EXPECT_FLOAT_EQ(aabb.GetMin().x, oldMin.x);
        EXPECT_FLOAT_EQ(aabb.GetMax().x, oldMax.x);
    }

    TEST(BoundingVolumeTest, Aabb_MergeAabb_TakesUnion)
    {
        Aabb       a = Aabb(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 1.f, 1.f, 1.f });
        const Aabb b = Aabb(Vec3 { 4.f, 4.f, 4.f }, Vec3 { 1.f, 1.f, 1.f });
        a.Merge(b);

        const Vec3 min = a.GetMin();
        const Vec3 max = a.GetMax();
        EXPECT_FLOAT_EQ(min.x, -1.f);
        EXPECT_FLOAT_EQ(max.x, 5.f);
        EXPECT_FLOAT_EQ(min.y, -1.f);
        EXPECT_FLOAT_EQ(max.z, 5.f);
    }

    ///////////////////////////////////////////////////////////////////////////
    // AABB Ray Intersection
    ///////////////////////////////////////////////////////////////////////////

    TEST(BoundingVolumeTest, Aabb_RayIntersect_HitFront)
    {
        const Aabb aabb = Aabb(Vec3 { 0.f, 0.f, 5.f }, Vec3 { 1.f, 1.f, 1.f });
        const Ray  ray(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 0.f, 0.f, 1.f });
        Hit        hit;

        ASSERT_TRUE(aabb.RayCast(ray, &hit));
        EXPECT_NEAR(hit.dist, 4.f, 1e-4f);
        EXPECT_FLOAT_EQ(hit.normal.z, -1.f);
        EXPECT_FLOAT_EQ(hit.normal.x, 0.f);
    }

    TEST(BoundingVolumeTest, Aabb_RayIntersect_MissCompletely)
    {
        const Aabb aabb = Aabb(Vec3 { 0.f, 0.f, 5.f }, Vec3 { 1.f, 1.f, 1.f });
        const Ray  ray(Vec3 { 5.f, 0.f, 0.f }, Vec3 { 0.f, 0.f, 1.f });

        EXPECT_FALSE(aabb.RayCast(ray, nullptr));
    }

    TEST(BoundingVolumeTest, Aabb_RayIntersect_RayBehindBox)
    {
        const Aabb aabb = Aabb(Vec3 { 0.f, 0.f, -5.f }, Vec3 { 1.f, 1.f, 1.f });
        const Ray  ray(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 0.f, 0.f, 1.f });

        EXPECT_FALSE(aabb.RayCast(ray, nullptr));
    }

    TEST(BoundingVolumeTest, Aabb_RayIntersect_RayInsideBox)
    {
        const Aabb aabb = Aabb(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 5.f, 5.f, 5.f });
        const Ray  ray(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 1.f, 0.f, 0.f });

        EXPECT_TRUE(aabb.RayCast(ray, nullptr));
    }

    TEST(BoundingVolumeTest, Aabb_RayIntersect_DiagonalHit)
    {
        const Aabb aabb = Aabb(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 1.f, 1.f, 1.f });
        const Vec3 dir  = Vec3 { 1.f, 1.f, 1.f };
        const Ray  ray(Vec3 { -3.f, -3.f, -3.f }, dir);
        Hit        hit;

        ASSERT_TRUE(aabb.RayCast(ray, &hit));
        EXPECT_NEAR(hit.point.x, -1.f, 0.01f);
        EXPECT_NEAR(hit.point.y, -1.f, 0.01f);
        EXPECT_NEAR(hit.point.z, -1.f, 0.01f);
    }

    TEST(BoundingVolumeTest, Aabb_RayIntersect_NullHitNocrash)
    {
        const Aabb aabb = Aabb(Vec3 { 0.f, 0.f, 5.f }, Vec3 { 1.f, 1.f, 1.f });
        const Ray  ray(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 0.f, 0.f, 1.f });

        EXPECT_TRUE(aabb.RayCast(ray, nullptr));
    }

    ///////////////////////////////////////////////////////////////////////////
    // AABB Transform
    ///////////////////////////////////////////////////////////////////////////

    TEST(BoundingVolumeTest, Aabb_Transform_Translation)
    {
        const Aabb aabb     = Aabb(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 1.f, 1.f, 1.f });
        const Mtx4 transMtx = Mtx4::CreateSRT(Vec3 { 1.f, 1.f, 1.f }, Quat::kIdentity, Vec3 { 5.f, 0.f, 0.f });
        const Aabb moved    = aabb.Transform(transMtx);

        EXPECT_NEAR(moved.center.x, 5.f, 1e-4f);
    }

    TEST(BoundingVolumeTest, Aabb_Transform_Scale)
    {
        const Aabb aabb     = Aabb(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 1.f, 1.f, 1.f });
        const Mtx4 scaleMtx = Mtx4::CreateSRT(Vec3 { 2.f, 2.f, 2.f }, Quat::kIdentity, Vec3::kZero);
        const Aabb scaled   = aabb.Transform(scaleMtx);

        EXPECT_NEAR(scaled.extents.x, 2.f, 1e-4f);
        EXPECT_NEAR(scaled.extents.y, 2.f, 1e-4f);
        EXPECT_NEAR(scaled.extents.z, 2.f, 1e-4f);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Plane
    ///////////////////////////////////////////////////////////////////////////

    TEST(BoundingVolumeTest, Plane_DistanceToPoint_PositiveHalfspace)
    {
        const Plane plane(Vec3 { 0.f, 1.f, 0.f }, 0.f);
        EXPECT_GT(plane.DistanceToPoint(Vec3 { 0.f, 1.f, 0.f }), 0.f);
        EXPECT_LT(plane.DistanceToPoint(Vec3 { 0.f, -1.f, 0.f }), 0.f);
        EXPECT_FLOAT_EQ(plane.DistanceToPoint(Vec3 { 0.f, 0.f, 0.f }), 0.f);
    }

    TEST(BoundingVolumeTest, Plane_IntersectsAabb_Inside)
    {
        const Plane plane(Vec3 { 0.f, 1.f, 0.f }, 0.f);
        const Aabb  box = Aabb(Vec3 { 0.f, 2.f, 0.f }, Vec3 { 1.f, 0.5f, 1.f });
        EXPECT_EQ(plane.Intersects(box), eIntersect::Inside);
    }

    TEST(BoundingVolumeTest, Plane_IntersectsAabb_Outside)
    {
        const Plane plane(Vec3 { 0.f, 1.f, 0.f }, 0.f);
        const Aabb  box = Aabb(Vec3 { 0.f, -2.f, 0.f }, Vec3 { 1.f, 0.5f, 1.f });
        EXPECT_EQ(plane.Intersects(box), eIntersect::Outside);
    }

    TEST(BoundingVolumeTest, Plane_IntersectsAabb_Straddles)
    {
        const Plane plane(Vec3 { 0.f, 1.f, 0.f }, 0.f);
        const Aabb  box = Aabb(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 1.f, 1.f, 1.f });
        EXPECT_EQ(plane.Intersects(box), eIntersect::Overlab);
    }

    TEST(BoundingVolumeTest, Plane_Normalize_UnitLength)
    {
        Plane plane(Vec3 { 3.f, 0.f, 4.f }, -5.f, false);
        plane.Normalize();

        const float len = plane.normal.Length();
        EXPECT_NEAR(len, 1.f, 1e-5f);
    }

    TEST(BoundingVolumeTest, Plane_IntersectsRay_FrontFacingHit)
    {
        const Plane plane(Vec3 { 0.f, 0.f, 1.f }, -5.f);
        const Ray   ray(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 0.f, 0.f, 1.f });
        Hit         hit;

        ASSERT_TRUE(plane.RayCast(ray, true, &hit));
        EXPECT_NEAR(hit.point.z, 5.f, 1e-4f);
    }

    TEST(BoundingVolumeTest, Plane_IntersectsRay_BackFacingMiss)
    {
        const Plane plane(Vec3 { 0.f, 0.f, 1.f }, -5.f);
        const Ray   ray(Vec3 { 0.f, 0.f, 10.f }, Vec3 { 0.f, 0.f, 1.f });

        EXPECT_FALSE(plane.RayCast(ray, false, nullptr));
    }

    TEST(BoundingVolumeTest, Plane_IntersectOf3Planes_Unique)
    {
        const Plane px(Vec3 { 1.f, 0.f, 0.f }, 0.f);
        const Plane py(Vec3 { 0.f, 1.f, 0.f }, 0.f);
        const Plane pz(Vec3 { 0.f, 0.f, 1.f }, 0.f);

        const Vec3 pt = Plane::IntersectOf3Planes(px, py, pz);
        EXPECT_NEAR(pt.x, 0.f, 1e-4f);
        EXPECT_NEAR(pt.y, 0.f, 1e-4f);
        EXPECT_NEAR(pt.z, 0.f, 1e-4f);
    }

    TEST(BoundingVolumeTest, Plane_IntersectOf3Planes_Offset)
    {
        const Plane px(Vec3 { 1.f, 0.f, 0.f }, -3.f);
        const Plane py(Vec3 { 0.f, 1.f, 0.f }, -7.f);
        const Plane pz(Vec3 { 0.f, 0.f, 1.f }, -2.f);

        const Vec3 pt = Plane::IntersectOf3Planes(px, py, pz);
        EXPECT_NEAR(pt.x, 3.f, 1e-4f);
        EXPECT_NEAR(pt.y, 7.f, 1e-4f);
        EXPECT_NEAR(pt.z, 2.f, 1e-4f);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Frustum
    ///////////////////////////////////////////////////////////////////////////

    TEST(BoundingVolumeTest, Frustum_IntersectsAabb_InsideAll)
    {
        const Plane   nearP(Vec3 { 0.f, 0.f, 1.f }, -1.f);
        const Plane   farP(Vec3 { 0.f, 0.f, -1.f }, 10.f);
        const Plane   left(Vec3 { 1.f, 0.f, 0.f }, 5.f);
        const Plane   right(Vec3 { -1.f, 0.f, 0.f }, 5.f);
        const Plane   bottom(Vec3 { 0.f, 1.f, 0.f }, 5.f);
        const Plane   top(Vec3 { 0.f, -1.f, 0.f }, 5.f);
        const Frustum frustum(left, right, bottom, top, nearP, farP);

        const Aabb box = Aabb(Vec3 { 0.f, 0.f, 5.f }, Vec3 { 0.5f, 0.5f, 0.5f });
        EXPECT_EQ(frustum.Intersects(box), eIntersect::Inside);
    }

    TEST(BoundingVolumeTest, Frustum_IntersectsAabb_CompletelyOutside)
    {
        const Plane   nearP(Vec3 { 0.f, 0.f, 1.f }, -1.f);
        const Plane   farP(Vec3 { 0.f, 0.f, -1.f }, 10.f);
        const Plane   left(Vec3 { 1.f, 0.f, 0.f }, 1.f);
        const Plane   right(Vec3 { -1.f, 0.f, 0.f }, 1.f);
        const Plane   bottom(Vec3 { 0.f, 1.f, 0.f }, 1.f);
        const Plane   top(Vec3 { 0.f, -1.f, 0.f }, 1.f);
        const Frustum frustum(left, right, bottom, top, nearP, farP);

        const Aabb box = Aabb(Vec3 { 10.f, 0.f, 5.f }, Vec3 { 0.5f, 0.5f, 0.5f });
        EXPECT_EQ(frustum.Intersects(box), eIntersect::Outside);
    }

    TEST(BoundingVolumeTest, Frustum_IntersectsAabb_CrossingOnePlane)
    {
        const Plane   nearP(Vec3 { 0.f, 0.f, 1.f }, -1.f);
        const Plane   farP(Vec3 { 0.f, 0.f, -1.f }, 10.f);
        const Plane   left(Vec3 { 1.f, 0.f, 0.f }, 1.f);
        const Plane   right(Vec3 { -1.f, 0.f, 0.f }, 1.f);
        const Plane   bottom(Vec3 { 0.f, 1.f, 0.f }, 1.f);
        const Plane   top(Vec3 { 0.f, -1.f, 0.f }, 1.f);
        const Frustum frustum(left, right, bottom, top, nearP, farP);

        const Aabb box = Aabb(Vec3 { -1.f, 0.f, 5.f }, Vec3 { 0.5f, 0.5f, 0.5f });
        EXPECT_EQ(frustum.Intersects(box), eIntersect::Overlab);
    }

    TEST(BoundingVolumeTest, Frustum_GetCornerPoints_Count)
    {
        const Plane        nearP(Vec3 { 0.f, 0.f, 1.f }, -1.f);
        const Plane        farP(Vec3 { 0.f, 0.f, -1.f }, 10.f);
        const Plane        left(Vec3 { 1.f, 0.f, 0.f }, 1.f);
        const Plane        right(Vec3 { -1.f, 0.f, 0.f }, 1.f);
        const Plane        bottom(Vec3 { 0.f, 1.f, 0.f }, 1.f);
        const Plane        top(Vec3 { 0.f, -1.f, 0.f }, 1.f);
        const Frustum      frustum(left, right, bottom, top, nearP, farP);
        const Corners corners = frustum.CalcCorners();

        EXPECT_EQ(corners.Size(), 8u);
    }

    TEST(BoundingVolumeTest, Frustum_GetCenter_Symmetry)
    {
        const Plane   nearP(Vec3 { 0.f, 0.f, 1.f }, -1.f);
        const Plane   farP(Vec3 { 0.f, 0.f, -1.f }, 9.f);
        const Plane   left(Vec3 { 1.f, 0.f, 0.f }, 1.f);
        const Plane   right(Vec3 { -1.f, 0.f, 0.f }, 1.f);
        const Plane   bottom(Vec3 { 0.f, 1.f, 0.f }, 1.f);
        const Plane   top(Vec3 { 0.f, -1.f, 0.f }, 1.f);
        const Frustum frustum(left, right, bottom, top, nearP, farP);

        const Vec3 center = frustum.CalcCenter();
        EXPECT_NEAR(center.x, 0.f, 1e-3f);
        EXPECT_NEAR(center.y, 0.f, 1e-3f);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Ray
    ///////////////////////////////////////////////////////////////////////////

    TEST(BoundingVolumeTest, Ray_Normalize_DirectionIsUnit)
    {
        Ray         ray(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 3.f, 0.f, 4.f }, true);
        const float len = ray.dir.Length();
        EXPECT_NEAR(len, 1.f, 1e-5f);
    }

    TEST(BoundingVolumeTest, Ray_Normalized_DoesNotModifyOriginal)
    {
        Ray       ray(Vec3 { 0.f, 0.f, 0.f }, Vec3 { 0.f, 2.f, 0.f }, false);
        const Ray normalized = ray.Normalized();

        EXPECT_FLOAT_EQ(ray.dir.y, 2.f);
        EXPECT_NEAR(normalized.dir.y, 1.f, 1e-5f);
    }

}   // namespace
}   // namespace Jam