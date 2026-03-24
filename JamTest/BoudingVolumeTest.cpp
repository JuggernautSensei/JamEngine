#include "pch.h"
#include <gtest/gtest.h>

// ============================================================
// Helpers
// ============================================================

static constexpr float k_eps = 1e-4f;

static bool Vec3Eq(const Jam::Vec3& a, const Jam::Vec3& b, float eps = k_eps)
{
    return std::abs(a.x - b.x) < eps
        && std::abs(a.y - b.y) < eps
        && std::abs(a.z - b.z) < eps;
}

// ============================================================
// eRegion
// ============================================================

TEST(Region, CalcRegion_AllCombinations)
{
    using namespace Jam;
    EXPECT_EQ(CalcRegion(false, false, false), eRegion::LeftBottomNear);
    EXPECT_EQ(CalcRegion(false, false, true), eRegion::LeftBottomFar);
    EXPECT_EQ(CalcRegion(false, true, false), eRegion::LeftTopNear);
    EXPECT_EQ(CalcRegion(false, true, true), eRegion::LeftTopFar);
    EXPECT_EQ(CalcRegion(true, false, false), eRegion::RightBottomNear);
    EXPECT_EQ(CalcRegion(true, false, true), eRegion::RightBottomFar);
    EXPECT_EQ(CalcRegion(true, true, false), eRegion::RightTopNear);
    EXPECT_EQ(CalcRegion(true, true, true), eRegion::RightTopFar);
}

TEST(Region, IsRight_IsTop_IsFar)
{
    using namespace Jam;
    EXPECT_FALSE(IsRight(eRegion::LeftBottomNear));
    EXPECT_TRUE(IsRight(eRegion::RightBottomNear));
    EXPECT_FALSE(IsTop(eRegion::LeftBottomNear));
    EXPECT_TRUE(IsTop(eRegion::LeftTopNear));
    EXPECT_FALSE(IsFar(eRegion::LeftBottomNear));
    EXPECT_TRUE(IsFar(eRegion::LeftBottomFar));
}

TEST(Region, CalcRegion_RoundTrip)
{
    using namespace Jam;
    for (int r = 0; r <= 1; ++r)
        for (int t = 0; t <= 1; ++t)
            for (int f = 0; f <= 1; ++f)
            {
                const eRegion region = CalcRegion(r, t, f);
                EXPECT_EQ(IsRight(region), r != 0);
                EXPECT_EQ(IsTop(region), t != 0);
                EXPECT_EQ(IsFar(region), f != 0);
            }
}

// ============================================================
// Aabb
// ============================================================

class AabbTest : public ::testing::Test
{
protected:
    Jam::Aabb unit {
        Jam::Vec3 { -1, -1, -1 },
        Jam::Vec3 {  1,  1,  1 }
    };
};

TEST_F(AabbTest, CenterAndExtents)
{
    EXPECT_TRUE(Vec3Eq(unit.GetCenter(), { 0, 0, 0 }));
    EXPECT_TRUE(Vec3Eq(unit.GetExtents(), { 1, 1, 1 }));
}

TEST_F(AabbTest, WidthHeightDepth)
{
    EXPECT_NEAR(unit.GetWidth(), 2.f, k_eps);
    EXPECT_NEAR(unit.GetHeight(), 2.f, k_eps);
    EXPECT_NEAR(unit.GetDepth(), 2.f, k_eps);
}

TEST_F(AabbTest, FromCenterExtents_RoundTrip)
{
    const Jam::Aabb box = Jam::Aabb::FromCenterExtents({ 3, 4, 5 }, { 1, 2, 3 });
    EXPECT_TRUE(Vec3Eq(box.min, { 2, 2, 2 }));
    EXPECT_TRUE(Vec3Eq(box.max, { 4, 6, 8 }));
    EXPECT_TRUE(Vec3Eq(box.GetCenter(), { 3, 4, 5 }));
    EXPECT_TRUE(Vec3Eq(box.GetExtents(), { 1, 2, 3 }));
}

TEST_F(AabbTest, GetCornerPoints_Correct)
{
    const auto corners = unit.GetCornerPoints();
    EXPECT_TRUE(Vec3Eq(corners[Jam::eRegion::LeftBottomNear], { -1, -1, -1 }));
    EXPECT_TRUE(Vec3Eq(corners[Jam::eRegion::RightTopFar], { 1, 1, 1 }));
    EXPECT_TRUE(Vec3Eq(corners[Jam::eRegion::LeftTopNear], { -1, 1, -1 }));
    EXPECT_TRUE(Vec3Eq(corners[Jam::eRegion::RightBottomFar], { 1, -1, 1 }));
}

