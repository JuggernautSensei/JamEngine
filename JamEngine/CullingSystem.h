#pragma once
#include "BoundBox.h"
#include "Components.h"
#include "DrawItem.h"
#include "Scene.h"

#include "vendor/bvh/v2/bbox.h"
#include "vendor/bvh/v2/default_builder.h"
#include "vendor/bvh/v2/node.h"
#include "vendor/bvh/v2/stack.h"
#include "vendor/bvh/v2/thread_pool.h"
#include "vendor/bvh/v2/vec.h"

namespace jam
{

struct DefaultCullingPolicy
{
    void   Clear() const {}
    void   AddItem(const DrawItem& _item, Entity _entity) const {}
    UInt64 CreateKey(const Mesh& _mesh, const Material& _material, UInt16 _minDepth, UInt16 _maxDepth) const { return 0; }
    void   Sort() const {}
};

template<typename TPolicy = DefaultCullingPolicy>
class CullingSystem
{
    using Scalar        = float;
    using V3            = bvh::v2::Vec<Scalar, 3>;
    using Box           = bvh::v2::BBox<Scalar, 3>;
    using Node          = bvh::v2::Node<Scalar, 3>;
    using BVH           = bvh::v2::Bvh<Node>;
    using ThreadPool    = bvh::v2::ThreadPool;
    using Executor      = bvh::v2::ParallelExecutor;
    using Builder       = bvh::v2::DefaultBuilder<Node>;
    using BuilderConfig = Builder::Config;
    using BVHIndex      = BVH::Index;

    struct DrawItemEx
    {
        DrawItem item;
        Entity   entity;
        AABB     aabbInW;
    };

public:
    explicit CullingSystem(TPolicy&& _policy = TPolicy {})
        : m_policy(std::forward<TPolicy>(_policy))
        , m_executor(m_threadPool)
    {
        m_boxes.reserve(1024);
        m_centers.reserve(1024);
        m_drawItems.reserve(1024);
        m_drawItemIDs.reserve(1024);
        m_tempIDs.reserve(1024);
        m_stack.reserve(64);
    }

    ~CullingSystem() = default;

    CullingSystem& operator=(const CullingSystem&) = delete;
    CullingSystem(const CullingSystem&)            = delete;
    CullingSystem(CullingSystem&&)                 = delete;
    CullingSystem& operator=(CullingSystem&&)      = delete;

    void Build(const Scene* _pScene);
    void Refit(const Scene* _pScene);
    void Collect(const Frustum& _frustumInW);

    TPolicy&       GetPolicy() { return m_policy; }
    const TPolicy& GetPolicy() const { return m_policy; }

private:
    JAM_NODISCARD bool   IsIntersectWithFrustum_(const Node& _node, const Frustum& _frustumInW) const;
    JAM_NODISCARD V3     ToV3_(const Vec3& _v) const;
    JAM_NODISCARD Vec3   ToVec3_(const V3& _v) const;
    JAM_NODISCARD Box    ToBox_(const Vec3& _min, const Vec3& _max) const;
    JAM_NODISCARD Box    ToBox_(const AABB& _aabb) const;
    JAM_NODISCARD UInt16 EncodeDepth_(float _depth, float _nearZ, float _invRange) const;

    std::vector<Box>        m_boxes;
    std::vector<V3>         m_centers;
    std::vector<DrawItemEx> m_drawItems;
    std::vector<UInt32>     m_drawItemIDs;

    std::vector<UInt32>   m_tempIDs;
    std::vector<BVHIndex> m_stack;

    TPolicy m_policy;

