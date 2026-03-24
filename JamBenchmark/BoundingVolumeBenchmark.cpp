#include "pch.h"

#include <DirectXCollision.h>   // BoundingBox, BoundingFrustum
#include <DirectXMath.h>

#include <benchmark/benchmark.h>
#include <random>
#include <vector>

// =============================================================================
// 테스트 데이터 생성 (Jam / DX 공용 파라미터)
// =============================================================================

static std::mt19937 s_rng { 42 };

inline float RandFloat(float lo, float hi)
{
    return std::uniform_real_distribution<float> { lo, hi }(s_rng);
}

// ---- Jam 타입 ----------------------------------------------------------------

static Jam::Aabb MakeRandomAabb()
{
    const Jam::Vec3 c = { RandFloat(-1000.f, 1000.f),
                          RandFloat(-1000.f, 1000.f),
                          RandFloat(-1000.f, 1000.f) };
    const Jam::Vec3 e = { RandFloat(0.5f, 50.f),
                          RandFloat(0.5f, 50.f),
                          RandFloat(0.5f, 50.f) };
    return Jam::Aabb::FromCenterExtents(c, e);
}

static Jam::Plane MakeRandomPlane()
{
    const Jam::Vec3 n = Normalized(Jam::Vec3 { RandFloat(-1.f, 1.f),
                                               RandFloat(-1.f, 1.f),
                                               RandFloat(-1.f, 1.f) });
    return Jam::Plane { n, RandFloat(-1000.f, 1000.f) };
}

static Jam::Frustum MakeRandomFrustum()
{
    const float r  = RandFloat(50.f, 500.f);
    const float zn = RandFloat(1.f, 100.f);
    const float zf = zn + RandFloat(100.f, 2000.f);
    const float ox = RandFloat(-500.f, 500.f);
    const float oy = RandFloat(-500.f, 500.f);
    const float oz = RandFloat(-500.f, 500.f);
    return Jam::Frustum {
        Jam::Plane { Jam::Vec3 { -1, 0, 0 },  -(ox - r) },
        Jam::Plane {  Jam::Vec3 { 1, 0, 0 },  -(ox + r) },
        Jam::Plane { Jam::Vec3 { 0, -1, 0 },  -(oy - r) },
        Jam::Plane {  Jam::Vec3 { 0, 1, 0 },  -(oy + r) },
        Jam::Plane { Jam::Vec3 { 0, 0, -1 }, -(oz + zn) },
        Jam::Plane {  Jam::Vec3 { 0, 0, 1 }, -(oz + zf) },
    };
}

// ---- DirectX 타입 -----------------------------------------------------------
// Jam과 동일한 파라미터로 생성해 공정한 비교를 보장

static DirectX::BoundingBox MakeDxAabb()
{
    const float cx = RandFloat(-1000.f, 1000.f);
    const float cy = RandFloat(-1000.f, 1000.f);
    const float cz = RandFloat(-1000.f, 1000.f);
    const float ex = RandFloat(0.5f, 50.f);
    const float ey = RandFloat(0.5f, 50.f);
    const float ez = RandFloat(0.5f, 50.f);
    // BoundingBox(center, extents)
    return DirectX::BoundingBox {
        { cx, cy, cz },
        { ex, ey, ez }
    };
}

// SimpleMath::Plane: (normal.x, normal.y, normal.z, d)  ax+by+cz+d=0
static DirectX::SimpleMath::Plane MakeDxPlane()
{
    DirectX::XMFLOAT3 n   = { RandFloat(-1.f, 1.f), RandFloat(-1.f, 1.f), RandFloat(-1.f, 1.f) };
    const float       len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    n.x /= len;
    n.y /= len;
    n.z /= len;
    return { n.x, n.y, n.z, RandFloat(-1000.f, 1000.f) };
}

// BoundingFrustum: 직교 프러스텀을 투영 행렬로 생성
// DirectX::BoundingFrustum은 뷰 공간 기준 생성 후 월드 변환 적용
static DirectX::BoundingFrustum MakeDxFrustum()
{
    const float r  = RandFloat(50.f, 500.f);
    const float zn = RandFloat(1.f, 100.f);
    const float zf = zn + RandFloat(100.f, 2000.f);
    const float ox = RandFloat(-500.f, 500.f);
    const float oy = RandFloat(-500.f, 500.f);
    const float oz = RandFloat(-500.f, 500.f);

    // 직교 투영 행렬로 BoundingFrustum 생성
    const DirectX::XMMATRIX proj = DirectX::XMMatrixOrthographicOffCenterLH(
        ox - r, ox + r,   // left, right
        oy - r,
        oy + r,   // bottom, top
        zn,
        zf   // near, far
    );

    DirectX::BoundingFrustum frustum;
    DirectX::BoundingFrustum::CreateFromMatrix(frustum, proj);

    // 월드 위치 적용
    const DirectX::XMMATRIX world = DirectX::XMMatrixTranslation(0.f, 0.f, oz);
    frustum.Transform(frustum, world);

    return frustum;
}