// ============================================================
// Aabb — Merge
// ============================================================

TEST(AabbMerge, MergeAabb_Expands)
{
    Jam::Aabb a {
        { 0, 0, 0 },
        { 1, 1, 1 }
    };
    a.Merge(Jam::Aabb {
        { -1, -2, -3 },
        {  2,  3,  4 }
    });
    EXPECT_TRUE(Vec3Eq(a.min, { -1, -2, -3 }));
    EXPECT_TRUE(Vec3Eq(a.max, { 2, 3, 4 }));
}

TEST(AabbMerge, MergePoint_Expands)
{
    Jam::Aabb a {
        { 0, 0, 0 },
        { 1, 1, 1 }
    };
    a.Merge(Jam::Vec3 { 5, -3, 2 });
    EXPECT_TRUE(Vec3Eq(a.min, { 0, -3, 0 }));
    EXPECT_TRUE(Vec3Eq(a.max, { 5, 1, 2 }));
}

TEST(AabbMerge, MergePoint_NoShrink)
{
    Jam::Aabb a {
        { -5, -5, -5 },
        {  5,  5,  5 }
    };
    a.Merge(Jam::Vec3 { 0, 0, 0 });
    EXPECT_TRUE(Vec3Eq(a.min, { -5, -5, -5 }));
    EXPECT_TRUE(Vec3Eq(a.max, { 5, 5, 5 }));
}

// ============================================================
// Aabb — Contains
// ============================================================

class AabbContainsTest : public ::testing::Test
{
protected:
    Jam::Aabb box {
        Jam::Vec3 { 0, 0, 0 },
        Jam::Vec3 { 4, 4, 4 }
    };
};

TEST_F(AabbContainsTest, Point_Inside) { EXPECT_TRUE(box.Contains({ 2, 2, 2 })); }
TEST_F(AabbContainsTest, Point_OnEdge)
{
    EXPECT_TRUE(box.Contains({ 0, 0, 0 }));
    EXPECT_TRUE(box.Contains({ 4, 4, 4 }));
}
TEST_F(AabbContainsTest, Point_Outside) { EXPECT_FALSE(box.Contains({ 5, 2, 2 })); }
TEST_F(AabbContainsTest, Aabb_Fully) { EXPECT_TRUE(box.Contains(Jam::Aabb {
    { 1, 1, 1 },
    { 3, 3, 3 }
 })); }
TEST_F(AabbContainsTest, Aabb_Partial) { EXPECT_FALSE(box.Contains(Jam::Aabb {
    { 3, 3, 3 },
    { 5, 5, 5 }
 })); }
TEST_F(AabbContainsTest, Aabb_Itself) { EXPECT_TRUE(box.Contains(box)); }

// ============================================================
// Aabb — Intersects(Aabb)
// ============================================================

class AabbIntersectAabbTest : public ::testing::Test
{
protected:
    Jam::Aabb a {
        Jam::Vec3 { 0, 0, 0 },
        Jam::Vec3 { 2, 2, 2 }
    };
};

TEST_F(AabbIntersectAabbTest, Overlapping) { EXPECT_TRUE(a.Intersects(Jam::Aabb {
    { 1, 1, 1 },
    { 3, 3, 3 }
 })); }
TEST_F(AabbIntersectAabbTest, Touching_NoOverlap) { EXPECT_TRUE(a.Intersects(Jam::Aabb {
    { 2, 0, 0 },
    { 4, 2, 2 }
 })); }
TEST_F(AabbIntersectAabbTest, Separated_X) { EXPECT_FALSE(a.Intersects(Jam::Aabb {
    { 3, 0, 0 },
    { 5, 2, 2 }
 })); }
TEST_F(AabbIntersectAabbTest, Separated_Y) { EXPECT_FALSE(a.Intersects(Jam::Aabb {
    { 0, 3, 0 },
    { 2, 5, 2 }
 })); }
TEST_F(AabbIntersectAabbTest, Separated_Z) { EXPECT_FALSE(a.Intersects(Jam::Aabb {
    { 0, 0, 3 },
    { 2, 2, 5 }
 })); }
