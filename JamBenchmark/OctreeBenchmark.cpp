#include "pch.h"
#include "../JamEngine/Octree.h"

// ============================================================
//  공통 헬퍼
// ============================================================

class BenchScene : public Scene
{
    JAM_SCENE_BODY(BenchScene);
};

struct AabbComponent
{
    JAM_COMPONENT_BODY(AabbComponent);
    Aabb aabb = {};
};

// 월드 공간 AABB 반환
static Aabb MakeWorldAabb(BenchScene& _scene, Entity _entity)
{
    const SceneNode&     node = _entity.GetComponent<SceneComponent>().node;
    const AabbComponent& comp = _entity.GetComponent<AabbComponent>();
    return comp.aabb.Transformed(node.GetWorldTransform().GetMatrix());
}

// 5000×5000×5000 월드에 _count 개의 Entity를 균일 분산하여 배치
// 크기는 [10, 100] 구간 랜덤
static std::vector<Entity> SpawnEntities(BenchScene& _scene, int _count)
{
    std::vector<Entity> entities;
    entities.reserve(_count);

    // 한 축당 격자 칸 수 (3D 격자 → cbrt)
    const int   gridN = static_cast<int>(std::ceil(std::cbrt(static_cast<double>(_count))));
    const float step  = 5000.f / static_cast<float>(gridN);
    const float half  = 5000.f * 0.5f;

    int spawned = 0;
    for (int xi = 0; xi < gridN && spawned < _count; ++xi)
    {
        for (int yi = 0; yi < gridN && spawned < _count; ++yi)
        {
            for (int zi = 0; zi < gridN && spawned < _count; ++zi)
            {
                // 격자 중심 + [-step/4, step/4] 지터(jitter) 로 완전 규칙 배열 방지
                const float jx = MakeRandom<float>(-step * 0.25f, step * 0.25f);
                const float jy = MakeRandom<float>(-step * 0.25f, step * 0.25f);
                const float jz = MakeRandom<float>(-step * 0.25f, step * 0.25f);

                const Vec3 center {
                    -half + (static_cast<float>(xi) + 0.5f) * step + jx,
                    -half + (static_cast<float>(yi) + 0.5f) * step + jy,
                    -half + (static_cast<float>(zi) + 0.5f) * step + jz,
                };

                // 크기: [10, 100] 균등 랜덤
                const float ext = MakeRandom<float>(10.f, 100.f);

                Entity         e    = _scene.CreateEntity();
                AabbComponent& comp = e.CreateComponent<AabbComponent>();
                comp.aabb           = Aabb::FromCenterExtents(Numbers<Vec3>::k_zero, Vec3(ext));
                e.ModifyWorldTransform().SetTranslation(center);

                entities.push_back(e);
                ++spawned;
            }
        }
    }
    return entities;
}

// 모든 Scene Entity 를 Octree 에 삽입
static void InsertAll(BenchScene& _scene, Octree<std::monostate>& _octree)
{
    for (auto&& [_, sc, aabb]: _scene.Each<SceneComponent, AabbComponent>())
    {
        const Aabb worldAabb = aabb.aabb.Transformed(sc.node.GetWorldTransform().GetMatrix());
        _octree.Insert(sc.node.GetOwner(), worldAabb, {});
    }
}

// Octree 생성에 쓸 기본 Desc (5000×5000×5000 월드 기준)
static OctreeDesc MakeWorldDesc()
{
    OctreeDesc desc;
    desc.initBaseExtents      = Vec3(5000.f);
    desc.minBaseExtents       = Vec3(10.f);
    desc.maxDepth             = 8;
    desc.subdivisionThreshold = 16;
    desc.looseFactor          = 2.f;
    return desc;
}

// ---- 교차 판정 람다 ----
static auto AabbIntersect()
{
    return [](const Aabb& _bv, const Aabb& _node) -> bool
    {
        return _bv.Intersects(_node);
    };
}

static auto FrustumIntersect()
{
    return [](const Frustum& _f, const Aabb& _node) -> bool
    {
        return _f.Intersects(_node) != ePlaneIntersectType::Outside;
    };
}

