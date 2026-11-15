#include "pch.h"
#include "../JamEngine/Octree.h"
#include <algorithm>
#include <random>

////////// helpers //////////

struct OctreeTestScene
{
    Entity CreateEntity()
    {
        return Entity { registry, registry.create(), UID { nextUid++ } };
    }

    entt::registry     registry;
    UInt32             nextUid = 0;
    HashMap<UID, Aabb> aabbs;
    Vector<Entity>     entities;
};

static Entity CreateEntity(OctreeTestScene& _scene, const Vec3 _center, const Vec3 _extents)
{
    Entity e                 = _scene.CreateEntity();
    _scene.aabbs[e.GetUID()] = Aabb { _center, _extents };
    _scene.entities.push_back(e);
    return e;
}

static void SetCenter(OctreeTestScene& _scene, const Entity _entity, const Vec3 _center)
{
    _scene.aabbs[_entity.GetUID()].center = _center;
}

static Aabb MakeWorldAabb(OctreeTestScene& _scene, const Entity _entity)
{
    return _scene.aabbs[_entity.GetUID()];
}

static void InsertAll(OctreeTestScene& _scene, Octree<std::monostate>& _octree)
{
    for (const Entity e: _scene.entities)
    {
        _octree.Insert(e, _scene.aabbs[e.GetUID()], {});
    }
}

static auto MakeAabbIntersect()
{
    return [](const Aabb& _bv, const Aabb& _nodeAabb) -> bool {
        return _bv.Overlap(_nodeAabb);
    };
}

static SizeT QueryCount(Octree<std::monostate>& _octree, const Aabb& _queryAabb)
{
    SizeT count = 0;
    _octree.Query(_queryAabb, MakeAabbIntersect(), [&](Entity, const std::monostate&) { ++count; });
    return count;
}

////////// Insert //////////

TEST(Octree, Insert_Single)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    InsertAll(scene, octree);

    EXPECT_EQ(octree.GetNumItems(), 1);
}

TEST(Octree, Insert_Multiple)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    for (int i = 0; i < 100; ++i)
    {
        CreateEntity(scene, Vec3(static_cast<float>(i) * 10.f, 0.f, 0.f), Vec3(1.f));
    }
    InsertAll(scene, octree);

    EXPECT_EQ(octree.GetNumItems(), 100);
}

TEST(Octree, Insert_AutoExpand)
{
    OctreeInfo desc      = {};
    desc.initBaseExtents = Vec3(64.f);

    OctreeTestScene        scene = {};
    Octree<std::monostate> octree { desc };

    CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    CreateEntity(scene, Vec3(9999.f), Vec3(1.f));
    InsertAll(scene, octree);

    EXPECT_EQ(octree.GetNumItems(), 2);
}

TEST(Octree, Insert_SamePosition)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    for (int i = 0; i < 10; ++i)
    {
        CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    }
    InsertAll(scene, octree);

    EXPECT_EQ(octree.GetNumItems(), 10);
}

TEST(Octree, Insert_TriggersSubdivision)
{
    OctreeInfo desc           = {};
    desc.subdivisionThreshold = 4;
    desc.initBaseExtents      = Vec3(512.f);

    OctreeTestScene        scene = {};
    Octree<std::monostate> octree { desc };

    for (int i = 0; i < 20; ++i)
    {
        CreateEntity(scene, Vec3(static_cast<float>(i) * 5.f, 0.f, 0.f), Vec3(1.f));
    }
    InsertAll(scene, octree);

    EXPECT_EQ(octree.GetNumItems(), 20);
}

TEST(Octree, Insert_VerySmallExtents)
{
    // extents 가 minBaseExtents 보다 훨씬 작은 아이템
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    for (int i = 0; i < 10; ++i)
    {
        CreateEntity(scene, Vec3(static_cast<float>(i) * 2.f, 0.f, 0.f), Vec3(0.01f));
    }
    InsertAll(scene, octree);

    EXPECT_EQ(octree.GetNumItems(), 10);
}

TEST(Octree, Insert_VeryLargeExtents)
{
    // 루트보다 큰 아이템 → 확장 후 루트에 위치
    OctreeInfo desc      = {};
    desc.initBaseExtents = Vec3(64.f);

    OctreeTestScene        scene = {};
    Octree<std::monostate> octree { desc };

    CreateEntity(scene, Vec3(0.f), Vec3(512.f));   // 루트보다 큼
    InsertAll(scene, octree);

    EXPECT_EQ(octree.GetNumItems(), 1);
}

TEST(Octree, Insert_OnOctreeBoundary)
{
    // 옥트리 경계면 위에 정확히 걸치는 아이템
    OctreeInfo desc      = {};
    desc.initBaseExtents = Vec3(128.f);
    desc.looseFactor     = 1.f;   // tight octree

    OctreeTestScene        scene = {};
    Octree<std::monostate> octree { desc };

    // 경계선(x=0) 위에 걸침
    CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    InsertAll(scene, octree);

    EXPECT_EQ(octree.GetNumItems(), 1);
}

TEST(Octree, Insert_AllOctants)
{
    // 8개 옥탄트 각각에 아이템 1개씩
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    const float d = 100.f;
    CreateEntity(scene, Vec3(-d, -d, -d), Vec3(1.f));
    CreateEntity(scene, Vec3(d, -d, -d), Vec3(1.f));
    CreateEntity(scene, Vec3(-d, d, -d), Vec3(1.f));
    CreateEntity(scene, Vec3(d, d, -d), Vec3(1.f));
    CreateEntity(scene, Vec3(-d, -d, d), Vec3(1.f));
    CreateEntity(scene, Vec3(d, -d, d), Vec3(1.f));
    CreateEntity(scene, Vec3(-d, d, d), Vec3(1.f));
    CreateEntity(scene, Vec3(d, d, d), Vec3(1.f));
    InsertAll(scene, octree);

    EXPECT_EQ(octree.GetNumItems(), 8);
}

TEST(Octree, Insert_MaxDepthRespected)
{
    OctreeInfo desc           = {};
    desc.maxDepth             = 2;
    desc.bUseMaxDepthPolicy   = true;
    desc.subdivisionThreshold = 1;
    desc.initBaseExtents      = Vec3(512.f);

    OctreeTestScene        scene = {};
    Octree<std::monostate> octree { desc };

    // maxDepth 초과하더라도 crash 없이 삽입
    for (int i = 0; i < 50; ++i)
    {
        CreateEntity(scene, Vec3(static_cast<float>(i) * 0.1f, 0.f, 0.f), Vec3(0.01f));
    }
    InsertAll(scene, octree);

    EXPECT_EQ(octree.GetNumItems(), 50);
}

////////// Remove //////////

TEST(Octree, Remove_Single)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    Entity e = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    InsertAll(scene, octree);

    octree.Remove(e);
    EXPECT_EQ(octree.GetNumItems(), 0);
}