TEST_F(AabbIntersectAabbTest, OneInsideAnother) { EXPECT_TRUE(a.Intersects(Jam::Aabb {
    { 0.5f, 0.5f, 0.5f },
    { 1.5f, 1.5f, 1.5f }
 })); }

// ============================================================
// Aabb — Intersects(Ray)
// ============================================================

class AabbRayTest : public ::testing::Test
{
protected:
    Jam::Aabb box {
        Jam::Vec3 { 0, 0, 0 },
        Jam::Vec3 { 2, 2, 2 }
    };
};

TEST_F(AabbRayTest, Hit_FrontFace)
{
    const auto r = box.Intersects(Jam::Ray {
        { 1, 1,  5 },
        { 0, 0, -1 }
    });
    EXPECT_TRUE(r.bHit);
    EXPECT_NEAR(r.distance, 3.f, k_eps);
}

TEST_F(AabbRayTest, Hit_FromInside)
{
    const auto r = box.Intersects(Jam::Ray {
        { 1, 1, 1 },
        { 0, 0, 1 }
    });
    EXPECT_TRUE(r.bHit);
    EXPECT_NEAR(r.distance, 1.f, k_eps);
}

TEST_F(AabbRayTest, Miss_Parallel_Outside) { EXPECT_FALSE(box.Intersects(Jam::Ray {
                                                                            { 5, 0, 1 },
                                                                            { 0, 1, 0 }
 })
                                                              .bHit); }
TEST_F(AabbRayTest, Miss_OppositeDirection) { EXPECT_FALSE(box.Intersects(Jam::Ray {
                                                                             { 1, 1, 5 },
                                                                             { 0, 0, 1 }
 })
                                                               .bHit); }

TEST_F(AabbRayTest, Hit_AxisAligned_X_InsideYZSlab)
{
    // direction=(1,0,0) — Y/Z 슬랩 안 → invDir.y/z=±inf, 정상 동작
    const auto r = box.Intersects(Jam::Ray {
        { -1, 1, 1 },
        {  1, 0, 0 }
    });
    EXPECT_TRUE(r.bHit);
    EXPECT_NEAR(r.distance, 1.f, k_eps);
}

TEST_F(AabbRayTest, Miss_AxisAligned_X_OutsideYSlab)
{
    EXPECT_FALSE(box.Intersects(Jam::Ray {
                                   { -1, 5, 1 },
                                   {  1, 0, 0 }
    })
                     .bHit);
}

TEST_F(AabbRayTest, Hit_Diagonal)
{
    EXPECT_TRUE(box.Intersects(Jam::Ray {
                                  { -2, -2, -2 },
                                  Jam::Normalized({  1,  1,  1 }
                                  )
    })
                    .bHit);
}

// ============================================================
// Obb
// ============================================================

class ObbTest : public ::testing::Test
{
protected:
    // center=(3,4,5), extents(half)=(1,2,3), size=(2,4,6)
    Jam::Obb obb = Jam::Obb::FromAabb(Jam::Aabb::FromCenterExtents({ 3, 4, 5 }, { 1, 2, 3 }));
};

TEST_F(ObbTest, GetCenter) { EXPECT_TRUE(Vec3Eq(obb.GetCenter(), { 3, 4, 5 })); }
TEST_F(ObbTest, GetExtents_HalfSize) { EXPECT_TRUE(Vec3Eq(obb.GetExtents(), { 1, 2, 3 })); }
TEST_F(ObbTest, GetWidth) { EXPECT_NEAR(obb.GetWidth(), 2.f, k_eps); }
TEST_F(ObbTest, GetHeight) { EXPECT_NEAR(obb.GetHeight(), 4.f, k_eps); }
TEST_F(ObbTest, GetDepth) { EXPECT_NEAR(obb.GetDepth(), 6.f, k_eps); }

TEST_F(ObbTest, ToAabb_RoundTrip)
{
    const Jam::Aabb aabb = obb.ToAabb();
    EXPECT_TRUE(Vec3Eq(aabb.min, { 2, 2, 2 }));
    EXPECT_TRUE(Vec3Eq(aabb.max, { 4, 6, 8 }));
}

TEST_F(ObbTest, Contains_Inside) { EXPECT_TRUE(obb.Contains({ 3, 4, 5 })); }
TEST_F(ObbTest, Contains_Outside) { EXPECT_FALSE(obb.Contains({ 10, 10, 10 })); }

