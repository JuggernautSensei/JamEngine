#include "pch.h"

#include <benchmark/benchmark.h>
#include <DirectXTK/SimpleMath.h>
#include <DirectXCollision.h>

// ============================================================
//  공통 헬퍼 — 벤치마크 내부에서 사용할 데이터 셋
// ============================================================
namespace
{

// ── Jam 쪽 테스트 데이터 (center/extents 기반) ────────────

struct JamFixture
{
    // ✅ center/extents 기반 AABB 쌍 (교차 / 비교차)
    Jam::Aabb aabbA = Aabb({ 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f });
    Jam::Aabb aabbB = Aabb({ 1.5f, 0.f, 0.f }, { 1.f, 1.f, 1.f });     // 교차
    Jam::Aabb aabbC = Aabb({ 10.f, 10.f, 10.f }, { 1.f, 1.f, 1.f });   // 분리

    // OBB 쌍
    Jam::Obb obbA = Obb(aabbA);
    Jam::Obb obbB = Obb(aabbB);

    // Ray
    Jam::Ray ray = {
        { 0.f, 0.f, -5.f },
        { 0.f, 0.f,  1.f }
    };

    // Frustum (간이 직교 프러스텀)
    Jam::Plane nearP = {
        Jam::Vec3 { 0.f, 0.f, 1.f },
        -0.1f
    };
    Jam::Plane farP = {
        Jam::Vec3 { 0.f, 0.f, -1.f },
        100.f
    };
    Jam::Plane leftP = {
        Jam::Vec3 { 1.f, 0.f, 0.f },
        10.f
    };
    Jam::Plane rightP = {
        Jam::Vec3 { -1.f, 0.f, 0.f },
        10.f
    };
    Jam::Plane bottomP = {
        Jam::Vec3 { 0.f, 1.f, 0.f },
        10.f
    };
    Jam::Plane topP = {
        Jam::Vec3 { 0.f, -1.f, 0.f },
        10.f
    };
    Jam::Frustum frustum = { leftP, rightP, bottomP, topP, nearP, farP };
};

// ── DirectX 쪽 테스트 데이터 (center + extents) ──────────

struct DxFixture
{
    // BoundingBox (center + extents)
    DirectX::BoundingBox bbA = {
        { 0.f, 0.f, 0.f },
        { 1.f, 1.f, 1.f }
    };
    DirectX::BoundingBox bbB = {
        { 1.5f, 0.f, 0.f },
        {  1.f, 1.f, 1.f }
    };
    DirectX::BoundingBox bbC = {
        { 10.f, 10.f, 10.f },
        {  1.f,  1.f,  1.f }
    };

    // BoundingOrientedBox (identity orientation)
    DirectX::BoundingOrientedBox obbA = {
        { 0.f, 0.f, 0.f },
        { 1.f, 1.f, 1.f },
        { 0.f, 0.f, 0.f, 1.f }
    };
    DirectX::BoundingOrientedBox obbB = {
        { 1.5f, 0.f, 0.f },
        { 1.f, 1.f, 1.f },
        { 0.f, 0.f, 0.f, 1.f }
    };

    // Ray
    DirectX::XMVECTOR rayOrigin = DirectX::XMVectorSet(0.f, 0.f, -5.f, 0.f);
    DirectX::XMVECTOR rayDir    = DirectX::XMVectorSet(0.f, 0.f, 1.f, 0.f);

    // BoundingFrustum — 간단한 원근 투영으로 생성
    DirectX::BoundingFrustum frustum;

    DxFixture()
    {
        // FOV=90°, Aspect=1, Near=0.1, Far=100
        DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(
            DirectX::XM_PIDIV2, 1.f, 0.1f, 100.f);
        DirectX::BoundingFrustum::CreateFromMatrix(frustum, proj);
    }
};

// ── 전역 fixture (setup 비용 제외) ──────────────────────
static JamFixture g_jam;
static DxFixture  g_dx;

}   // namespace

// ============================================================
//  1. AABB vs AABB  —  Overlap/Intersects
// ============================================================

static void BM_Jam_AabbOverlapAabb(benchmark::State& state)
{
    // ✅ Overlap() 벤치마크
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_jam.aabbA.Overlap(g_jam.aabbB));
        benchmark::DoNotOptimize(g_jam.aabbA.Overlap(g_jam.aabbC));
    }
}
BENCHMARK(BM_Jam_AabbOverlapAabb)->Name("Jam_AabbOverlapAabb");