TEST(Octree, Remove_All)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    Vector<Entity> entities = {};
    for (int i = 0; i < 50; ++i)
    {
        entities.push_back(CreateEntity(scene, Vec3(static_cast<float>(i) * 5.f, 0.f, 0.f), Vec3(1.f)));
    }
    InsertAll(scene, octree);

    for (const Entity e: entities)
    {
        octree.Remove(e);
    }
    EXPECT_EQ(octree.GetNumItems(), 0);
}

TEST(Octree, Remove_Middle)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    Vector<Entity> entities = {};
    for (int i = 0; i < 10; ++i)
    {
        entities.push_back(CreateEntity(scene, Vec3(static_cast<float>(i) * 10.f, 0.f, 0.f), Vec3(1.f)));
    }
    InsertAll(scene, octree);

    for (int i = 2; i < 7; ++i)
    {
        octree.Remove(entities[i]);
    }
    EXPECT_EQ(octree.GetNumItems(), 5);
}

TEST(Octree, Remove_ThenQueryReturnsNothing)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    Entity e = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    InsertAll(scene, octree);
    octree.Remove(e);

    const Aabb queryAabb = Aabb(Vec3(0.f), Vec3(9999.f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 0);
}

TEST(Octree, Remove_InsertAfterRemove)
{
    // 제거 후 같은 entity 재삽입
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    Entity     e  = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    const Aabb bv = MakeWorldAabb(scene, e);
    octree.Insert(e, bv, {});
    EXPECT_EQ(octree.GetNumItems(), 1);

    octree.Remove(e);
    EXPECT_EQ(octree.GetNumItems(), 0);

    octree.Insert(e, bv, {});
    EXPECT_EQ(octree.GetNumItems(), 1);
}

TEST(Octree, Remove_FromDifferentDepths)
{
    OctreeInfo desc           = {};
    desc.subdivisionThreshold = 2;
    desc.initBaseExtents      = Vec3(512.f);

    OctreeTestScene        scene = {};
    Octree<std::monostate> octree { desc };

    // 분할 유발해서 여러 깊이에 분산
    Vector<Entity> entities = {};
    for (int i = 0; i < 30; ++i)
    {
        entities.push_back(CreateEntity(scene, Vec3(static_cast<float>(i) * 3.f, 0.f, 0.f), Vec3(1.f)));
    }
    InsertAll(scene, octree);

    // 역순 제거
    for (int i = 29; i >= 0; --i)
    {
        octree.Remove(entities[i]);
        EXPECT_EQ(octree.GetNumItems(), static_cast<SizeT>(i)) << "i: " << i;
    }
}

////////// Update //////////

TEST(Octree, Update_StayInSameNode)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    Entity e = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    InsertAll(scene, octree);

    octree.Update(e, Aabb(Vec3(0.1f), Vec3(1.f)));
    EXPECT_EQ(octree.GetNumItems(), 1);
}

TEST(Octree, Update_MoveToAnotherSubtree)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    Entity e = CreateEntity(scene, Vec3(-200.f, 0.f, 0.f), Vec3(1.f));
    InsertAll(scene, octree);

    octree.Update(e, Aabb(Vec3(200.f, 0.f, 0.f), Vec3(1.f)));
    EXPECT_EQ(octree.GetNumItems(), 1);
}

TEST(Octree, Update_MoveOutsideRoot)
{
    OctreeInfo desc      = {};
    desc.initBaseExtents = Vec3(64.f);

    OctreeTestScene        scene = {};
    Octree<std::monostate> octree { desc };

    Entity e = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    InsertAll(scene, octree);

    octree.Update(e, Aabb(Vec3(99999.f), Vec3(1.f)));
    EXPECT_EQ(octree.GetNumItems(), 1);
}

TEST(Octree, Update_QueryReflectsNewPosition)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    Entity e = CreateEntity(scene, Vec3(-500.f, 0.f, 0.f), Vec3(1.f));
    InsertAll(scene, octree);

    const Aabb posX = Aabb(Vec3(500.f, 0.f, 0.f), Vec3(10.f));
    EXPECT_EQ(QueryCount(octree, posX), 0);

    octree.Update(e, Aabb(Vec3(500.f, 0.f, 0.f), Vec3(1.f)));
    EXPECT_EQ(QueryCount(octree, posX), 1);
}

TEST(Octree, Update_OldPositionNoLongerHit)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    Entity e = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    InsertAll(scene, octree);

    const Aabb oldRegion = Aabb(Vec3(0.f), Vec3(5.f));
    EXPECT_EQ(QueryCount(octree, oldRegion), 1);

    octree.Update(e, Aabb(Vec3(500.f, 0.f, 0.f), Vec3(1.f)));
    EXPECT_EQ(QueryCount(octree, oldRegion), 0);
}

TEST(Octree, Update_MultipleEntitiesNumItemsStable)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    constexpr int  k_count  = 30;
    Vector<Entity> entities = {};
    for (int i = 0; i < k_count; ++i)
    {
        entities.push_back(CreateEntity(scene, Vec3(static_cast<float>(i) * 5.f, 0.f, 0.f), Vec3(1.f)));
    }
    InsertAll(scene, octree);

    for (int frame = 0; frame < 20; ++frame)
    {
        for (int i = 0; i < k_count; ++i)
        {
            const float x = static_cast<float>(i) * 5.f + static_cast<float>(frame) * 3.f;
            octree.Update(entities[i], Aabb(Vec3(x, 0.f, 0.f), Vec3(1.f)));
        }
        EXPECT_EQ(octree.GetNumItems(), k_count) << "frame: " << frame;
    }
}

TEST(Octree, Update_BackAndForth)
{
    // A → B → A 반복 이동
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    Entity e = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    InsertAll(scene, octree);

    const Aabb posA = Aabb(Vec3(0.f, 0.f, 0.f), Vec3(1.f));
    const Aabb posB = Aabb(Vec3(500.f, 0.f, 0.f), Vec3(1.f));

    for (int i = 0; i < 10; ++i)
    {
        octree.Update(e, (i % 2 == 0) ? posB : posA);
        EXPECT_EQ(octree.GetNumItems(), 1) << "iter: " << i;
    }
}

TEST(Octree, Update_ExpandMultipleTimes)
{
    // 반복 확장을 유발하는 업데이트
    OctreeInfo desc      = {};
    desc.initBaseExtents = Vec3(32.f);

    OctreeTestScene        scene = {};
    Octree<std::monostate> octree { desc };

    Entity e = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    InsertAll(scene, octree);

    // 점점 더 멀리 이동
    for (int i = 1; i <= 5; ++i)
    {
        const float dist = static_cast<float>(i) * 10000.f;
        octree.Update(e, Aabb(Vec3(dist, 0.f, 0.f), Vec3(1.f)));
        EXPECT_EQ(octree.GetNumItems(), 1) << "step: " << i;
    }
}

////////// Query //////////

TEST(Octree, Query_HitAll)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    for (int i = 0; i < 10; ++i)
    {
        CreateEntity(scene, Vec3(static_cast<float>(i) * 2.f, 0.f, 0.f), Vec3(1.f));
    }
    InsertAll(scene, octree);

    const Aabb queryAabb = Aabb(Vec3(0.f), Vec3(9999.f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 10);
}