TEST_F(ObbTest, RayCast_Hit)
{
    const auto r = obb.Intersects(Jam::Ray {
        { 3, 4, 20 },
        { 0, 0, -1 }
    });
    EXPECT_TRUE(r.bHit);
    EXPECT_NEAR(r.distance, 12.f, k_eps);   // z=20에서 max.z=8까지 거리 12
}

TEST_F(ObbTest, RayCast_Miss)
{
    EXPECT_FALSE(obb.Intersects(Jam::Ray {
                                   { 50, 50, 50 },
                                   {  0,  0, -1 }
    })
                     .bHit);
}

// ============================================================
// Plane
// 외부 법선 기준:
//   DistanceToPoint > 0 = 법선 방향(외부) = Outside
//   DistanceToPoint < 0 = 법선 반대(내부) = Inside
// ============================================================

class PlaneTest : public ::testing::Test
{
protected:
    // XZ 평면(y=0), 외부 법선 = +Y
    // Plane(n, d): dot(n,p)+d=0 → y+0=0 → y=0 평면
    Jam::Plane xzPlane {
        Jam::Vec3 { 0, 1, 0 },
        0.f
    };
};

TEST_F(PlaneTest, IsNormalized) { EXPECT_TRUE(xzPlane.IsNormalized()); }
TEST_F(PlaneTest, GetNormal) { EXPECT_TRUE(Vec3Eq(xzPlane.GetNormal(), { 0, 1, 0 })); }
TEST_F(PlaneTest, Distance_Above) { EXPECT_NEAR(xzPlane.DistanceToPoint({ 0, 3, 0 }), 3.f, k_eps); }
TEST_F(PlaneTest, Distance_Below) { EXPECT_NEAR(xzPlane.DistanceToPoint({ 0, -2, 0 }), -2.f, k_eps); }
TEST_F(PlaneTest, Distance_OnPlane) { EXPECT_NEAR(xzPlane.DistanceToPoint({ 5, 0, 5 }), 0.f, k_eps); }

TEST_F(PlaneTest, RayCast_Hit)
{
    const auto r = xzPlane.Intersects(Jam::Ray {
        { 0,  5, 0 },
        { 0, -1, 0 }
    });
    EXPECT_TRUE(r.bHit);
    EXPECT_NEAR(r.distance, 5.f, k_eps);
}

TEST_F(PlaneTest, RayCast_Miss_OppositeDir) { EXPECT_FALSE(xzPlane.Intersects(Jam::Ray {
                                                                                 { 0, -1, 0 },
                                                                                 { 0, -1, 0 }
 })
                                                               .bHit); }
TEST_F(PlaneTest, RayCast_Miss_Parallel) { EXPECT_FALSE(xzPlane.Intersects(Jam::Ray {
                                                                              { 0, 1, 0 },
                                                                              { 1, 0, 0 }
 })
                                                            .bHit); }

// 외부 법선 기준: DistanceToPoint > 0 = Outside, < 0 = Inside
TEST_F(PlaneTest, IntersectAabb_Outside_AllAbove)
{
    // 박스 전체가 y>0 → 법선 방향 → Outside
    EXPECT_EQ(xzPlane.Intersects(Jam::Aabb {
                  { -1, 1, -1 },
                  {  1, 3,  1 }
    }),
              Jam::ePlaneIntersectType::Outside);
}

TEST_F(PlaneTest, IntersectAabb_Inside_AllBelow)
{
    // 박스 전체가 y<0 → 법선 반대 → Inside
    EXPECT_EQ(xzPlane.Intersects(Jam::Aabb {
                  { -1, -3, -1 },
                  {  1, -1,  1 }
    }),
              Jam::ePlaneIntersectType::Inside);
}

TEST_F(PlaneTest, IntersectAabb_Intersect_Straddles)
{
    EXPECT_EQ(xzPlane.Intersects(Jam::Aabb {
                  { -1, -1, -1 },
                  {  1,  1,  1 }
    }),
              Jam::ePlaneIntersectType::Intersects);
}

// ============================================================
// Plane — IntersectOf3Planes
// ============================================================

TEST(PlaneIntersect3, MeetAtOrigin)
{
    const Jam::Plane px {
        { 1, 0, 0 },
        0.f
    };
    const Jam::Plane py {
        { 0, 1, 0 },
        0.f
    };
    const Jam::Plane pz {
        { 0, 0, 1 },
        0.f
    };
    EXPECT_TRUE(Vec3Eq(Jam::Plane::IntersectOf3Planes(px, py, pz), { 0, 0, 0 }));
}

