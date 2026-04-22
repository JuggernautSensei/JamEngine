#include "pch.h"
#include "../JamEngine/Octree.h"

////////// helpers //////////

class OctreeTestScene : public Scene
{
    JAM_SCENE_BODY(OctreeTestScene);
};

struct AabbComponent
{
    JAM_COMPONENT_BODY(AabbComponent);
    Aabb aabb = {};
};

static Entity CreateEntity(OctreeTestScene& _scene, const Vec3 _center, const Vec3 _extents)
{
    Entity         e    = _scene.CreateEntity();
    AabbComponent& comp = e.CreateComponent<AabbComponent>();
    comp.aabb           = Aabb::FromCenterExtents(Vec3::k_zero, _extents);
    e.ModifyWorldTransform().SetTranslation(_center);
    return e;
}

static Aabb MakeWorldAabb(OctreeTestScene& _scene, const Entity _entity)
{
    const SceneNode&     node = _entity.GetComponent<SceneComponent>().node;
    const AabbComponent& comp = _entity.GetComponent<AabbComponent>();
    return comp.aabb.Transform(node.GetWorldTransform().GetSRT());
}

static void InsertAll(OctreeTestScene& _scene, Octree<std::monostate>& _octree)
{
    for (auto&& [_, sc, aabb]: _scene.Each<SceneComponent, AabbComponent>())
    {
        const Aabb aabbW = aabb.aabb.Transform(sc.node.GetWorldTransform().GetSRT());
        _octree.Insert(sc.node.GetOwner(), aabbW, {});
    }
}

static auto MakeAabbIntersect()
{
    return [](const Aabb& _bv, const Aabb& _nodeAabb) -> bool
    {
        return _bv.Intersects(_nodeAabb);
    };
}

static SizeT QueryCount(Octree<std::monostate>& _octree, const Aabb& _queryAabb)
{
    SizeT count = 0;
    _octree.Query(_queryAabb, MakeAabbIntersect(), [&](Entity, const std::monostate&)
                  { ++count; });
    return count;
}

////////// Insert //////////

TEST(Octree, Insert_Single)
{
    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree {};

    CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    InsertAll(scene, octree);

    EXPECT_EQ(octree.GetNumItems(), 1);
}

TEST(Octree, Insert_Multiple)
{
    OctreeTestScene        scene  = {};
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
    OctreeDesc desc      = {};
    desc.initBaseExtents = Vec3(64.f);

    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree { desc };

    CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    CreateEntity(scene, Vec3(9999.f), Vec3(1.f));
    InsertAll(scene, octree);

    EXPECT_EQ(octree.GetNumItems(), 2);
}

TEST(Octree, Insert_SamePosition)
{
    OctreeTestScene        scene  = {};
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
    OctreeDesc desc           = {};
    desc.subdivisionThreshold = 4;
    desc.initBaseExtents      = Vec3(512.f);

    OctreeTestScene        scene  = {};
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
    OctreeTestScene        scene  = {};
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
    OctreeDesc desc      = {};
    desc.initBaseExtents = Vec3(64.f);

    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree { desc };

    CreateEntity(scene, Vec3(0.f), Vec3(512.f));   // 루트보다 큼
    InsertAll(scene, octree);

    EXPECT_EQ(octree.GetNumItems(), 1);
}

TEST(Octree, Insert_OnOctreeBoundary)
{
    // 옥트리 경계면 위에 정확히 걸치는 아이템
    OctreeDesc desc      = {};
    desc.initBaseExtents = Vec3(128.f);
    desc.looseFactor     = 1.f;   // tight octree

    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree { desc };

    // 경계선(x=0) 위에 걸침
    CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    InsertAll(scene, octree);

    EXPECT_EQ(octree.GetNumItems(), 1);
}

TEST(Octree, Insert_AllOctants)
{
    // 8개 옥탄트 각각에 아이템 1개씩
    OctreeTestScene        scene  = {};
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
    OctreeDesc desc           = {};
    desc.maxDepth             = 2;
    desc.bUseMaxDepthPolicy   = true;
    desc.subdivisionThreshold = 1;
    desc.initBaseExtents      = Vec3(512.f);

    OctreeTestScene        scene  = {};
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
    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree {};

    Entity e = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    InsertAll(scene, octree);

    octree.Remove(e);
    EXPECT_EQ(octree.GetNumItems(), 0);
}