TEST(Octree, Query_HitNone)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    for (int i = 0; i < 10; ++i)
    {
        CreateEntity(scene, Vec3(static_cast<float>(i) * 2.f, 0.f, 0.f), Vec3(1.f));
    }
    InsertAll(scene, octree);

    const Aabb queryAabb = Aabb(Vec3(99999.f), Vec3(0.5f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 0);
}

TEST(Octree, Query_HitPartial)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    for (int i = 0; i < 5; ++i)
    {
        CreateEntity(scene, Vec3(-50.f + static_cast<float>(i) * 10.f, 0.f, 0.f), Vec3(1.f));   // 음수
        CreateEntity(scene, Vec3(50.f + static_cast<float>(i) * 10.f, 0.f, 0.f), Vec3(1.f));    // 양수
    }
    InsertAll(scene, octree);

    // 양의 x 영역만 [25, 125]
    const Aabb queryAabb = Aabb(Vec3(75.f, 0.f, 0.f), Vec3(50.f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 5);
}

TEST(Octree, Query_CorrectEntities)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    Entity eCenter = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    Entity eFar    = CreateEntity(scene, Vec3(500.f), Vec3(1.f));
    InsertAll(scene, octree);

    const Aabb     queryAabb = Aabb(Vec3(0.f), Vec3(5.f));
    Vector<Entity> hits      = {};
    octree.Query(queryAabb, MakeAabbIntersect(), [&](Entity _e, const std::monostate&) { hits.push_back(_e); });

    EXPECT_EQ(hits.size(), 1);
    EXPECT_EQ(hits[0], eCenter);
}