TEST(PlaneIntersect3, MeetAtOffset)
{
    const Jam::Plane px {
        { 1, 0, 0 },
        -1.f, false
    };
    const Jam::Plane py {
        { 0, 1, 0 },
        -2.f, false
    };
    const Jam::Plane pz {
        { 0, 0, 1 },
        -3.f, false
    };
    EXPECT_TRUE(Vec3Eq(Jam::Plane::IntersectOf3Planes(px, py, pz), { 1, 2, 3 }));
}

TEST(PlaneIntersect3, ParallelPlanes_ReturnsZero)
{
    const Jam::Plane p1 {
        { 1, 0, 0 },
        0.f
    };
    const Jam::Plane p2 {
        { 1, 0, 0 },
        -1.f, false
    };
    const Jam::Plane p3 {
        { 0, 1, 0 },
        0.f
    };
    EXPECT_TRUE(Vec3Eq(Jam::Plane::IntersectOf3Planes(p1, p2, p3), { 0, 0, 0 }));
}

// ============================================================
// Frustum
//
// 외부 법선 기준 직교 Frustum: x∈[-5,5], y∈[-5,5], z∈[1,10]
// 각 평면: Plane(n, d), 방정식 dot(n,p)+d=0, d=-dot(n, 평면위의점)
//
//   Left   x=-5, n=(-1,0,0): d=-dot((-1,0,0),(-5,0,0))=-5
//   Right  x= 5, n=( 1,0,0): d=-dot(( 1,0,0),( 5,0,0))=-5
//   Bottom y=-5, n=(0,-1,0):  d=-5
//   Top    y= 5, n=(0, 1,0):  d=-5
//   Near   z= 1, n=(0,0,-1):  d=-dot((0,0,-1),(0,0,1))=1
//   Far    z=10, n=(0,0, 1):  d=-dot((0,0, 1),(0,0,10))=-10
//
// 내부점 (0,0,5) 검증 (모두 < 0이어야 Inside):
//   Left:   dot((-1,0,0),(0,0,5))+(-5) = -5 < 0 ✅
//   Right:  dot(( 1,0,0),(0,0,5))+(-5) = -5 < 0 ✅
//   Bottom: dot((0,-1,0),(0,0,5))+(-5) = -5 < 0 ✅
//   Top:    dot((0, 1,0),(0,0,5))+(-5) = -5 < 0 ✅
//   Near:   dot((0,0,-1),(0,0,5))+(1)  = -4 < 0 ✅
//   Far:    dot((0,0, 1),(0,0,5))+(-10)= -5 < 0 ✅
// ============================================================

class FrustumTest : public ::testing::Test
{
protected:
    Jam::Frustum MakeOrthoFrustum()
    {
        return Jam::Frustum {
            Jam::Plane { Jam::Vec3 { -1, 0, 0 },  -5.f }, // Left
            Jam::Plane {  Jam::Vec3 { 1, 0, 0 },  -5.f }, // Right
            Jam::Plane { Jam::Vec3 { 0, -1, 0 },  -5.f }, // Bottom
            Jam::Plane {  Jam::Vec3 { 0, 1, 0 },  -5.f }, // Top
            Jam::Plane { Jam::Vec3 { 0, 0, -1 },   1.f }, // Near  z=1
            Jam::Plane {  Jam::Vec3 { 0, 0, 1 }, -10.f }, // Far   z=10
        };
    }

    Jam::Frustum frustum = MakeOrthoFrustum();
};

TEST_F(FrustumTest, IntersectAabb_FullyInside)
{
    EXPECT_EQ(frustum.Intersects(Jam::Aabb {
                  { -1, -1, 2 },
                  {  1,  1, 5 }
    }),
              Jam::ePlaneIntersectType::Inside);
}

TEST_F(FrustumTest, IntersectAabb_Outside_BeyondFar)
{
    EXPECT_EQ(frustum.Intersects(Jam::Aabb {
                  { -1, -1, 11 },
                  {  1,  1, 15 }
    }),
              Jam::ePlaneIntersectType::Outside);
}

TEST_F(FrustumTest, IntersectAabb_Outside_BeforeNear)
{
    EXPECT_EQ(frustum.Intersects(Jam::Aabb {
                  { -1, -1, -5 },
                  {  1,  1,  0 }
    }),
              Jam::ePlaneIntersectType::Outside);
}