// 게임 카메라를 모방한 Frustum 생성
// near=0.5, far=500, fov=90°, aspect=16/9
static Frustum MakeGameFrustum(const Vec3 _pos, const Vec3 _lookAt)
{
    const Mtx4 view = Mtx4::CreateLookAt(_pos, _lookAt, Vec3(0.f, 1.f, 0.f));
    const Mtx4 proj = Mtx4::CreatePerspectiveFieldOfView(
        k_halfPi,     // fov 90°
        16.f / 9.f,   // aspect
        0.5f,         // near
        500.f);       // far
    return Frustum { view * proj };
}

// ============================================================
//  벤치마크 1 : AABB vs AABB  (캐릭터 충돌 쿼리)
//  캐릭터 AABB = 100×100×100,  월드 내 랜덤 위치
// ============================================================

// ---- 대조군 : 전수조사 ----
static void BM_AABB_BruteForce(benchmark::State& state)
{
    const int objectCount = static_cast<int>(state.range(0));

    BenchScene scene;

    const auto        entities = SpawnEntities(scene, objectCount);
    std::vector<Aabb> worldAabbs;
    worldAabbs.reserve(entities.size());
    for (const Entity e: entities)
        worldAabbs.push_back(MakeWorldAabb(scene, e));

    // 캐릭터 쿼리 AABB (100×100×100) 여러 위치 사전 생성
    constexpr int     kQueryCount = 64;
    std::vector<Aabb> queries;
    queries.reserve(kQueryCount);
    for (int i = 0; i < kQueryCount; ++i)
    {
        const Vec3 c {
            MakeRandom<float>(-2500.f, 2500.f),
            MakeRandom<float>(-2500.f, 2500.f),
            MakeRandom<float>(-2500.f, 2500.f),
        };
        queries.push_back(Aabb::FromCenterExtents(c, Vec3(100.f)));
    }

    int64_t totalHits = 0;
    int     qi        = 0;
    for (auto _: state)
    {
        const Aabb& q    = queries[qi++ % kQueryCount];
        int         hits = 0;
        for (const Aabb& aabb: worldAabbs)
        {
            if (q.Intersects(aabb))
                ++hits;
        }
        benchmark::DoNotOptimize(hits);
        totalHits += hits;
    }

    // 반복(iteration)당 평균 히트 수를 별도 컬럼으로 출력
    state.counters["hits_per_iter"] = benchmark::Counter(
        static_cast<double>(totalHits),
        benchmark::Counter::kAvgIterations);
    state.SetLabel("brute-force");
}

// ---- 실험군 : Octree ----
static void BM_AABB_Octree(benchmark::State& state)
{
    const int objectCount = static_cast<int>(state.range(0));

    BenchScene             scene;
    Octree<std::monostate> octree { MakeWorldDesc() };

    SpawnEntities(scene, objectCount);
    InsertAll(scene, octree);

    constexpr int     kQueryCount = 64;
    std::vector<Aabb> queries;
    queries.reserve(kQueryCount);
    for (int i = 0; i < kQueryCount; ++i)
    {
        const Vec3 c {
            MakeRandom<float>(-2500.f, 2500.f),
            MakeRandom<float>(-2500.f, 2500.f),
            MakeRandom<float>(-2500.f, 2500.f),
        };
        queries.push_back(Aabb::FromCenterExtents(c, Vec3(100.f)));
    }

    int64_t totalHits = 0;
    int     qi        = 0;
    for (auto _: state)
    {
        const Aabb& q    = queries[qi++ % kQueryCount];
        int         hits = 0;
        octree.Query(q, AabbIntersect(), [&](Entity, const std::monostate&)
                     { ++hits; });
        benchmark::DoNotOptimize(hits);
        totalHits += hits;
    }

    state.counters["hits_per_iter"] = benchmark::Counter(
        static_cast<double>(totalHits),
        benchmark::Counter::kAvgIterations);
    state.SetLabel("octree");
}

// ============================================================
//  벤치마크 2 : AABB vs Frustum  (카메라 컬링 쿼리)
//  near=0.5, far=500, fov=90°, aspect=16/9
// ============================================================