// =============================================================================
// Jam 벤치마크
// =============================================================================

static void BM_Jam_AabbVsAabb(benchmark::State& state)
{
    const int              n = static_cast<int>(state.range(0));
    std::vector<Jam::Aabb> setA(n), setB(n);
    for (int i = 0; i < n; ++i)
    {
        setA[i] = MakeRandomAabb();
        setB[i] = MakeRandomAabb();
    }

    for (auto _: state)
    {
        long long hits = 0;
        for (int i = 0; i < n; ++i)
            hits += setA[i].Intersects(setB[i]) ? 1 : 0;
        benchmark::DoNotOptimize(hits);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_Jam_AabbVsPlane(benchmark::State& state)
{
    const int               n = static_cast<int>(state.range(0));
    std::vector<Jam::Aabb>  boxes(n);
    std::vector<Jam::Plane> planes(n);
    for (int i = 0; i < n; ++i)
    {
        boxes[i]  = MakeRandomAabb();
        planes[i] = MakeRandomPlane();
    }

    for (auto _: state)
    {
        long long hits = 0;
        for (int i = 0; i < n; ++i)
            hits += (planes[i].Intersects(boxes[i]) != Jam::ePlaneIntersectType::Outside) ? 1 : 0;
        benchmark::DoNotOptimize(hits);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_Jam_AabbVsFrustum(benchmark::State& state)
{
    const int                 n = static_cast<int>(state.range(0));
    std::vector<Jam::Aabb>    boxes(n);
    std::vector<Jam::Frustum> frustums(n);
    for (int i = 0; i < n; ++i)
    {
        boxes[i]    = MakeRandomAabb();
        frustums[i] = MakeRandomFrustum();
    }

    for (auto _: state)
    {
        long long hits = 0;
        for (int i = 0; i < n; ++i)
            hits += (frustums[i].Intersects(boxes[i]) != Jam::ePlaneIntersectType::Outside) ? 1 : 0;
        benchmark::DoNotOptimize(hits);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

// =============================================================================
// DirectX 벤치마크
// =============================================================================

static void BM_DX_AabbVsAabb(benchmark::State& state)
{
    const int                         n = static_cast<int>(state.range(0));
    std::vector<DirectX::BoundingBox> setA(n), setB(n);
    for (int i = 0; i < n; ++i)
    {
        setA[i] = MakeDxAabb();
        setB[i] = MakeDxAabb();
    }

    for (auto _: state)
    {
        long long hits = 0;
        for (int i = 0; i < n; ++i)
            hits += setA[i].Intersects(setB[i]) ? 1 : 0;
        benchmark::DoNotOptimize(hits);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_DX_AabbVsPlane(benchmark::State& state)
{
    const int                               n = static_cast<int>(state.range(0));
    std::vector<DirectX::BoundingBox>       boxes(n);
    std::vector<DirectX::SimpleMath::Plane> planes(n);
    for (int i = 0; i < n; ++i)
    {
        boxes[i]  = MakeDxAabb();
        planes[i] = MakeDxPlane();
    }

    for (auto _: state)
    {
        long long hits = 0;
        for (int i = 0; i < n; ++i)
        {
            // BoundingBox::Intersects(FXMVECTOR plane) → PlaneIntersectionType
            // DISJOINT == Outside
            const DirectX::PlaneIntersectionType t = boxes[i].Intersects(planes[i]);
            hits += (t != DirectX::DISJOINT) ? 1 : 0;
        }
        benchmark::DoNotOptimize(hits);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_DX_AabbVsFrustum(benchmark::State& state)
{
    const int                             n = static_cast<int>(state.range(0));
    std::vector<DirectX::BoundingBox>     boxes(n);
    std::vector<DirectX::BoundingFrustum> frustums(n);
    for (int i = 0; i < n; ++i)
    {
        boxes[i]    = MakeDxAabb();
        frustums[i] = MakeDxFrustum();
    }

    for (auto _: state)
    {
        long long hits = 0;
        for (int i = 0; i < n; ++i)
        {
            // BoundingFrustum::Intersects(BoundingBox) → bool
            hits += frustums[i].Intersects(boxes[i]) ? 1 : 0;
        }
        benchmark::DoNotOptimize(hits);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

// =============================================================================
// 등록: 1000 → 64000, ×2
// =============================================================================

BENCHMARK(BM_Jam_AabbVsAabb)->RangeMultiplier(2)->Range(1000, 64000)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_DX_AabbVsAabb)->RangeMultiplier(2)->Range(1000, 64000)->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Jam_AabbVsPlane)->RangeMultiplier(2)->Range(1000, 64000)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_DX_AabbVsPlane)->RangeMultiplier(2)->Range(1000, 64000)->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Jam_AabbVsFrustum)->RangeMultiplier(2)->Range(1000, 64000)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_DX_AabbVsFrustum)->RangeMultiplier(2)->Range(1000, 64000)->Unit(benchmark::kMillisecond);