TEST_F(FrustumTest, IntersectAabb_Outside_OffToSide)
{
    EXPECT_EQ(frustum.Intersects(Jam::Aabb {
                  {  8, 0, 2 },
                  { 12, 4, 8 }
    }),
              Jam::ePlaneIntersectType::Outside);
}

TEST_F(FrustumTest, IntersectAabb_Intersect_NearPlane)
{
    EXPECT_EQ(frustum.Intersects(Jam::Aabb {
                  { -1, -1, 0 },
                  {  1,  1, 3 }
    }),
              Jam::ePlaneIntersectType::Intersects);
}

TEST_F(FrustumTest, IntersectAabb_Intersect_SidePlane)
{
    EXPECT_EQ(frustum.Intersects(Jam::Aabb {
                  { 4, -1, 2 },
                  { 6,  1, 8 }
    }),
              Jam::ePlaneIntersectType::Intersects);
}

TEST_F(FrustumTest, IntersectAabb_LargeBox_NotOutside)
{
    EXPECT_NE(frustum.Intersects(Jam::Aabb {
                  { -100, -100, -100 },
                  {  100,  100,  100 }
    }),
              Jam::ePlaneIntersectType::Outside);
}

// Ray 검증:
// origin=(0,0,-5), dir=(0,0,1)
//   Near: n=(0,0,-1), d=1, denom=-1<0 → tNear, numer=-(5+1)=-6, t=6  → tNear=6
//   Far:  n=(0,0, 1), d=-10, denom=1>0 → tFar,  numer=-(-5-10)=15, t=15 → tFar=15
//   tNear=6 < tFar=15, tNear>=0 → distance=6 ✅

TEST_F(FrustumTest, RayCast_Hit_ThroughCenter)
{
    const auto r = frustum.Intersects(Jam::Ray {
        { 0, 0, -5 },
        { 0, 0,  1 }
    });
    EXPECT_TRUE(r.bHit);
    EXPECT_NEAR(r.distance, 6.f, k_eps);
}

TEST_F(FrustumTest, RayCast_Miss_Parallel_Outside)
{
    EXPECT_FALSE(frustum.Intersects(Jam::Ray {
                                       { 10, 0, 5 },
                                       {  0, 0, 1 }
    })
                     .bHit);
}

TEST_F(FrustumTest, RayCast_Miss_OppositeDirection)
{
    EXPECT_FALSE(frustum.Intersects(Jam::Ray {
                                       { 0, 0, -5 },
                                       { 0, 0, -1 }
    })
                     .bHit);
}

TEST_F(FrustumTest, RayCast_Hit_FromInside)
{
    // origin=(0,0,5) 내부, dir=(0,0,1) → Far(z=10)까지 거리 5
    // Far: denom=1>0 → tFar, numer=-(5-10)=5, t=5 → tFar=5
    // Near: denom=-1<0 → tNear, numer=-(−5+1)=4, t=-4 → tNear=-4
    // tNear=-4 < tFar=5, tNear<0 → distance=tFar=5 ✅
    const auto r = frustum.Intersects(Jam::Ray {
        { 0, 0, 5 },
        { 0, 0, 1 }
    });
    EXPECT_TRUE(r.bHit);
    EXPECT_NEAR(r.distance, 5.f, k_eps);
}

// ============================================================
// Frustum — GetCenter
// ============================================================

TEST_F(FrustumTest, GetCenter_OrthoFrustum)
{
    const Jam::Vec3 c = frustum.GetCenter();
    EXPECT_NEAR(c.x, 0.f, k_eps);
    EXPECT_NEAR(c.y, 0.f, k_eps);
    EXPECT_NEAR(c.z, 5.5f, k_eps);
}

// ============================================================
// Ray
// ============================================================

TEST(RayTest, NormalizeOnConstruct)
{
    const Jam::Ray ray {
        { 0, 0, 0 },
        { 0, 0, 5 }
    };
    EXPECT_TRUE(ray.IsDirectionNormalized());
    EXPECT_TRUE(Vec3Eq(ray.direction, { 0, 0, 1 }));
}

TEST(RayTest, NormalizedDirection_DoesNotModifyOriginal)
{
    Jam::Ray ray {
        { 0, 0, 0 },
        { 0, 0, 3 },
        false
    };
    const Jam::Ray norm = ray.NormalizedDirection();
    EXPECT_TRUE(Vec3Eq(norm.direction, { 0, 0, 1 }));
    EXPECT_TRUE(Vec3Eq(ray.direction, { 0, 0, 3 }));
}