TEST(Octree, Query_EmptyOctree)
{
    Octree<std::monostate> octree {};

    const Aabb queryAabb = Aabb(Vec3(0.f), Vec3(9999.f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 0);
}

TEST(Octree, Query_TouchingBoundary)
{
    // 쿼리 aabb 와 아이템 aabb 가 접하는 경우 (Intersects 는 접함도 hit)
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    // 아이템: [9, 11] (center=10, ext=1)
    CreateEntity(scene, Vec3(10.f, 0.f, 0.f), Vec3(1.f));
    InsertAll(scene, octree);

    // 쿼리: [-1, 9] (center=4, ext=5) → x=9 에서 접함
    const Aabb queryAabb = Aabb(Vec3(4.f, 0.f, 0.f), Vec3(5.f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 1);
}

TEST(Octree, Query_JustOutsideBoundary)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    // 아이템: [9, 11]
    CreateEntity(scene, Vec3(10.f, 0.f, 0.f), Vec3(1.f));
    InsertAll(scene, octree);

    // 쿼리: [-1, 8.9] → 접하지 않음
    const Aabb queryAabb = Aabb(Vec3(3.95f, 0.f, 0.f), Vec3(4.95f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 0);
}

TEST(Octree, Query_LargeItemSmallQuery)
{
    // 아이템이 쿼리 영역보다 훨씬 큰 경우
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    CreateEntity(scene, Vec3(0.f), Vec3(500.f));   // 거대 아이템
    InsertAll(scene, octree);

    // 작은 쿼리 aabb 가 거대 아이템 내부에 완전히 들어가는 경우
    const Aabb queryAabb = Aabb(Vec3(0.f), Vec3(1.f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 1);
}

TEST(Octree, Query_NoFalsePositives)
{
    // 근처에 있지만 겹치지 않는 아이템들이 쿼리에 걸리지 않아야 함
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    // x 축 방향으로 촘촘히 배치 (간격 3, ext 1 → 인접하지 않음)
    for (int i = 0; i < 20; ++i)
    {
        CreateEntity(scene, Vec3(static_cast<float>(i) * 3.f, 0.f, 0.f), Vec3(1.f));
    }
    InsertAll(scene, octree);

    // 정중앙 하나만 잡히는 쿼리
    const Aabb queryAabb = Aabb(Vec3(0.f), Vec3(1.f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 1);
}

TEST(Octree, Query_3DIntersection)
{
    // 3축 모두 교차 조건 확인
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    CreateEntity(scene, Vec3(10.f, 10.f, 10.f), Vec3(1.f));    // 교차
    CreateEntity(scene, Vec3(10.f, 10.f, -10.f), Vec3(1.f));   // z 불일치
    CreateEntity(scene, Vec3(10.f, -10.f, 10.f), Vec3(1.f));   // y 불일치
    CreateEntity(scene, Vec3(-10.f, 10.f, 10.f), Vec3(1.f));   // x 불일치
    InsertAll(scene, octree);

    const Aabb queryAabb = Aabb(Vec3(10.f, 10.f, 10.f), Vec3(2.f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 1);
}

////////// ShrinkToFit //////////

TEST(Octree, ShrinkToFit_AfterRemoveAll)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    Vector<Entity> entities = {};
    for (int i = 0; i < 20; ++i)
    {
        entities.push_back(CreateEntity(scene, Vec3(static_cast<float>(i) * 5.f, 0.f, 0.f), Vec3(1.f)));
    }
    InsertAll(scene, octree);

    for (const Entity e: entities)
    {
        octree.Remove(e);
    }

    octree.ShrinkToFit();
    EXPECT_EQ(octree.GetNumItems(), 0);
}

TEST(Octree, ShrinkToFit_AfterExpand)
{
    OctreeInfo desc      = {};
    desc.initBaseExtents = Vec3(64.f);

    OctreeTestScene        scene = {};
    Octree<std::monostate> octree { desc };

    Entity eFar = CreateEntity(scene, Vec3(9999.f), Vec3(1.f));
    InsertAll(scene, octree);

    octree.Remove(eFar);
    octree.ShrinkToFit();
    EXPECT_EQ(octree.GetNumItems(), 0);
}

TEST(Octree, ShrinkToFit_ItemsStillQueryable)
{
    // shrink 후에도 남은 아이템이 쿼리에 잘 걸려야 함
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    Vector<Entity> entities = {};
    for (int i = 0; i < 20; ++i)
    {
        entities.push_back(CreateEntity(scene, Vec3(static_cast<float>(i) * 5.f, 0.f, 0.f), Vec3(1.f)));
    }
    InsertAll(scene, octree);

    // 절반 제거
    for (int i = 0; i < 10; ++i)
    {
        octree.Remove(entities[i]);
    }

    octree.ShrinkToFit();
    EXPECT_EQ(octree.GetNumItems(), 10);

    const Aabb queryAabb = Aabb(Vec3(0.f), Vec3(9999.f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 10);
}

TEST(Octree, ShrinkToFit_Idempotent)
{
    // 여러 번 호출해도 결과가 동일해야 함
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    for (int i = 0; i < 10; ++i)
    {
        CreateEntity(scene, Vec3(static_cast<float>(i) * 5.f, 0.f, 0.f), Vec3(1.f));
    }
    InsertAll(scene, octree);

    octree.ShrinkToFit();
    octree.ShrinkToFit();
    octree.ShrinkToFit();

    EXPECT_EQ(octree.GetNumItems(), 10);
}

////////// UserData //////////

TEST(Octree, UserData_StoreAndRetrieve)
{
    struct TagData
    {
        int tag = -1;
    };

    OctreeTestScene scene = {};
    Octree<TagData> octree {};

    Entity e = CreateEntity(scene, Vec3(0.f), Vec3(1.f));

    octree.Insert(e, MakeWorldAabb(scene, e), TagData { 42 });

    int foundTag = -1;
    octree.Query(
        Aabb(Vec3(0.f), Vec3(5.f)),
        MakeAabbIntersect(),
        [&](Entity, const TagData& _data) { foundTag = _data.tag; });

    EXPECT_EQ(foundTag, 42);
}

TEST(Octree, UserData_MultipleDistinct)
{
    struct IdData
    {
        int id = -1;
    };

    OctreeTestScene scene = {};
    Octree<IdData>  octree {};

    for (int i = 0; i < 5; ++i)
    {
        Entity e = CreateEntity(scene, Vec3(static_cast<float>(i) * 10.f, 0.f, 0.f), Vec3(1.f));
        octree.Insert(e, MakeWorldAabb(scene, e), IdData { i });
    }

    // id=2 위치만 쿼리
    Vector<int> foundIds = {};
    octree.Query(
        Aabb(Vec3(20.f, 0.f, 0.f), Vec3(2.f)),
        MakeAabbIntersect(),
        [&](Entity, const IdData& _data) { foundIds.push_back(_data.id); });

    EXPECT_EQ(foundIds.size(), 1);
    EXPECT_EQ(foundIds[0], 2);
}

////////// 실제 씬 시뮬레이션 //////////

TEST(Octree, Scene_DynamicMovement)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    constexpr int  k_count  = 50;
    Vector<Entity> entities = {};
    for (int i = 0; i < k_count; ++i)
    {
        entities.push_back(CreateEntity(scene, Vec3(static_cast<float>(i) * 3.f, 0.f, 0.f), Vec3(1.f)));
    }
    InsertAll(scene, octree);

    for (int frame = 0; frame < 50; ++frame)
    {
        const float offset = static_cast<float>(frame) * 0.5f;
        for (int i = 0; i < k_count; ++i)
        {
            const Vec3 newCenter = Vec3(static_cast<float>(i) * 3.f + offset, 0.f, 0.f);
            octree.Update(entities[i], Aabb(newCenter, Vec3(1.f)));
        }
        EXPECT_EQ(octree.GetNumItems(), k_count) << "frame: " << frame;
    }
}

TEST(Octree, Scene_SpawnAndDespawn)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    for (int wave = 0; wave < 5; ++wave)
    {
        Vector<Entity> waveEntities = {};
        const float    offset       = static_cast<float>(wave) * 100.f;

        for (int i = 0; i < 20; ++i)
        {
            Entity e = CreateEntity(scene, Vec3(offset + static_cast<float>(i) * 3.f, 0.f, 0.f), Vec3(1.f));
            waveEntities.push_back(e);
            octree.Insert(e, MakeWorldAabb(scene, e), {});
        }
        EXPECT_EQ(octree.GetNumItems(), 20) << "wave spawn: " << wave;

        for (const Entity e: waveEntities)
        {
            octree.Remove(e);
        }
        EXPECT_EQ(octree.GetNumItems(), 0) << "wave despawn: " << wave;
    }
}

TEST(Octree, Scene_MixedStaticDynamic)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    // 정적 오브젝트: 10x10 격자
    constexpr int k_staticCount = 100;
    for (int x = 0; x < 10; ++x)
    {
        for (int z = 0; z < 10; ++z)
        {
            CreateEntity(scene, Vec3(static_cast<float>(x) * 20.f, 0.f, static_cast<float>(z) * 20.f), Vec3(5.f));
        }
    }

    // 동적 오브젝트
    constexpr int  k_dynamicCount  = 10;
    Vector<Entity> dynamicEntities = {};
    for (int i = 0; i < k_dynamicCount; ++i)
    {
        dynamicEntities.push_back(CreateEntity(scene, Vec3(static_cast<float>(i) * 2.f, 0.f, 0.f), Vec3(1.f)));
    }

    InsertAll(scene, octree);
    EXPECT_EQ(octree.GetNumItems(), k_staticCount + k_dynamicCount);

    // 동적 오브젝트만 이동
    for (int frame = 0; frame < 10; ++frame)
    {
        for (int i = 0; i < k_dynamicCount; ++i)
        {
            const Vec3 newCenter = Vec3(static_cast<float>(i) * 2.f, static_cast<float>(frame) * 5.f, 0.f);
            octree.Update(dynamicEntities[i], Aabb(newCenter, Vec3(1.f)));
        }
        EXPECT_EQ(octree.GetNumItems(), k_staticCount + k_dynamicCount) << "frame: " << frame;
    }
}

TEST(Octree, Scene_ShrinkAfterMassRemove)
{
    // 대량 삽입 → 대량 제거 → shrink → 재삽입 사이클
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    for (int cycle = 0; cycle < 3; ++cycle)
    {
        Vector<Entity> entities = {};
        for (int i = 0; i < 50; ++i)
        {
            Entity e = CreateEntity(scene, Vec3(static_cast<float>(i) * 5.f, 0.f, 0.f), Vec3(1.f));
            entities.push_back(e);
            octree.Insert(e, MakeWorldAabb(scene, e), {});
        }
        EXPECT_EQ(octree.GetNumItems(), 50) << "cycle insert: " << cycle;

        for (const Entity e: entities)
        {
            octree.Remove(e);
        }
        octree.ShrinkToFit();
        EXPECT_EQ(octree.GetNumItems(), 0) << "cycle shrink: " << cycle;
    }
}

TEST(Octree, Scene_QueryConsistencyAfterManyUpdates)
{
    // 많은 업데이트 이후에도 쿼리 결과가 실제 위치와 일치하는지 확인
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    constexpr int  k_count  = 20;
    Vector<Entity> entities = {};
    for (int i = 0; i < k_count; ++i)
    {
        entities.push_back(CreateEntity(scene, Vec3(0.f), Vec3(1.f)));
    }
    InsertAll(scene, octree);

    // 모든 entity 를 서로 다른 위치로 분산
    for (int i = 0; i < k_count; ++i)
    {
        const Vec3 finalPos = Vec3(static_cast<float>(i) * 100.f, 0.f, 0.f);
        octree.Update(entities[i], Aabb(finalPos, Vec3(1.f)));
    }

    // 각 entity 위치를 개별 쿼리로 확인
    for (int i = 0; i < k_count; ++i)
    {
        const Vec3 pos       = Vec3(static_cast<float>(i) * 100.f, 0.f, 0.f);
        const Aabb queryAabb = Aabb(pos, Vec3(2.f));
        EXPECT_EQ(QueryCount(octree, queryAabb), 1) << "entity index: " << i;
    }
}

////////// helpers 추가 //////////

static float RandomFloat(const float _min, const float _max)
{
    static std::mt19937 s_rng { 12345 };
    return std::uniform_real_distribution<float> { _min, _max }(s_rng);
}

static Frustum MakeFrustum(const Vec3 _pos, const Vec3 _lookAt)
{
    const Mtx4 viewMatrix = Mtx4::MakeLookTo(_pos, (_lookAt - _pos).Normalized(), { 0.f, 1.f, 0.f });
    const Mtx4 projMatrix = Mtx4::MakePerspective(kHalfPi, 16.f / 9.f, 0.1f, 1000.f);
    return Frustum { viewMatrix * projMatrix }.Normalized();
}

static auto MakeFrustumIntersect()
{
    return [](const Frustum& _frustum, const Aabb& _nodeAabb) -> bool {
        return _frustum.Overlap(_nodeAabb);
    };
}

static SizeT QueryCountFrustum(Octree<std::monostate>& _octree, const Frustum& _frustum)
{
    SizeT count = 0;
    _octree.Query(_frustum, MakeFrustumIntersect(), [&](Entity, const std::monostate&) { ++count; });
    return count;
}

////////// Query_Frustum //////////

TEST(Octree, Query_Frustum_HitAll)
{
    // 모든 아이템이 frustum 안에 있는 경우
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    // 원점 근처에 촘촘히 배치
    for (int i = 0; i < 10; ++i)
    {
        CreateEntity(scene, Vec3(static_cast<float>(i) * 2.f, 0.f, 50.f), Vec3(1.f));
    }
    InsertAll(scene, octree);

    // 원점에서 +z 방향을 바라보는 frustum
    const Frustum frustum = MakeFrustum(Vec3(0.f, 0.f, -50.f), Vec3(0.f, 0.f, 0.f));
    EXPECT_EQ(QueryCountFrustum(octree, frustum), 10);
}

TEST(Octree, Query_Frustum_HitNone)
{
    // 모든 아이템이 frustum 밖에 있는 경우
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    // frustum 뒤쪽 (-z) 에 배치
    for (int i = 0; i < 10; ++i)
    {
        CreateEntity(scene, Vec3(static_cast<float>(i) * 2.f, 0.f, -500.f), Vec3(1.f));
    }
    InsertAll(scene, octree);

    const Frustum frustum = MakeFrustum(Vec3(0.f, 0.f, -50.f), Vec3(0.f, 0.f, 0.f));
    EXPECT_EQ(QueryCountFrustum(octree, frustum), 0);
}

TEST(Octree, Query_Frustum_HitPartial)
{
    // frustum 안팎에 각각 배치
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    // frustum 안: +z 방향 정면
    for (int i = 0; i < 5; ++i)
    {
        CreateEntity(scene, Vec3(static_cast<float>(i) * 2.f, 0.f, 50.f), Vec3(1.f));
    }
    // frustum 밖: 뒤쪽
    for (int i = 0; i < 5; ++i)
    {
        CreateEntity(scene, Vec3(static_cast<float>(i) * 2.f, 0.f, -500.f), Vec3(1.f));
    }
    InsertAll(scene, octree);

    const Frustum frustum = MakeFrustum(Vec3(0.f, 0.f, -50.f), Vec3(0.f, 0.f, 0.f));
    EXPECT_EQ(QueryCountFrustum(octree, frustum), 5);
}

TEST(Octree, Query_Frustum_EmptyOctree)
{
    Octree<std::monostate> octree {};

    const Frustum frustum = MakeFrustum(Vec3(0.f, 0.f, -50.f), Vec3(0.f, 0.f, 0.f));
    EXPECT_EQ(QueryCountFrustum(octree, frustum), 0);
}

TEST(Octree, Query_Frustum_ItemOnNearPlane)
{
    // near plane 에 걸치는 아이템
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    // near = 0.1f 이므로 카메라 바로 앞에 배치
    CreateEntity(scene, Vec3(0.f, 0.f, -49.8f), Vec3(1.f));
    InsertAll(scene, octree);

    const Frustum frustum = MakeFrustum(Vec3(0.f, 0.f, -50.f), Vec3(0.f, 0.f, 0.f));
    EXPECT_EQ(QueryCountFrustum(octree, frustum), 1);
}

TEST(Octree, Query_Frustum_ItemOnFarPlane)
{
    // far plane 에 걸치는 아이템 (far = 1000.f)
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    // 카메라(z=-50) + far(1000) = z=950 근처
    CreateEntity(scene, Vec3(0.f, 0.f, 948.f), Vec3(1.f));
    InsertAll(scene, octree);

    const Frustum frustum = MakeFrustum(Vec3(0.f, 0.f, -50.f), Vec3(0.f, 0.f, 0.f));
    EXPECT_EQ(QueryCountFrustum(octree, frustum), 1);
}

TEST(Octree, Query_Frustum_ItemBeyondFarPlane)
{
    // far plane 너머 아이템은 걸리지 않아야 함
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    // 카메라(z=-50) + far(1000) = z=950, 훨씬 넘어서 배치
    CreateEntity(scene, Vec3(0.f, 0.f, 9999.f), Vec3(1.f));
    InsertAll(scene, octree);

    const Frustum frustum = MakeFrustum(Vec3(0.f, 0.f, -50.f), Vec3(0.f, 0.f, 0.f));
    EXPECT_EQ(QueryCountFrustum(octree, frustum), 0);
}

TEST(Octree, Query_Frustum_ItemOutsideSidePlane)
{
    // 좌우 side plane 밖 아이템
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    // fov=90도, aspect=16/9 → 충분히 옆으로 치우치면 out
    CreateEntity(scene, Vec3(99999.f, 0.f, 50.f), Vec3(1.f));
    InsertAll(scene, octree);

    const Frustum frustum = MakeFrustum(Vec3(0.f, 0.f, -50.f), Vec3(0.f, 0.f, 0.f));
    EXPECT_EQ(QueryCountFrustum(octree, frustum), 0);
}

TEST(Octree, Query_Frustum_LargeItemIntersectsFrustum)
{
    // 거대 aabb 가 frustum 에 일부만 걸치는 경우
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    // frustum 밖에 중심이 있지만 크기가 커서 걸침
    CreateEntity(scene, Vec3(0.f, 0.f, 5000.f), Vec3(4200.f));
    InsertAll(scene, octree);

    const Frustum frustum = MakeFrustum(Vec3(0.f, 0.f, -50.f), Vec3(0.f, 0.f, 0.f));
    EXPECT_EQ(QueryCountFrustum(octree, frustum), 1);
}

TEST(Octree, Query_Frustum_CorrectEntities)
{
    // frustum 안에 있는 entity 가 정확히 반환되는지 확인
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    Entity eInside  = CreateEntity(scene, Vec3(0.f, 0.f, 50.f), Vec3(1.f));     // 안
    Entity eOutside = CreateEntity(scene, Vec3(0.f, 0.f, -500.f), Vec3(1.f));   // 밖
    InsertAll(scene, octree);

    const Frustum  frustum = MakeFrustum(Vec3(0.f, 0.f, -50.f), Vec3(0.f, 0.f, 0.f));
    Vector<Entity> hits    = {};
    octree.Query(frustum, MakeFrustumIntersect(), [&](Entity _e, const std::monostate&) { hits.push_back(_e); });

    EXPECT_EQ(hits.size(), 1);
    EXPECT_EQ(hits[0], eInside);
}

TEST(Octree, Query_Frustum_NoFalsePositives)
{
    // frustum 밖에 촘촘히 배치된 아이템이 오탐 없이 제외되는지 확인
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    // frustum 완전 뒤쪽에 빽빽하게 배치
    for (int x = 0; x < 5; ++x)
    {
        for (int y = 0; y < 5; ++y)
        {
            CreateEntity(scene, Vec3(static_cast<float>(x) * 5.f, static_cast<float>(y) * 5.f, -500.f), Vec3(1.f));
        }
    }
    InsertAll(scene, octree);

    const Frustum frustum = MakeFrustum(Vec3(0.f, 0.f, -50.f), Vec3(0.f, 0.f, 0.f));
    EXPECT_EQ(QueryCountFrustum(octree, frustum), 0);
}

TEST(Octree, Query_Frustum_AfterUpdate)
{
    // update 후 frustum 쿼리 결과가 즉시 반영되는지 확인
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    Entity e = CreateEntity(scene, Vec3(0.f, 0.f, -500.f), Vec3(1.f));   // 처음엔 밖
    InsertAll(scene, octree);

    const Frustum frustum = MakeFrustum(Vec3(0.f, 0.f, -50.f), Vec3(0.f, 0.f, 0.f));
    EXPECT_EQ(QueryCountFrustum(octree, frustum), 0);

    // frustum 안으로 이동
    octree.Update(e, Aabb(Vec3(0.f, 0.f, 50.f), Vec3(1.f)));
    EXPECT_EQ(QueryCountFrustum(octree, frustum), 1);

    // 다시 밖으로
    octree.Update(e, Aabb(Vec3(0.f, 0.f, -500.f), Vec3(1.f)));
    EXPECT_EQ(QueryCountFrustum(octree, frustum), 0);
}

TEST(Octree, Query_Frustum_MovingCamera)
{
    // 카메라(frustum) 가 이동하면서 다른 아이템을 포착하는 시나리오
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    // x 축 방향으로 분산 배치
    Vector<Entity> entities = {};
    for (int i = 0; i < 5; ++i)
    {
        entities.push_back(CreateEntity(scene, Vec3(static_cast<float>(i) * 200.f, 0.f, 50.f), Vec3(1.f)));
    }
    InsertAll(scene, octree);

    // 카메라를 x 축 방향으로 이동하면서 각 아이템 순서대로 포착
    for (int i = 0; i < 5; ++i)
    {
        const float   camX    = static_cast<float>(i) * 200.f;
        const Frustum frustum = MakeFrustum(Vec3(camX, 0.f, -50.f), Vec3(camX, 0.f, 0.f));
        EXPECT_GE(QueryCountFrustum(octree, frustum), 1) << "camera step: " << i;
    }
}

TEST(Octree, Query_Frustum_AabbVsFrustum_Consistency)
{
    // 선형 전수조사 결과와 옥트리 쿼리 결과가 일치하는지 검증
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    Vector<Aabb> aabbs = {};
    for (int i = 0; i < 50; ++i)
    {
        const Vec3 c = Vec3 {
            RandomFloat(-200.f, 200.f),
            RandomFloat(-200.f, 200.f),
            RandomFloat(-200.f, 200.f),
        };
        Entity e = CreateEntity(scene, c, Vec3(5.f));
        aabbs.push_back(MakeWorldAabb(scene, e));
    }
    InsertAll(scene, octree);

    const Frustum frustum = MakeFrustum(Vec3(0.f, 0.f, -50.f), Vec3(0.f, 0.f, 0.f));

    // 선형 전수조사
    SizeT linearCount = 0;
    for (const Aabb& aabb: aabbs)
    {
        if (frustum.Overlap(aabb))
        {
            ++linearCount;
        }
    }

    // 옥트리 쿼리
    const SizeT octreeCount = QueryCountFrustum(octree, frustum);

    EXPECT_EQ(octreeCount, linearCount);
}

////////// 핸들 / 일괄 빌드 / 3상태 쿼리 //////////

namespace
{

struct TrackedItem
{
    Entity           entity = Entity::kNull;
    Aabb             aabb   = {};
    OctreeItemHandle handle = kInvalidOctreeItemHandle;
};

Vec3 RandomPoint(const float _range)
{
    return Vec3 { RandomFloat(-_range, _range), RandomFloat(-_range, _range), RandomFloat(-_range, _range) };
}

Aabb RandomBox(const float _range, const float _maxExtent)
{
    return Aabb {
        RandomPoint(_range), Vec3 { RandomFloat(0.1f, _maxExtent), RandomFloat(0.1f, _maxExtent), RandomFloat(0.1f, _maxExtent) }
    };
}

Vector<UInt32> SortedUids(Vector<UInt32> _uids)
{
    std::sort(_uids.begin(), _uids.end());
    return _uids;
}

// 질의 Aabb에 대한 3상태 분류
eContainment ClassifyAabb(const Aabb& _query, const Aabb& _box)
{
    if (!_query.Overlap(_box))
    {
        return eContainment::Outside;
    }
    return _query.Contains(_box) ? eContainment::Inside : eContainment::Intersects;
}

Frustum RandomFrustum()
{
    const Vec3 pos = RandomPoint(200.f);
    Vec3       dir = RandomPoint(1.f);
    if (dir.LengthSq() < 0.01f)
    {
        dir = { 0.f, 0.f, 1.f };
    }
    return MakeFrustum(pos, pos + dir.Normalized() * 10.f);
}

// 옥트리 결과와 브루트포스 결과가 같은지 확인
template<typename TOctree>
void ExpectQueriesMatchBruteForce(const TOctree& _octree, const Vector<TrackedItem>& _items)
{
    EXPECT_EQ(_octree.GetNumItems(), _items.size());

    for (Int32 q = 0; q < 12; ++q)
    {
        const Aabb query = RandomBox(250.f, 120.f);

        Vector<UInt32> expected;
        for (const TrackedItem& item: _items)
        {
            if (query.Overlap(item.aabb))
            {
                expected.push_back(item.entity.GetUID().GetValue());
            }
        }

        Vector<UInt32> byBool;
        _octree.Query(
            query,
            [](const Aabb& _q, const Aabb& _box) { return _q.Overlap(_box); },
            [&](Entity _e, const std::monostate&) { byBool.push_back(_e.GetUID().GetValue()); });

        Vector<UInt32> byClassify;
        _octree.QueryClassified(
            query,
            [](const Aabb& _q, const Aabb& _box) { return ClassifyAabb(_q, _box); },
            [&](Entity _e, const std::monostate&) { byClassify.push_back(_e.GetUID().GetValue()); });

        EXPECT_EQ(SortedUids(byBool), SortedUids(expected)) << "aabb query " << q;
        EXPECT_EQ(SortedUids(byClassify), SortedUids(expected)) << "aabb classified query " << q;
    }

    for (Int32 q = 0; q < 6; ++q)
    {
        const Frustum frustum = RandomFrustum();

        Vector<UInt32> expected;
        for (const TrackedItem& item: _items)
        {
            if (frustum.Classify(item.aabb) != eContainment::Outside)
            {
                expected.push_back(item.entity.GetUID().GetValue());
            }
        }

        Vector<UInt32> byBool;
        _octree.Query(
            frustum,
            [](const Frustum& _f, const Aabb& _box) { return _f.Classify(_box) != eContainment::Outside; },
            [&](Entity _e, const std::monostate&) { byBool.push_back(_e.GetUID().GetValue()); });

        Vector<UInt32> byClassify;
        _octree.QueryClassified(
            frustum,
            [](const Frustum& _f, const Aabb& _box) { return _f.Classify(_box); },
            [&](Entity _e, const std::monostate&) { byClassify.push_back(_e.GetUID().GetValue()); });

        EXPECT_EQ(SortedUids(byBool), SortedUids(expected)) << "frustum query " << q;
        EXPECT_EQ(SortedUids(byClassify), SortedUids(expected)) << "frustum classified query " << q;
    }
}

}   // namespace

TEST(Octree, Handle_InsertReturnsDistinctValidHandles)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    Vector<OctreeItemHandle> handles;
    for (Int32 i = 0; i < 100; ++i)
    {
        const Entity e = CreateEntity(scene, RandomPoint(100.f), Vec3(1.f));
        handles.push_back(octree.Insert(e, MakeWorldAabb(scene, e), {}));
        EXPECT_EQ(octree.GetHandle(e), handles.back());
    }

    std::sort(handles.begin(), handles.end());
    EXPECT_EQ(std::adjacent_find(handles.begin(), handles.end()), handles.end());
    EXPECT_EQ(handles.front(), 0);
}

TEST(Octree, Handle_UpdateAndRemoveByHandle)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    const Entity e      = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    const auto   handle = octree.Insert(e, MakeWorldAabb(scene, e), {});

    octree.Update(handle, Aabb(Vec3(500.f, 0.f, 0.f), Vec3(1.f)));
    EXPECT_EQ(QueryCount(octree, Aabb(Vec3(0.f), Vec3(5.f))), 0);
    EXPECT_EQ(QueryCount(octree, Aabb(Vec3(500.f, 0.f, 0.f), Vec3(5.f))), 1);

    octree.Remove(handle);
    EXPECT_EQ(octree.GetNumItems(), 0);
    EXPECT_EQ(octree.GetHandle(e), kInvalidOctreeItemHandle);
}