TEST(Octree, Remove_All)
{
    OctreeTestScene        scene  = {};
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
    OctreeTestScene        scene  = {};
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
    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree {};

    Entity e = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    InsertAll(scene, octree);
    octree.Remove(e);

    const Aabb queryAabb = Aabb::FromCenterExtents(Vec3(0.f), Vec3(9999.f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 0);
}

TEST(Octree, Remove_InsertAfterRemove)
{
    // 제거 후 같은 entity 재삽입
    OctreeTestScene        scene  = {};
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
    OctreeDesc desc           = {};
    desc.subdivisionThreshold = 2;
    desc.initBaseExtents      = Vec3(512.f);

    OctreeTestScene        scene  = {};
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
    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree {};

    Entity e = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    InsertAll(scene, octree);

    octree.Update(e, Aabb::FromCenterExtents(Vec3(0.1f), Vec3(1.f)));
    EXPECT_EQ(octree.GetNumItems(), 1);
}

TEST(Octree, Update_MoveToAnotherSubtree)
{
    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree {};

    Entity e = CreateEntity(scene, Vec3(-200.f, 0.f, 0.f), Vec3(1.f));
    InsertAll(scene, octree);

    octree.Update(e, Aabb::FromCenterExtents(Vec3(200.f, 0.f, 0.f), Vec3(1.f)));
    EXPECT_EQ(octree.GetNumItems(), 1);
}

TEST(Octree, Update_MoveOutsideRoot)
{
    OctreeDesc desc      = {};
    desc.initBaseExtents = Vec3(64.f);

    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree { desc };

    Entity e = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    InsertAll(scene, octree);

    octree.Update(e, Aabb::FromCenterExtents(Vec3(99999.f), Vec3(1.f)));
    EXPECT_EQ(octree.GetNumItems(), 1);
}

TEST(Octree, Update_QueryReflectsNewPosition)
{
    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree {};

    Entity e = CreateEntity(scene, Vec3(-500.f, 0.f, 0.f), Vec3(1.f));
    InsertAll(scene, octree);

    const Aabb posX = Aabb::FromCenterExtents(Vec3(500.f, 0.f, 0.f), Vec3(10.f));
    EXPECT_EQ(QueryCount(octree, posX), 0);

    octree.Update(e, Aabb::FromCenterExtents(Vec3(500.f, 0.f, 0.f), Vec3(1.f)));
    EXPECT_EQ(QueryCount(octree, posX), 1);
}

TEST(Octree, Update_OldPositionNoLongerHit)
{
    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree {};

    Entity e = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    InsertAll(scene, octree);

    const Aabb oldRegion = Aabb::FromCenterExtents(Vec3(0.f), Vec3(5.f));
    EXPECT_EQ(QueryCount(octree, oldRegion), 1);

    octree.Update(e, Aabb::FromCenterExtents(Vec3(500.f, 0.f, 0.f), Vec3(1.f)));
    EXPECT_EQ(QueryCount(octree, oldRegion), 0);
}

TEST(Octree, Update_MultipleEntitiesNumItemsStable)
{
    OctreeTestScene        scene  = {};
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
            octree.Update(entities[i], Aabb::FromCenterExtents(Vec3(x, 0.f, 0.f), Vec3(1.f)));
        }
        EXPECT_EQ(octree.GetNumItems(), k_count) << "frame: " << frame;
    }
}

TEST(Octree, Update_BackAndForth)
{
    // A → B → A 반복 이동
    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree {};

    Entity e = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    InsertAll(scene, octree);

    const Aabb posA = Aabb::FromCenterExtents(Vec3(0.f, 0.f, 0.f), Vec3(1.f));
    const Aabb posB = Aabb::FromCenterExtents(Vec3(500.f, 0.f, 0.f), Vec3(1.f));

    for (int i = 0; i < 10; ++i)
    {
        octree.Update(e, (i % 2 == 0) ? posB : posA);
        EXPECT_EQ(octree.GetNumItems(), 1) << "iter: " << i;
    }
}