// ---- 대조군 : 전수조사 ----
static void BM_Frustum_BruteForce(benchmark::State& state)
{
    const int objectCount = static_cast<int>(state.range(0));

    BenchScene scene;

    const auto        entities = SpawnEntities(scene, objectCount);
    std::vector<Aabb> worldAabbs;
    worldAabbs.reserve(entities.size());
    for (const Entity e: entities)
        worldAabbs.push_back(MakeWorldAabb(scene, e));

    // 월드 내 임의 카메라 위치/방향 사전 생성
    constexpr int        kQueryCount = 32;
    std::vector<Frustum> frustums;
    frustums.reserve(kQueryCount);
    for (int i = 0; i < kQueryCount; ++i)
    {
        const Vec3 pos {
            MakeRandom<float>(-2000.f, 2000.f),
            MakeRandom<float>(-500.f, 500.f),
            MakeRandom<float>(-2000.f, 2000.f),
        };
        const Vec3 target {
            MakeRandom<float>(-2000.f, 2000.f),
            MakeRandom<float>(-500.f, 500.f),
            MakeRandom<float>(-2000.f, 2000.f),
        };
        frustums.push_back(MakeGameFrustum(pos, target));
    }

    int64_t totalHits = 0;
    int     qi        = 0;
    for (auto _: state)
    {
        const Frustum& f    = frustums[qi++ % kQueryCount];
        int            hits = 0;
        for (const Aabb& aabb: worldAabbs)
        {
            if (f.Intersects(aabb) != ePlaneIntersectType::Outside)
                ++hits;
        }
        benchmark::DoNotOptimize(hits);
        totalHits += hits;
    }

    state.counters["hits_per_iter"] = benchmark::Counter(
        static_cast<double>(totalHits),
        benchmark::Counter::kAvgIterations);
    state.SetLabel("brute-force");
}

// ---- 실험군 : Octree ----
static void BM_Frustum_Octree(benchmark::State& state)
{
    const int objectCount = static_cast<int>(state.range(0));

    BenchScene             scene;
    Octree<std::monostate> octree { MakeWorldDesc() };

    SpawnEntities(scene, objectCount);
    InsertAll(scene, octree);

    constexpr int        kQueryCount = 32;
    std::vector<Frustum> frustums;
    frustums.reserve(kQueryCount);
    for (int i = 0; i < kQueryCount; ++i)
    {
        const Vec3 pos {
            MakeRandom<float>(-2000.f, 2000.f),
            MakeRandom<float>(-500.f, 500.f),
            MakeRandom<float>(-2000.f, 2000.f),
        };
        const Vec3 target {
            MakeRandom<float>(-2000.f, 2000.f),
            MakeRandom<float>(-500.f, 500.f),
            MakeRandom<float>(-2000.f, 2000.f),
        };
        frustums.push_back(MakeGameFrustum(pos, target));
    }

    int64_t totalHits = 0;
    int     qi        = 0;
    for (auto _: state)
    {
        const Frustum& f    = frustums[qi++ % kQueryCount];
        int            hits = 0;
        octree.Query(f, FrustumIntersect(), [&](Entity, const std::monostate&)
                     { ++hits; });
        benchmark::DoNotOptimize(hits);
        totalHits += hits;
    }

    state.counters["hits_per_iter"] = benchmark::Counter(
        static_cast<double>(totalHits),
        benchmark::Counter::kAvgIterations);
    state.SetLabel("octree");
}

// ============================================================
//  벤치마크 3 : Octree 단독 — 생성(Insert)
//  씬에 Entity 를 미리 만들어 두고, Octree 생성+삽입 시간 측정
// ============================================================
static void BM_Octree_Build(benchmark::State& state)
{
    const int objectCount = static_cast<int>(state.range(0));

    BenchScene scene;
    const auto entities = SpawnEntities(scene, objectCount);

    std::vector<std::pair<Entity, Aabb>> data;
    data.reserve(entities.size());
    for (const Entity e: entities)
        data.emplace_back(e, MakeWorldAabb(scene, e));

    for (auto _: state)
    {
        Octree<std::monostate> octree { MakeWorldDesc() };
        for (auto& [e, aabb]: data)
            octree.Insert(e, aabb, {});
        benchmark::DoNotOptimize(octree.GetNumItems());
    }

    // 삽입 아이템 수는 매 반복 고정이므로 상수 기록
    state.counters["items_inserted"] = static_cast<double>(objectCount);
}