    BVH        m_bvh;
    ThreadPool m_threadPool;
    Executor   m_executor;
};

// 템플릿 구현
template<typename TPolicy>
void CullingSystem<TPolicy>::Build(const Scene* _pScene)
{
    m_boxes.clear();
    m_centers.clear();
    m_drawItems.clear();
    m_drawItemIDs.clear();

    auto   view = _pScene->View<TransformComponent, MeshRenderer>();
    size_t n    = view.size_hint();
    m_boxes.reserve(n);
    m_centers.reserve(n);
    m_drawItems.reserve(n);
    m_drawItemIDs.reserve(n);

    for (entt::entity e: view)
    {
        const TransformComponent& t = view.get<TransformComponent>(e);
        const MeshRenderer&       m = view.get<MeshRenderer>(e);

        if (!m.pMesh || !m.pMaterial)
            continue;

        const Mat4 world   = t.transform.GetWorld().GetMatrix();
        const AABB aabbInL = m.pMesh->GetBoundingBox();
        const AABB aabbInW = AABB::Transform(aabbInL, world);

        m_boxes.emplace_back(ToBox_(aabbInW));
        m_centers.emplace_back(ToV3_(aabbInW.Center()));

        DrawItemEx itemEx;
        itemEx.item.pMesh     = m.pMesh.get();
        itemEx.item.pMaterial = m.pMaterial.get();
        itemEx.item.world     = world;
        itemEx.aabbInW        = aabbInW;
        itemEx.entity         = t.transform.GetOwner();
        m_drawItems.emplace_back(std::move(itemEx));
        m_drawItemIDs.emplace_back(NarrowCast<UInt32>(m_drawItems.size() - 1));
    }

    BuilderConfig cfg;
    cfg.quality = Builder::Quality::Medium;
    m_bvh       = Builder::build(m_threadPool, m_boxes, m_centers, cfg);

    struct ExecutorLoop
    {
        void operator()(const size_t begin, const size_t end) const
        {
            for (size_t i = begin; i < end; ++i)
                tmpIDs[i] = drawItemIds[bvh->prim_ids[i]];
        }

        BVH*                 bvh;
        std::vector<UInt32>* tmpIDs;
        std::vector<UInt32>* drawItemIds;
    };

    m_tempIDs.resize(m_drawItemIDs.size());
    m_executor.for_each(0, m_drawItemIDs.size(), ExecutorLoop { &m_bvh, &m_tempIDs, &m_drawItemIDs });
    m_drawItemIDs.swap(m_tempIDs);
}

template<typename TPolicy>
void CullingSystem<TPolicy>::Refit(const Scene* _pScene)
{
    auto view = _pScene->View<TransformComponent, MeshRenderer>();

    m_bvh.refit(
        [&](Node& node)
        {
            Box merge;
            merge.make_empty();

            size_t start = node.index.first_id();
            size_t end   = start + node.index.prim_count();
            for (size_t i = start; i < end; ++i)
            {
                DrawItemEx& itemEx = m_drawItems[m_drawItemIDs[i]];
                const auto& [t, m] = view.get<TransformComponent, MeshRenderer>(itemEx.entity.GetHandle());
                itemEx.item.world  = t.transform.GetWorld().CreateWorld();
                itemEx.aabbInW     = AABB::Transform(m.pMesh->GetBoundingBox(), itemEx.item.world);
                m_boxes[i]         = ToBox_(itemEx.aabbInW);

                merge.extend(m_boxes[i]);
            }
            node.set_bbox(merge);
        });
}

template<typename TPolicy>
void CullingSystem<TPolicy>::Collect(const Frustum& _frustumInW)
{
    m_policy.Clear();

    if (m_drawItemIDs.empty())
        return;

    const Vec3  cameraPos     = _frustumInW.position;
    const Vec3  frustumLookTo = _frustumInW.ViewLookTo();
    const float nearZ         = _frustumInW.GetNear();
    const float farZ          = _frustumInW.GetFar();
    const float invRange      = IsNearlyEqual(nearZ, farZ) ? 0 : 1.0f / (farZ - nearZ);

    m_stack.clear();
    m_stack.push_back(m_bvh.get_root().index);
    while (!m_stack.empty())
    {
        BVHIndex top = m_stack.back();
        m_stack.pop_back();

        if (top.is_leaf())
        {
            size_t start = top.first_id();
            size_t end   = start + top.prim_count();
            for (size_t i = start; i < end; ++i)
            {
                const UInt32 idx    = m_drawItemIDs[i];
                DrawItemEx&  itemEx = m_drawItems[idx];
                DrawItem&    item   = itemEx.item;

                const AABB& aabbInW = itemEx.aabbInW;
                if (_frustumInW.Intersects(aabbInW) == ePlaneIntersectType::Outside)
                    continue;

                const Vec3 centerInW  = aabbInW.Center();
                const Vec3 extendsInW = aabbInW.Extents();

                const Vec3  cmrToCenter = centerInW - cameraPos;
                const float centerZ     = frustumLookTo.Dot(cmrToCenter);
                const Vec3  absLookDir  = Absolute(frustumLookTo);
                const float rView       = absLookDir.Dot(extendsInW);

                const UInt16 minDepth = EncodeDepth_(centerZ - rView, nearZ, invRange);
                const UInt16 maxDepth = EncodeDepth_(centerZ + rView, nearZ, invRange);
                item.sortKey          = m_policy.CreateKey(*item.pMesh, *item.pMaterial, minDepth, maxDepth);
                m_policy.AddItem(item, itemEx.entity);
            }
        }
        else
        {
            const Node& left  = m_bvh.nodes[top.first_id()];
            const Node& right = m_bvh.nodes[top.first_id() + 1];

            if (IsIntersectWithFrustum_(left, _frustumInW))
                m_stack.push_back(left.index);

            if (IsIntersectWithFrustum_(right, _frustumInW))
                m_stack.push_back(right.index);
        }
    }

    m_policy.Sort();
}

template<typename TPolicy>
bool CullingSystem<TPolicy>::IsIntersectWithFrustum_(const Node& _node, const Frustum& _frustumInW) const
{
    const Box& box = _node.get_bbox();
    const AABB aabbInW { ToVec3_(box.min), ToVec3_(box.max) };
    return _frustumInW.Intersects(aabbInW) != ePlaneIntersectType::Outside;
}

template<typename TPolicy>
CullingSystem<TPolicy>::V3 CullingSystem<TPolicy>::ToV3_(const Vec3& _v) const
{
    return V3 { _v.x, _v.y, _v.z };
}

template<typename TPolicy>
CullingSystem<TPolicy>::Box CullingSystem<TPolicy>::ToBox_(const Vec3& _min, const Vec3& _max) const
{
    return Box { ToV3_(_min), ToV3_(_max) };
}

template<typename TPolicy>
CullingSystem<TPolicy>::Box CullingSystem<TPolicy>::ToBox_(const AABB& _aabb) const
{
    return Box { ToV3_(_aabb.min), ToV3_(_aabb.max) };
}

template<typename TPolicy>
Vec3 CullingSystem<TPolicy>::ToVec3_(const V3& _v) const
{
    return Vec3 { _v[0], _v[1], _v[2] };
}

template<typename TPolicy>
UInt16 CullingSystem<TPolicy>::EncodeDepth_(const float _depth, const float _nearZ, const float _invRange) const
{
    return static_cast<UInt16>(std::lroundf(Clamp(((_depth - _nearZ) * _invRange) * 65535.0f, 0.f, 65535.f)));
}

}   // namespace jam