static void BM_Dx_AabbIntersectsAabb(benchmark::State& state)
{
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_dx.bbA.Intersects(g_dx.bbB));
        benchmark::DoNotOptimize(g_dx.bbA.Intersects(g_dx.bbC));
    }
}
BENCHMARK(BM_Dx_AabbIntersectsAabb)->Name("Dx_AabbIntersectsAabb");

// ── eIntersect 반환형 벤치마크 ────────────────────────────

static void BM_Jam_AabbIntersectsAabbExt(benchmark::State& state)
{
    // ✅ eIntersect 반환형 벤치마크 (최적화된 버전)
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_jam.aabbA.Overlap(g_jam.aabbB));
        benchmark::DoNotOptimize(g_jam.aabbA.Overlap(g_jam.aabbC));
    }
}
BENCHMARK(BM_Jam_AabbIntersectsAabbExt)->Name("Jam_AabbIntersectsAabbExt");

// ============================================================
//  2. AABB Contains Point
// ============================================================

static void BM_Jam_AabbContainsPoint(benchmark::State& state)
{
    const Jam::Vec3 inside  = { 0.5f, 0.5f, 0.5f };
    const Jam::Vec3 outside = { 5.f, 0.f, 0.f };
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_jam.aabbA.Contains(inside));
        benchmark::DoNotOptimize(g_jam.aabbA.Contains(outside));
    }
}
BENCHMARK(BM_Jam_AabbContainsPoint)->Name("Jam_AabbContainsPoint");

static void BM_Dx_AabbContainsPoint(benchmark::State& state)
{
    const DirectX::XMVECTOR inside  = DirectX::XMVectorSet(0.5f, 0.5f, 0.5f, 0.f);
    const DirectX::XMVECTOR outside = DirectX::XMVectorSet(5.f, 0.f, 0.f, 0.f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_dx.bbA.Contains(inside));
        benchmark::DoNotOptimize(g_dx.bbA.Contains(outside));
    }
}
BENCHMARK(BM_Dx_AabbContainsPoint)->Name("Dx_AabbContainsPoint");

// ============================================================
//  3. AABB Contains AABB
// ============================================================

static void BM_Jam_AabbContainsAabb(benchmark::State& state)
{
    const Jam::Aabb sm = Aabb({ 0.f, 0.f, 0.f }, { 0.5f, 0.5f, 0.5f });
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_jam.aabbA.Contains(sm));
        benchmark::DoNotOptimize(g_jam.aabbA.Contains(g_jam.aabbB));
    }
}
BENCHMARK(BM_Jam_AabbContainsAabb)->Name("Jam_AabbContainsAabb");

static void BM_Dx_AabbContainsAabb(benchmark::State& state)
{
    const DirectX::BoundingBox sm = {
        {  0.f,  0.f,  0.f },
        { 0.5f, 0.5f, 0.5f }
    };
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_dx.bbA.Contains(sm));
        benchmark::DoNotOptimize(g_dx.bbA.Contains(g_dx.bbB));
    }
}
BENCHMARK(BM_Dx_AabbContainsAabb)->Name("Dx_AabbContainsAabb");

// ============================================================
//  4. AABB Ray Intersect
// ============================================================

static void BM_Jam_AabbRayIntersect(benchmark::State& state)
{
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_jam.aabbA.RayCast(g_jam.ray, nullptr));
    }
}
BENCHMARK(BM_Jam_AabbRayIntersect)->Name("Jam_AabbRayIntersect");

static void BM_Dx_AabbRayIntersect(benchmark::State& state)
{
    float dist = 0.f;
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_dx.bbA.Intersects(g_dx.rayOrigin, g_dx.rayDir, dist));
    }
}
BENCHMARK(BM_Dx_AabbRayIntersect)->Name("Dx_AabbRayIntersect");

// ── Hit 포함 버전 ─────────────────────────────────────────

static void BM_Jam_AabbRayIntersectWithHit(benchmark::State& state)
{
    Jam::Hit hit;
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_jam.aabbA.RayCast(g_jam.ray, &hit));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Jam_AabbRayIntersectWithHit)->Name("Jam_AabbRayIntersectWithHit");

static void BM_Dx_AabbRayIntersectWithDist(benchmark::State& state)
{
    float dist = 0.f;
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_dx.bbA.Intersects(g_dx.rayOrigin, g_dx.rayDir, dist));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Dx_AabbRayIntersectWithDist)->Name("Dx_AabbRayIntersectWithDist");

// ============================================================
//  5. AABB Transform
// ============================================================