// ============================================================
//  벤치마크 4 : Octree 단독 — 삭제(Remove)
//  Octree 가 꽉 찬 상태에서 전체 삭제 시간 측정
// ============================================================
static void BM_Octree_RemoveAll(benchmark::State& state)
{
    const int objectCount = static_cast<int>(state.range(0));

    BenchScene scene;
    const auto entities = SpawnEntities(scene, objectCount);

    std::vector<std::pair<Entity, Aabb>> data;
    data.reserve(entities.size());
    for (const Entity e: entities)
        data.emplace_back(e, MakeWorldAabb(scene, e));

    for (auto _: state)
    {
        // Setup: 매 반복마다 새 Octree 빌드 (측정 제외)
        state.PauseTiming();
        Octree<std::monostate> octree { MakeWorldDesc() };
        for (auto& [e, aabb]: data)
            octree.Insert(e, aabb, {});
        state.ResumeTiming();

        // 측정: 전체 삭제
        for (auto& [e, aabb]: data)
            octree.Remove(e);
        benchmark::DoNotOptimize(octree.GetNumItems());
    }

    state.counters["items_removed"] = static_cast<double>(objectCount);
}

// ============================================================
//  벤치마크 5 : Octree 단독 — 일부 업데이트
//  매 프레임 100 / 500 / 1000 개 Entity 를 새 위치로 Update
//  (전체 오브젝트 수는 objectCount 고정, 업데이트 수는 arg(1))
// ============================================================
static void BM_Octree_PartialUpdate(benchmark::State& state)
{
    const int objectCount = static_cast<int>(state.range(0));
    const int updateCount = static_cast<int>(state.range(1));

    BenchScene             scene;
    Octree<std::monostate> octree { MakeWorldDesc() };

    const auto entities = SpawnEntities(scene, objectCount);
    InsertAll(scene, octree);

    // 업데이트할 Entity 수 (전체보다 크지 않게 클램프)
    const int actualUpdate = std::min(updateCount, objectCount);

    // 새 위치 목록 사전 생성 (64 세트 순환)
    constexpr int                  kSets = 64;
    std::vector<std::vector<Aabb>> newAabbSets(kSets);
    for (auto& set: newAabbSets)
    {
        set.reserve(actualUpdate);
        for (int i = 0; i < actualUpdate; ++i)
        {
            const Vec3 c {
                MakeRandom<float>(-2500.f, 2500.f),
                MakeRandom<float>(-2500.f, 2500.f),
                MakeRandom<float>(-2500.f, 2500.f),
            };
            const float ext = MakeRandom<float>(10.f, 100.f);
            set.push_back(Aabb::FromCenterExtents(c, Vec3(ext)));
        }
    }

    int setIdx = 0;
    for (auto _: state)
    {
        const auto& aabbs = newAabbSets[setIdx++ % kSets];
        for (int i = 0; i < actualUpdate; ++i)
            octree.Update(entities[i], aabbs[i]);
        benchmark::DoNotOptimize(octree.GetNumItems());
    }

    state.counters["items_updated"] = static_cast<double>(actualUpdate);
}

// ============================================================
//  오브젝트 수 범위: 1024 → 2048 → 4096 → 8192 → 16384
//  (이산적, 약 2배씩)
// ============================================================

#define OBJECT_COUNTS \
    ->Arg(1024)       \
        ->Arg(2048)   \
        ->Arg(4096)   \
        ->Arg(8192)   \
        ->Arg(16384)

// ---- AABB Query ----
BENCHMARK(BM_AABB_BruteForce)->Unit(benchmark::kMillisecond) OBJECT_COUNTS;
BENCHMARK(BM_AABB_Octree)->Unit(benchmark::kMillisecond) OBJECT_COUNTS;