TEST(Octree, Handle_ReusedAfterRemove)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    const Entity a       = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    const Entity b       = CreateEntity(scene, Vec3(10.f), Vec3(1.f));
    const auto   handleA = octree.Insert(a, MakeWorldAabb(scene, a), {});
    octree.Remove(handleA);

    const auto handleB = octree.Insert(b, MakeWorldAabb(scene, b), {});
    EXPECT_EQ(handleB, handleA);
    EXPECT_EQ(QueryCount(octree, Aabb(Vec3(10.f), Vec3(2.f))), 1);
    EXPECT_EQ(QueryCount(octree, Aabb(Vec3(0.f), Vec3(2.f))), 0);
}

TEST(Octree, Random_InsertUpdateRemove_MatchesBruteForce)
{
    OctreeInfo info           = {};
    info.initBaseExtents      = Vec3(64.f);
    info.subdivisionThreshold = 8;
    info.looseFactor          = 1.5f;

    OctreeTestScene        scene = {};
    Octree<std::monostate> octree { info };
    Vector<TrackedItem>    items;

    for (Int32 i = 0; i < 1500; ++i)
    {
        TrackedItem item;
        item.entity = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
        item.aabb   = RandomBox(300.f, 8.f);
        item.handle = octree.Insert(item.entity, item.aabb, {});
        items.push_back(item);
    }
    ExpectQueriesMatchBruteForce(octree, items);

    for (Int32 iter = 0; iter < 400; ++iter)
    {
        for (Int32 k = 0; k < 20; ++k)
        {
            const Int32 op = static_cast<Int32>(RandomFloat(0.f, 100.f));
            if (op < 60 && !items.empty())
            {
                TrackedItem& item = items[static_cast<SizeT>(RandomFloat(0.f, static_cast<float>(items.size()) - 0.01f))];
                if (RandomFloat(0.f, 1.f) < 0.8f)
                {
                    item.aabb.center += RandomPoint(2.f);   // 작은 이동
                }
                else
                {
                    item.aabb = RandomBox(300.f, 8.f);   // 순간이동
                }
                if (RandomFloat(0.f, 1.f) < 0.5f)
                {
                    octree.Update(item.handle, item.aabb);
                }
                else
                {
                    octree.Update(item.entity, item.aabb);
                }
            }
            else if (op < 80 && !items.empty())
            {
                const SizeT index = static_cast<SizeT>(RandomFloat(0.f, static_cast<float>(items.size()) - 0.01f));
                if (RandomFloat(0.f, 1.f) < 0.5f)
                {
                    octree.Remove(items[index].handle);
                }
                else
                {
                    octree.Remove(items[index].entity);
                }
                items[index] = items.back();
                items.pop_back();
            }
            else
            {
                TrackedItem item;
                item.entity = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
                item.aabb   = RandomBox(350.f, 8.f);
                item.handle = octree.Insert(item.entity, item.aabb, {});
                items.push_back(item);
            }
        }

        if (iter % 40 == 0)
        {
            ExpectQueriesMatchBruteForce(octree, items);
        }
    }
    ExpectQueriesMatchBruteForce(octree, items);

    octree.ShrinkToFit();
    ExpectQueriesMatchBruteForce(octree, items);
}