TEST(Octree, Update_ExpandMultipleTimes)
{
    // 반복 확장을 유발하는 업데이트
    OctreeDesc desc      = {};
    desc.initBaseExtents = Vec3(32.f);

    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree { desc };

    Entity e = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    InsertAll(scene, octree);

    // 점점 더 멀리 이동
    for (int i = 1; i <= 5; ++i)
    {
        const float dist = static_cast<float>(i) * 10000.f;
        octree.Update(e, Aabb::FromCenterExtents(Vec3(dist, 0.f, 0.f), Vec3(1.f)));
        EXPECT_EQ(octree.GetNumItems(), 1) << "step: " << i;
    }
}

////////// Query //////////

TEST(Octree, Query_HitAll)
{
    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree {};

    for (int i = 0; i < 10; ++i)
    {
        CreateEntity(scene, Vec3(static_cast<float>(i) * 2.f, 0.f, 0.f), Vec3(1.f));
    }
    InsertAll(scene, octree);

    const Aabb queryAabb = Aabb::FromCenterExtents(Vec3(0.f), Vec3(9999.f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 10);
}

TEST(Octree, Query_HitNone)
{
    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree {};

    for (int i = 0; i < 10; ++i)
    {
        CreateEntity(scene, Vec3(static_cast<float>(i) * 2.f, 0.f, 0.f), Vec3(1.f));
    }
    InsertAll(scene, octree);

    const Aabb queryAabb = Aabb::FromCenterExtents(Vec3(99999.f), Vec3(0.5f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 0);
}

TEST(Octree, Query_HitPartial)
{
    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree {};

    for (int i = 0; i < 5; ++i)
    {
        CreateEntity(scene, Vec3(-50.f + static_cast<float>(i) * 10.f, 0.f, 0.f), Vec3(1.f));   // 음수
        CreateEntity(scene, Vec3(50.f + static_cast<float>(i) * 10.f, 0.f, 0.f), Vec3(1.f));    // 양수
    }
    InsertAll(scene, octree);

    // 양의 x 영역만 [25, 125]
    const Aabb queryAabb = Aabb::FromCenterExtents(Vec3(75.f, 0.f, 0.f), Vec3(50.f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 5);
}

TEST(Octree, Query_CorrectEntities)
{
    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree {};

    Entity eCenter = CreateEntity(scene, Vec3(0.f), Vec3(1.f));
    Entity eFar    = CreateEntity(scene, Vec3(500.f), Vec3(1.f));
    InsertAll(scene, octree);

    const Aabb     queryAabb = Aabb::FromCenterExtents(Vec3(0.f), Vec3(5.f));
    Vector<Entity> hits      = {};
    octree.Query(queryAabb, MakeAabbIntersect(), [&](Entity _e, const std::monostate&)
                 { hits.push_back(_e); });

    EXPECT_EQ(hits.size(), 1);
    EXPECT_EQ(hits[0], eCenter);
}

TEST(Octree, Query_EmptyOctree)
{
    Octree<std::monostate> octree {};

    const Aabb queryAabb = Aabb::FromCenterExtents(Vec3(0.f), Vec3(9999.f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 0);
}

TEST(Octree, Query_TouchingBoundary)
{
    // 쿼리 aabb 와 아이템 aabb 가 접하는 경우 (Intersects 는 접함도 hit)
    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree {};

    // 아이템: [9, 11] (center=10, ext=1)
    CreateEntity(scene, Vec3(10.f, 0.f, 0.f), Vec3(1.f));
    InsertAll(scene, octree);

    // 쿼리: [-1, 9] (center=4, ext=5) → x=9 에서 접함
    const Aabb queryAabb = Aabb::FromCenterExtents(Vec3(4.f, 0.f, 0.f), Vec3(5.f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 1);
}

TEST(Octree, Query_JustOutsideBoundary)
{
    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree {};

    // 아이템: [9, 11]
    CreateEntity(scene, Vec3(10.f, 0.f, 0.f), Vec3(1.f));
    InsertAll(scene, octree);

    // 쿼리: [-1, 8.9] → 접하지 않음
    const Aabb queryAabb = Aabb::FromCenterExtents(Vec3(3.95f, 0.f, 0.f), Vec3(4.95f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 0);
}

TEST(Octree, Query_LargeItemSmallQuery)
{
    // 아이템이 쿼리 영역보다 훨씬 큰 경우
    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree {};

    CreateEntity(scene, Vec3(0.f), Vec3(500.f));   // 거대 아이템
    InsertAll(scene, octree);

    // 작은 쿼리 aabb 가 거대 아이템 내부에 완전히 들어가는 경우
    const Aabb queryAabb = Aabb::FromCenterExtents(Vec3(0.f), Vec3(1.f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 1);
}

TEST(Octree, Query_NoFalsePositives)
{
    // 근처에 있지만 겹치지 않는 아이템들이 쿼리에 걸리지 않아야 함
    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree {};

    // x 축 방향으로 촘촘히 배치 (간격 3, ext 1 → 인접하지 않음)
    for (int i = 0; i < 20; ++i)
    {
        CreateEntity(scene, Vec3(static_cast<float>(i) * 3.f, 0.f, 0.f), Vec3(1.f));
    }
    InsertAll(scene, octree);

    // 정중앙 하나만 잡히는 쿼리
    const Aabb queryAabb = Aabb::FromCenterExtents(Vec3(0.f), Vec3(1.f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 1);
}

TEST(Octree, Query_3DIntersection)
{
    // 3축 모두 교차 조건 확인
    OctreeTestScene        scene  = {};
    Octree<std::monostate> octree {};

    CreateEntity(scene, Vec3(10.f, 10.f, 10.f), Vec3(1.f));    // 교차
    CreateEntity(scene, Vec3(10.f, 10.f, -10.f), Vec3(1.f));   // z 불일치
    CreateEntity(scene, Vec3(10.f, -10.f, 10.f), Vec3(1.f));   // y 불일치
    CreateEntity(scene, Vec3(-10.f, 10.f, 10.f), Vec3(1.f));   // x 불일치
    InsertAll(scene, octree);

    const Aabb queryAabb = Aabb::FromCenterExtents(Vec3(10.f, 10.f, 10.f), Vec3(2.f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 1);
}

////////// ShrinkToFit //////////

TEST(Octree, ShrinkToFit_AfterRemoveAll)
{
    OctreeTestScene        scene  = {};
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
    OctreeDesc desc      = {};
    desc.initBaseExtents = Vec3(64.f);

    OctreeTestScene        scene  = {};
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
    OctreeTestScene        scene  = {};
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

    const Aabb queryAabb = Aabb::FromCenterExtents(Vec3(0.f), Vec3(9999.f));
    EXPECT_EQ(QueryCount(octree, queryAabb), 10);
}

TEST(Octree, ShrinkToFit_Idempotent)
{
    // 여러 번 호출해도 결과가 동일해야 함
    OctreeTestScene        scene  = {};
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

    OctreeTestScene scene  = {};
    Octree<TagData> octree {};

    Entity e    = scene.CreateEntity();
    auto&  comp = e.CreateComponent<AabbComponent>();
    comp.aabb   = Aabb::FromCenterExtents(Vec3::k_zero, Vec3(1.f));
    e.ModifyWorldTransform().SetTranslation(Vec3(0.f));

    octree.Insert(e, MakeWorldAabb(scene, e), TagData { 42 });

    int foundTag = -1;
    octree.Query(
        Aabb::FromCenterExtents(Vec3(0.f), Vec3(5.f)),
        MakeAabbIntersect(),
        [&](Entity, const TagData& _data)
        { foundTag = _data.tag; });

    EXPECT_EQ(foundTag, 42);
}

TEST(Octree, UserData_MultipleDistinct)
{
    struct IdData
    {
        int id = -1;
    };

    OctreeTestScene scene  = {};
    Octree<IdData>  octree {};

    for (int i = 0; i < 5; ++i)
    {
        Entity e = scene.CreateEntity();
        auto&  c = e.CreateComponent<AabbComponent>();
        c.aabb   = Aabb::FromCenterExtents(Vec3::k_zero, Vec3(1.f));
        e.ModifyWorldTransform().SetTranslation(Vec3(static_cast<float>(i) * 10.f, 0.f, 0.f));
        octree.Insert(e, MakeWorldAabb(scene, e), IdData { i });
    }

    // id=2 위치만 쿼리
    Vector<int> foundIds = {};
    octree.Query(
        Aabb::FromCenterExtents(Vec3(20.f, 0.f, 0.f), Vec3(2.f)),
        MakeAabbIntersect(),
        [&](Entity, const IdData& _data)
        { foundIds.push_back(_data.id); });

    EXPECT_EQ(foundIds.size(), 1);
    EXPECT_EQ(foundIds[0], 2);
}

////////// 실제 씬 시뮬레이션 //////////

TEST(Octree, Scene_DynamicMovement)
{
    OctreeTestScene        scene  = {};
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
            octree.Update(entities[i], Aabb::FromCenterExtents(newCenter, Vec3(1.f)));
        }
        EXPECT_EQ(octree.GetNumItems(), k_count) << "frame: " << frame;
    }
}

TEST(Octree, Scene_SpawnAndDespawn)
{
    OctreeTestScene        scene  = {};
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
    OctreeTestScene        scene  = {};
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
            octree.Update(dynamicEntities[i], Aabb::FromCenterExtents(newCenter, Vec3(1.f)));
        }
        EXPECT_EQ(octree.GetNumItems(), k_staticCount + k_dynamicCount) << "frame: " << frame;
    }
}

TEST(Octree, Scene_ShrinkAfterMassRemove)
{
    // 대량 삽입 → 대량 제거 → shrink → 재삽입 사이클
    OctreeTestScene        scene  = {};
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
    OctreeTestScene        scene  = {};
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
        octree.Update(entities[i], Aabb::FromCenterExtents(finalPos, Vec3(1.f)));
    }

    // 각 entity 위치를 개별 쿼리로 확인
    for (int i = 0; i < k_count; ++i)
    {
        const Vec3 pos       = Vec3(static_cast<float>(i) * 100.f, 0.f, 0.f);
        const Aabb queryAabb = Aabb::FromCenterExtents(pos, Vec3(2.f));
        EXPECT_EQ(QueryCount(octree, queryAabb), 1) << "entity index: " << i;
    }
}

////////// helpers 추가 //////////

static Frustum MakeFrustum(const Vec3 _pos, const Vec3 _lookAt)
{
    const Matrix viewMatrix = Matrix::CreateLookAt(_pos, _lookAt, { 0.f, 1.f, 0.f });
    const Matrix projMatrix = Matrix::CreatePerspectiveFieldOfView(k_halfPi, 16.f / 9.f, 0.1f, 1000.f);
    return Frustum { viewMatrix * projMatrix };
}

static auto MakeFrustumIntersect()
{
    return [](const Frustum& _frustum, const Aabb& _nodeAabb) -> bool
    {
        return _frustum.Intersects(_nodeAabb) != ePlaneIntersectType::Outside;
    };
}

static SizeT QueryCountFrustum(Octree<std::monostate>& _octree, const Frustum& _frustum)
{
    SizeT count = 0;
    _octree.Query(_frustum, MakeFrustumIntersect(), [&](Entity, const std::monostate&)
                  { ++count; });
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
    octree.Query(frustum, MakeFrustumIntersect(), [&](Entity _e, const std::monostate&)
                 { hits.push_back(_e); });

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
    octree.Update(e, Aabb::FromCenterExtents(Vec3(0.f, 0.f, 50.f), Vec3(1.f)));
    EXPECT_EQ(QueryCountFrustum(octree, frustum), 1);

    // 다시 밖으로
    octree.Update(e, Aabb::FromCenterExtents(Vec3(0.f, 0.f, -500.f), Vec3(1.f)));
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
            MakeRandom<float>(-200.f, 200.f),
            MakeRandom<float>(-200.f, 200.f),
            MakeRandom<float>(-200.f, 200.f),
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
        if (frustum.Intersects(aabb) != ePlaneIntersectType::Outside)
        {
            ++linearCount;
        }
    }

    // 옥트리 쿼리
    const SizeT octreeCount = QueryCountFrustum(octree, frustum);

    EXPECT_EQ(octreeCount, linearCount);
}