// ---- Frustum Query ----
BENCHMARK(BM_Frustum_BruteForce)->Unit(benchmark::kMillisecond) OBJECT_COUNTS;
BENCHMARK(BM_Frustum_Octree)->Unit(benchmark::kMillisecond) OBJECT_COUNTS;

// ---- Octree 단독: 생성 / 삭제 ----
BENCHMARK(BM_Octree_Build)->Unit(benchmark::kMillisecond) OBJECT_COUNTS;
BENCHMARK(BM_Octree_RemoveAll)->Unit(benchmark::kMillisecond) OBJECT_COUNTS;

// ---- Octree 단독: 일부 업데이트 ----
//  (오브젝트 수, 업데이트 수) 조합
BENCHMARK(BM_Octree_PartialUpdate)
    ->Args({ 1024, 100 })
    ->Args({ 1024, 500 })
    ->Args({ 1024, 1000 })
    ->Args({ 4096, 100 })
    ->Args({ 4096, 500 })
    ->Args({ 4096, 1000 })
    ->Args({ 16384, 100 })
    ->Args({ 16384, 500 })
    ->Args({ 16384, 1000 })
    ->Unit(benchmark::kMillisecond);

/*
 
[ 디버그 ]

2026-03-20T12:05:30+09:00
Running C:\Users\Ahnjiwoo\Desktop\JamEngine\JamEngine\x64\Debug\JamBenchmark.exe
Run on (32 X 2495 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x16)
  L1 Instruction 32 KiB (x16)
  L2 Unified 1024 KiB (x16)
  L3 Unified 32768 KiB (x2)
***WARNING*** Library was built as DEBUG. Timings may be affected.
---------------------------------------------------------------------------------------------
Benchmark                                   Time             CPU   Iterations UserCounters...
---------------------------------------------------------------------------------------------
BM_AABB_BruteForce/1024                 0.004 ms        0.004 ms       179200 hits_per_iter=0.3125 brute-force
BM_AABB_BruteForce/2048                 0.008 ms        0.008 ms        89600 hits_per_iter=0.609375 brute-force
BM_AABB_BruteForce/4096                 0.015 ms        0.015 ms        44800 hits_per_iter=0.9375 brute-force
BM_AABB_BruteForce/8192                 0.031 ms        0.030 ms        23579 hits_per_iter=1.89058 brute-force
BM_AABB_BruteForce/16384                0.067 ms        0.068 ms         8960 hits_per_iter=3.92188 brute-force
BM_AABB_Octree/1024                     0.002 ms        0.002 ms       248889 hits_per_iter=0.296879 octree
BM_AABB_Octree/2048                     0.003 ms        0.003 ms       263529 hits_per_iter=0.499991 octree
BM_AABB_Octree/4096                     0.003 ms        0.003 ms       213333 hits_per_iter=1.12503 octree
BM_AABB_Octree/8192                     0.004 ms        0.004 ms       179200 hits_per_iter=1.92188 octree
BM_AABB_Octree/16384                    0.004 ms        0.004 ms       160000 hits_per_iter=4.1875 octree
BM_Frustum_BruteForce/1024              0.206 ms        0.204 ms         2987 hits_per_iter=7.15869 brute-force
BM_Frustum_BruteForce/2048              0.400 ms        0.394 ms         1545 hits_per_iter=13.7476 brute-force
BM_Frustum_BruteForce/4096              0.800 ms        0.802 ms          896 hits_per_iter=24.3438 brute-force
BM_Frustum_BruteForce/8192               1.66 ms         1.67 ms          448 hits_per_iter=57.625 brute-force
BM_Frustum_BruteForce/16384              3.38 ms         3.43 ms          187 hits_per_iter=109.513 brute-force
BM_Frustum_Octree/1024                  0.167 ms        0.165 ms         4073 hits_per_iter=7.78296 octree
BM_Frustum_Octree/2048                  0.244 ms        0.241 ms         2987 hits_per_iter=13.375 octree
BM_Frustum_Octree/4096                  0.324 ms        0.322 ms         2133 hits_per_iter=27.4079 octree
BM_Frustum_Octree/8192                  0.511 ms        0.516 ms         1000 hits_per_iter=57.326 octree
BM_Frustum_Octree/16384                 0.639 ms        0.642 ms         1120 hits_per_iter=109.094 octree
BM_Octree_Build/1024                     3.37 ms         3.37 ms          213 items_inserted=1.024k
BM_Octree_Build/2048                     6.28 ms         6.25 ms           90 items_inserted=2.048k
BM_Octree_Build/4096                     11.0 ms         11.0 ms           64 items_inserted=4.096k
BM_Octree_Build/8192                     26.3 ms         26.2 ms           28 items_inserted=8.192k
BM_Octree_Build/16384                    51.9 ms         52.6 ms           11 items_inserted=16.384k
BM_Octree_RemoveAll/1024                 1.23 ms         1.20 ms          560 items_removed=1.024k
BM_Octree_RemoveAll/2048                 2.38 ms         2.45 ms          345 items_removed=2.048k
BM_Octree_RemoveAll/4096                 4.67 ms         8.44 ms          100 items_removed=4.096k
BM_Octree_RemoveAll/8192                 9.66 ms         8.54 ms           64 items_removed=8.192k
BM_Octree_RemoveAll/16384                19.3 ms         17.7 ms           37 items_removed=16.384k
BM_Octree_PartialUpdate/1024/100        0.115 ms        0.115 ms         6400 items_updated=100
BM_Octree_PartialUpdate/1024/500        0.578 ms        0.586 ms         1120 items_updated=500
BM_Octree_PartialUpdate/1024/1000        1.16 ms         1.15 ms          640 items_updated=1k
BM_Octree_PartialUpdate/4096/100        0.115 ms        0.117 ms         6400 items_updated=100
BM_Octree_PartialUpdate/4096/500        0.583 ms        0.578 ms         1000 items_updated=500
BM_Octree_PartialUpdate/4096/1000        1.18 ms         1.17 ms          560 items_updated=1k
BM_Octree_PartialUpdate/16384/100       0.122 ms        0.120 ms         5600 items_updated=100
BM_Octree_PartialUpdate/16384/500       0.605 ms        0.600 ms         1120 items_updated=500
BM_Octree_PartialUpdate/16384/1000       1.23 ms         1.23 ms          560 items_updated=1k

[ 릴리즈 ]

2026-03-20T12:07:15+09:00
Running C:\Users\Ahnjiwoo\Desktop\JamEngine\JamEngine\x64\Release\JamBenchmark.exe
Run on (32 X 2495 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x16)
  L1 Instruction 32 KiB (x16)
  L2 Unified 1024 KiB (x16)
  L3 Unified 32768 KiB (x2)
---------------------------------------------------------------------------------------------
Benchmark                                   Time             CPU   Iterations UserCounters...
---------------------------------------------------------------------------------------------
BM_AABB_BruteForce/1024                 0.001 ms        0.001 ms      1120000 hits_per_iter=0.3125 brute-force
BM_AABB_BruteForce/2048                 0.002 ms        0.002 ms       344615 hits_per_iter=0.484381 brute-force
BM_AABB_BruteForce/4096                 0.003 ms        0.003 ms       248889 hits_per_iter=0.968757 brute-force
BM_AABB_BruteForce/8192                 0.006 ms        0.006 ms       112000 hits_per_iter=1.875 brute-force
BM_AABB_BruteForce/16384                0.015 ms        0.015 ms        44800 hits_per_iter=3.70312 brute-force
BM_AABB_Octree/1024                     0.000 ms        0.000 ms      1792000 hits_per_iter=0.25 octree
BM_AABB_Octree/2048                     0.000 ms        0.000 ms      1792000 hits_per_iter=0.484375 octree
BM_AABB_Octree/4096                     0.001 ms        0.001 ms      1000000 hits_per_iter=0.96875 octree
BM_AABB_Octree/8192                     0.001 ms        0.001 ms       896000 hits_per_iter=1.82812 octree
BM_AABB_Octree/16384                    0.001 ms        0.001 ms      1120000 hits_per_iter=4.14062 octree
BM_Frustum_BruteForce/1024              0.013 ms        0.013 ms        56000 hits_per_iter=7.5625 brute-force
BM_Frustum_BruteForce/2048              0.035 ms        0.035 ms        20364 hits_per_iter=13.5314 brute-force
BM_Frustum_BruteForce/4096              0.068 ms        0.068 ms        11200 hits_per_iter=25.0625 brute-force
BM_Frustum_BruteForce/8192              0.138 ms        0.140 ms         5600 hits_per_iter=57.3438 brute-force
BM_Frustum_BruteForce/16384             0.273 ms        0.267 ms         2635 hits_per_iter=112.844 brute-force
BM_Frustum_Octree/1024                  0.012 ms        0.012 ms        56000 hits_per_iter=7.6875 octree
BM_Frustum_Octree/2048                  0.013 ms        0.013 ms        40727 hits_per_iter=13.5939 octree
BM_Frustum_Octree/4096                  0.018 ms        0.018 ms        37333 hits_per_iter=26.5312 octree
BM_Frustum_Octree/8192                  0.045 ms        0.045 ms        17231 hits_per_iter=55.7188 octree
BM_Frustum_Octree/16384                 0.060 ms        0.060 ms        17920 hits_per_iter=109.469 octree
BM_Octree_Build/1024                    0.254 ms        0.255 ms         2635 items_inserted=1.024k
BM_Octree_Build/2048                    0.371 ms        0.375 ms         1792 items_inserted=2.048k
BM_Octree_Build/4096                    0.726 ms        0.725 ms         1120 items_inserted=4.096k
BM_Octree_Build/8192                     2.05 ms         2.05 ms          320 items_inserted=8.192k
BM_Octree_Build/16384                    4.07 ms         4.06 ms          154 items_inserted=16.384k
BM_Octree_RemoveAll/1024                0.061 ms        0.070 ms        10000 items_removed=1.024k
BM_Octree_RemoveAll/2048                0.110 ms        0.114 ms         5600 items_removed=2.048k
BM_Octree_RemoveAll/4096                0.169 ms        0.184 ms         4073 items_removed=4.096k
BM_Octree_RemoveAll/8192                0.349 ms        0.339 ms         1659 items_removed=8.192k
BM_Octree_RemoveAll/16384               0.661 ms        0.669 ms          747 items_removed=16.384k
BM_Octree_PartialUpdate/1024/100        0.003 ms        0.003 ms       235789 items_updated=100
BM_Octree_PartialUpdate/1024/500        0.024 ms        0.024 ms        29867 items_updated=500
BM_Octree_PartialUpdate/1024/1000       0.049 ms        0.049 ms        14452 items_updated=1k
BM_Octree_PartialUpdate/4096/100        0.003 ms        0.003 ms       235789 items_updated=100
BM_Octree_PartialUpdate/4096/500        0.026 ms        0.026 ms        28000 items_updated=500
BM_Octree_PartialUpdate/4096/1000       0.054 ms        0.054 ms        11200 items_updated=1k
BM_Octree_PartialUpdate/16384/100       0.004 ms        0.004 ms       172308 items_updated=100
BM_Octree_PartialUpdate/16384/500       0.030 ms        0.030 ms        21333 items_updated=500
BM_Octree_PartialUpdate/16384/1000      0.059 ms        0.059 ms        11200 items_updated=1k

Debug vs Release 전체 배율 요약
항목 (16k 기준)	        Debug	        Release	        배율	4.17ms  예산 점유 (Rel)
AABB BruteForce	        0.067 ms	0.015 ms	4.5×	0.36%
AABB Octree	        0.004 ms	0.001 ms	4.0×	<0.03%
Frustum BruteForce	3.38 ms	        0.273 ms	12.4×	6.5%
Frustum Octree	        0.639 ms	0.060 ms	10.7×	1.4%
Build	                51.9 ms	        4.07 ms	        12.7×	97.6% ⚠
RemoveAll	        19.3 ms	        0.661 ms	29.2×	15.8%
PartialUpdate 100개	0.122 ms	0.004 ms	30.5×	0.10%
PartialUpdate 1000개	1.23 ms	        0.059 ms	20.8×	1.4%


 */