TEST(Octree, Build_MatchesBruteForce)
{
    OctreeInfo info           = {};
    info.initBaseExtents      = Vec3(128.f);
    info.subdivisionThreshold = 16;
    info.looseFactor          = 1.5f;

    OctreeTestScene                           scene = {};
    Octree<std::monostate>                    octree { info };
    Vector<TrackedItem>                       items;
    Vector<Octree<std::monostate>::BuildItem> buildItems;

    for (Int32 i = 0; i < 6000; ++i)
    {
        TrackedItem item;
        item.entity = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
        item.aabb   = RandomBox(i % 100 == 0 ? 600.f : 120.f, 6.f);   // 일부는 루트 밖
        items.push_back(item);
        buildItems.push_back({ item.entity, item.aabb, {} });
    }

    Vector<OctreeItemHandle> handles;
    octree.Build(buildItems, &handles);

    ASSERT_EQ(handles.size(), items.size());
    for (SizeT i = 0; i < items.size(); ++i)
    {
        EXPECT_EQ(handles[i], static_cast<OctreeItemHandle>(i));
        items[i].handle = handles[i];
        EXPECT_EQ(octree.GetHandle(items[i].entity), handles[i]);
    }
    ExpectQueriesMatchBruteForce(octree, items);

    // 빌드 후 갱신과 삭제
    for (Int32 k = 0; k < 2000; ++k)
    {
        TrackedItem& item = items[static_cast<SizeT>(RandomFloat(0.f, static_cast<float>(items.size()) - 0.01f))];
        item.aabb.center += RandomPoint(3.f);
        octree.Update(item.handle, item.aabb);
    }
    for (Int32 k = 0; k < 500; ++k)
    {
        octree.Remove(items.back().handle);
        items.pop_back();
    }
    ExpectQueriesMatchBruteForce(octree, items);
}