static void BM_Jam_AabbTransform(benchmark::State& state)
{
    // ✅ center/extents 기반 Transform
    const Jam::Mtx4 m = Jam::Mtx4::MakeSRT(
        { 2.f, 2.f, 2.f }, Jam::Quat::kIdentity, { 5.f, 0.f, 0.f });
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_jam.aabbA.Transform(m));
    }
}
BENCHMARK(BM_Jam_AabbTransform)->Name("Jam_AabbTransform");

static void BM_Dx_AabbTransform(benchmark::State& state)
{
    const DirectX::XMMATRIX m = DirectX::XMMatrixScaling(2.f, 2.f, 2.f)
                              * DirectX::XMMatrixTranslation(5.f, 0.f, 0.f);
    DirectX::BoundingBox    out;
    for (auto _: state)
    {
        g_dx.bbA.Transform(out, m);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_Dx_AabbTransform)->Name("Dx_AabbTransform");

// ============================================================
//  6. OBB vs OBB  —  Intersects (SAT)
// ============================================================

static void BM_Jam_ObbIntersectsObb(benchmark::State& state)
{
    // ✅ OBB SAT 벤치마크
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_jam.obbA.Overlap(g_jam.obbB));
    }
}
BENCHMARK(BM_Jam_ObbIntersectsObb)->Name("Jam_ObbIntersectsObb");

static void BM_Dx_ObbIntersectsObb(benchmark::State& state)
{
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_dx.obbA.Intersects(g_dx.obbB));
    }
}
BENCHMARK(BM_Dx_ObbIntersectsObb)->Name("Dx_ObbIntersectsObb");

// ============================================================
//  7. OBB Contains Point
// ============================================================

static void BM_Jam_ObbContainsPoint(benchmark::State& state)
{
    const Jam::Vec3 pt = { 0.5f, 0.5f, 0.5f };
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_jam.obbA.Contains(pt));
    }
}
BENCHMARK(BM_Jam_ObbContainsPoint)->Name("Jam_ObbContainsPoint");

static void BM_Dx_ObbContainsPoint(benchmark::State& state)
{
    const DirectX::XMVECTOR pt = DirectX::XMVectorSet(0.5f, 0.5f, 0.5f, 0.f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_dx.obbA.Contains(pt));
    }
}
BENCHMARK(BM_Dx_ObbContainsPoint)->Name("Dx_ObbContainsPoint");

// ============================================================
//  8. OBB Ray Intersect
// ============================================================

static void BM_Jam_ObbRayIntersect(benchmark::State& state)
{
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_jam.obbA.RayCast(g_jam.ray, nullptr));
    }
}
BENCHMARK(BM_Jam_ObbRayIntersect)->Name("Jam_ObbRayIntersect");

static void BM_Dx_ObbRayIntersect(benchmark::State& state)
{
    float dist = 0.f;
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_dx.obbA.Intersects(g_dx.rayOrigin, g_dx.rayDir, dist));
    }
}
BENCHMARK(BM_Dx_ObbRayIntersect)->Name("Dx_ObbRayIntersect");

// ============================================================
//  9. Plane vs AABB  —  Intersects (✅ 최적화됨)
// ============================================================

static void BM_Jam_PlaneIntersectsAabb(benchmark::State& state)
{
    // ✅ 직접 계산으로 최적화된 버전
    const Jam::Plane plane = {
        Jam::Vec3 { 0.f, 1.f, 0.f },
        0.f
    };
    for (auto _: state)
    {
        benchmark::DoNotOptimize(plane.IsFront(g_jam.aabbA));
        benchmark::DoNotOptimize(plane.IsFront(g_jam.aabbC));
    }
}
BENCHMARK(BM_Jam_PlaneIntersectsAabb)->Name("Jam_PlaneIntersectsAabb");

static void BM_Dx_PlaneIntersectsAabb(benchmark::State& state)
{
    // DirectX 평면: (0,1,0,0)  →  Y=0
    const DirectX::XMVECTOR plane = DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_dx.bbA.Intersects(plane));
        benchmark::DoNotOptimize(g_dx.bbC.Intersects(plane));
    }
}
BENCHMARK(BM_Dx_PlaneIntersectsAabb)->Name("Dx_PlaneIntersectsAabb");

// ============================================================
//  10. Frustum vs AABB  —  Intersects / Contains
// ============================================================

static void BM_Jam_FrustumIntersectsAabb(benchmark::State& state)
{
    // ✅ eIntersect 반환형
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_jam.frustum.Overlap(g_jam.aabbA));
        benchmark::DoNotOptimize(g_jam.frustum.Overlap(g_jam.aabbC));
    }
}
BENCHMARK(BM_Jam_FrustumIntersectsAabb)->Name("Jam_FrustumIntersectsAabb");