TEST(Octree, Build_ReplacesPreviousContent)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    const Entity old = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    octree.Insert(old, MakeWorldAabb(scene, old), {});

    Vector<Octree<std::monostate>::BuildItem> buildItems;
    for (Int32 i = 0; i < 10; ++i)
    {
        buildItems.push_back({ CreateEntity(scene, Vec3(0.f), Vec3(1.f)), RandomBox(50.f, 2.f), {} });
    }
    octree.Build(buildItems);

    EXPECT_EQ(octree.GetNumItems(), 10);
    EXPECT_EQ(octree.GetHandle(old), kInvalidOctreeItemHandle);
}

TEST(Octree, Build_Empty)
{
    Octree<std::monostate> octree {};
    octree.Build(Vector<Octree<std::monostate>::BuildItem> {});
    EXPECT_EQ(octree.GetNumItems(), 0);
}

TEST(Octree, Clear_ThenReuse)
{
    OctreeTestScene        scene = {};
    Octree<std::monostate> octree {};

    for (Int32 i = 0; i < 100; ++i)
    {
        const Entity e = CreateEntity(scene, RandomPoint(50.f), Vec3(1.f));
        octree.Insert(e, MakeWorldAabb(scene, e), {});
    }
    octree.Clear();
    EXPECT_EQ(octree.GetNumItems(), 0);

    const Entity again = CreateEntity(scene, Vec3(5.f), Vec3(1.f));
    octree.Insert(again, MakeWorldAabb(scene, again), {});
    EXPECT_EQ(octree.GetNumItems(), 1);
    EXPECT_EQ(QueryCount(octree, Aabb(Vec3(5.f), Vec3(2.f))), 1);
}

TEST(Octree, QueryClassified_InsideNodeCollectsWithoutItemTests)
{
    OctreeInfo info           = {};
    info.subdivisionThreshold = 4;
    info.initBaseExtents      = Vec3(64.f);

    OctreeTestScene        scene = {};
    Octree<std::monostate> octree { info };
    for (Int32 i = 0; i < 200; ++i)
    {
        const Entity e = CreateEntity(scene, RandomPoint(40.f), Vec3(1.f));
        octree.Insert(e, MakeWorldAabb(scene, e), {});
    }

    // 모든 노드 Inside 판정 시 아이템 판정 없이 전부 수집되어야 함
    Int32 numClassifyCalls = 0;
    Int32 numCollected     = 0;
    octree.QueryClassified(
        Aabb(Vec3(0.f), Vec3(10000.f)),
        [&](const Aabb&, const Aabb&) {
            ++numClassifyCalls;
            return eContainment::Inside;
        },
        [&](Entity, const std::monostate&) { ++numCollected; });

    EXPECT_EQ(numCollected, 200);
    EXPECT_EQ(numClassifyCalls, 1);   // 루트 하나만 판정
}

TEST(Frustum, Classify_ConsistentWithOverlap)
{
    for (Int32 f = 0; f < 20; ++f)
    {
        const Frustum frustum = RandomFrustum();
        for (Int32 i = 0; i < 300; ++i)
        {
            const Aabb box = RandomBox(250.f, 40.f);
            EXPECT_EQ(frustum.Classify(box) != eContainment::Outside, frustum.Overlap(box));
        }
    }
}

TEST(Frustum, Classify_InsideOutsideIntersects)
{
    const Frustum frustum = MakeFrustum(Vec3(0.f), Vec3(0.f, 0.f, 10.f));

    EXPECT_EQ(frustum.Classify(Aabb(Vec3(0.f, 0.f, 100.f), Vec3(1.f))), eContainment::Inside);
    EXPECT_EQ(frustum.Classify(Aabb(Vec3(0.f, 0.f, -100.f), Vec3(1.f))), eContainment::Outside);
    EXPECT_EQ(frustum.Classify(Aabb(Vec3(0.f, 0.f, 1000.f), Vec3(10.f))), eContainment::Intersects);   // 먼 평면에 걸침
    EXPECT_EQ(frustum.Classify(Aabb(Vec3(5000.f, 0.f, 100.f), Vec3(1.f))), eContainment::Outside);
}

TEST(Octree, Build_ThenEntityApi)
{
    OctreeInfo info           = {};
    info.initBaseExtents      = Vec3(128.f);
    info.subdivisionThreshold = 8;

    OctreeTestScene                           scene = {};
    Octree<std::monostate>                    octree { info };
    Vector<TrackedItem>                       items;
    Vector<Octree<std::monostate>::BuildItem> buildItems;

    for (Int32 i = 0; i < 1000; ++i)
    {
        TrackedItem item;
        item.entity = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
        item.aabb   = RandomBox(100.f, 4.f);
        item.handle = i;
        items.push_back(item);
        buildItems.push_back({ item.entity, item.aabb, {} });
    }
    octree.Build(buildItems);

    // 핸들 API 먼저. Entity 맵 생성 전 삭제
    for (Int32 k = 0; k < 100; ++k)
    {
        octree.Remove(items.back().handle);
        items.pop_back();
    }

    // 이후 첫 Entity API 호출 시 남은 아이템 기준으로 맵 생성되어야 함
    for (Int32 k = 0; k < 200; ++k)
    {
        TrackedItem& item = items[static_cast<SizeT>(RandomFloat(0.f, static_cast<float>(items.size()) - 0.01f))];
        item.aabb.center += RandomPoint(5.f);
        octree.Update(item.entity, item.aabb);
        EXPECT_EQ(octree.GetHandle(item.entity), item.handle);
    }

    for (Int32 k = 0; k < 100; ++k)
    {
        octree.Remove(items.back().entity);
        items.pop_back();
    }

    TrackedItem added;
    added.entity = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    added.aabb   = RandomBox(100.f, 4.f);
    added.handle = octree.Insert(added.entity, added.aabb, {});
    items.push_back(added);

    ExpectQueriesMatchBruteForce(octree, items);
}

TEST(Octree, Build_RemovedEntitiesAreUnknownToEntityApi)
{
    OctreeTestScene                           scene = {};
    Octree<std::monostate>                    octree {};
    Vector<Octree<std::monostate>::BuildItem> buildItems;

    for (Int32 i = 0; i < 50; ++i)
    {
        buildItems.push_back({ CreateEntity(scene, Vec3(0.f), Vec3(1.f)), RandomBox(50.f, 2.f), {} });
    }
    octree.Build(buildItems);

    octree.Remove(static_cast<OctreeItemHandle>(7));
    EXPECT_EQ(octree.GetHandle(buildItems[7].entity), kInvalidOctreeItemHandle);
    EXPECT_EQ(octree.GetHandle(buildItems[8].entity), 8);
    EXPECT_EQ(octree.GetNumItems(), 49);
}