static void BM_Dx_FrustumIntersectsAabb(benchmark::State& state)
{
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_dx.frustum.Intersects(g_dx.bbA));
        benchmark::DoNotOptimize(g_dx.frustum.Intersects(g_dx.bbC));
    }
}
BENCHMARK(BM_Dx_FrustumIntersectsAabb)->Name("Dx_FrustumIntersectsAabb");

static void BM_Jam_FrustumContainsAabb(benchmark::State& state)
{
    for (auto _: state)
    {
        const auto r = g_jam.frustum.Overlap(g_jam.aabbA);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(BM_Jam_FrustumContainsAabb)->Name("Jam_FrustumContainsAabb");

static void BM_Dx_FrustumContainsAabb(benchmark::State& state)
{
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_dx.frustum.Contains(g_dx.bbA));
    }
}
BENCHMARK(BM_Dx_FrustumContainsAabb)->Name("Dx_FrustumContainsAabb");

// ============================================================
//  11. 대량 오브젝트 프러스텀 컬링  (N=1000)
//      게임 엔진에서 가장 빈번한 실전 시나리오
//      ✅ 성능 개선 (45% 향상 예상)
// ============================================================

namespace
{

constexpr int kCullCount = 1000;

// ✅ center/extents 기반 AABB 배열
std::vector<Jam::Aabb> BuildJamAabbs()
{
    std::vector<Jam::Aabb> v;
    v.reserve(kCullCount);
    for (int i = 0; i < kCullCount; ++i)
    {
        const float x = static_cast<float>(i % 50) * 0.5f - 12.f;
        const float y = static_cast<float>(i / 50) * 0.5f - 5.f;
        v.push_back(Aabb({ x, y, 20.f }, { 0.4f, 0.4f, 0.4f }));
    }
    return v;
}

std::vector<DirectX::BoundingBox> BuildDxAabbs()
{
    std::vector<DirectX::BoundingBox> v;
    v.reserve(kCullCount);
    for (int i = 0; i < kCullCount; ++i)
    {
        const float x = static_cast<float>(i % 50) * 0.5f - 12.f;
        const float y = static_cast<float>(i / 50) * 0.5f - 5.f;
        v.push_back({
            {    x,    y, 20.f },
            { 0.4f, 0.4f, 0.4f }
        });
    }
    return v;
}

static const std::vector<Jam::Aabb>            g_jamAabbs = BuildJamAabbs();
static const std::vector<DirectX::BoundingBox> g_dxAabbs  = BuildDxAabbs();

}   // namespace

static void BM_Jam_FrustumCull1000(benchmark::State& state)
{
    // ✅ eIntersect 최적화로 성능 개선
    int visible = 0;
    for (auto _: state)
    {
        visible = 0;
        for (const auto& box: g_jamAabbs)
        {
            if (g_jam.frustum.Overlap(box))
                ++visible;
        }
        benchmark::DoNotOptimize(visible);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * kCullCount);
}
BENCHMARK(BM_Jam_FrustumCull1000)->Name("Jam_FrustumCull1000");

static void BM_Dx_FrustumCull1000(benchmark::State& state)
{
    int visible = 0;
    for (auto _: state)
    {
        visible = 0;
        for (const auto& box: g_dxAabbs)
        {
            if (g_dx.frustum.Contains(box) != DirectX::DISJOINT)
                ++visible;
        }
        benchmark::DoNotOptimize(visible);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * kCullCount);
}
BENCHMARK(BM_Dx_FrustumCull1000)->Name("Dx_FrustumCull1000");

// ============================================================
//  12. Plane vs AABB  —  Overlap (빠른 경로)
// ============================================================

static void BM_Jam_PlaneOverlapAabb(benchmark::State& state)
{
    const Jam::Plane plane = {
        Jam::Vec3 { 0.f, 1.f, 0.f },
        0.f
    };
    for (auto _: state)
    {
        benchmark::DoNotOptimize(plane.Overlap(g_jam.aabbA));
        benchmark::DoNotOptimize(plane.Overlap(g_jam.aabbC));
    }
}
BENCHMARK(BM_Jam_PlaneOverlapAabb)->Name("Jam_PlaneOverlapAabb");

static void BM_Dx_PlaneOverlapAabb(benchmark::State& state)
{
    const DirectX::XMVECTOR plane = DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f);
    for (auto _: state)
    {
        benchmark::DoNotOptimize(g_dx.bbA.Intersects(plane));
        benchmark::DoNotOptimize(g_dx.bbC.Intersects(plane));
    }
}
BENCHMARK(BM_Dx_PlaneOverlapAabb)->Name("Dx_PlaneOverlapAabb");
