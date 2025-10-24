#include "pch.h"

#include "CullingSystem.h"

namespace jam
{                       

void CullingSystem::Build(const Scene* _pScene)
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

        const Mat4 world   = t.transform.GetWorld().CreateWorld();
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

    m_tempIDs.resize(m_drawItemIDs.size());
    m_executor.for_each(0,
                        m_drawItemIDs.size(),
                        [&](const size_t begin, const size_t end)
                        {
                            for (size_t i = begin; i < end; ++i)
                                m_tempIDs[i] = m_drawItemIDs[m_bvh.prim_ids[i]];
                        });

    m_drawItemIDs.swap(m_tempIDs);
}

void CullingSystem::Refit(const Scene* _pScene)
{
    auto view = _pScene->View<TransformComponent, MeshRenderer>();   // view를 통한 접근이 더 빠름

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

void CullingSystem::Clear()
{
    m_boxes.clear();
    m_centers.clear();
    m_drawItems.clear();
    m_drawItemIDs.clear();
    m_bvh = BVH {};
}

CullingSystem::CullingSystem()
{
    m_boxes.reserve(1024);
    m_centers.reserve(1024);
    m_drawItems.reserve(1024);
    m_drawItemIDs.reserve(1024);
    m_tempIDs.reserve(1024);
    m_stack.reserve(64);
}

bool CullingSystem::IsIntersectWithFrustum_(const Node& _node, const Frustum& _frustumInW) const
{
    const Box& box = _node.get_bbox();
    const AABB aabbInW { ToVec3_(box.min), ToVec3_(box.max) };
    return _frustumInW.Intersects(aabbInW) != ePlaneIntersectType::Outside;
}

CullingSystem::V3 CullingSystem::ToV3_(const Vec3& _v) const
{
    return V3 { _v.x, _v.y, _v.z };
}

CullingSystem::Box CullingSystem::ToBox_(const Vec3& _min, const Vec3& _max) const
{
    return Box { ToV3_(_min), ToV3_(_max) };
}

CullingSystem::Box CullingSystem::ToBox_(const AABB& _aabb) const
{
    return Box { ToV3_(_aabb.min), ToV3_(_aabb.max) };
}

Vec3 CullingSystem::ToVec3_(const V3& _v) const
{
    return Vec3 { _v[0], _v[1], _v[2] };
}

UInt16 CullingSystem::EncodeDepth_(const float _depth, const float _nearZ, const float _invRange) const
{
    return static_cast<UInt16>(std::lroundf(Clamp(((_depth - _nearZ) * _invRange) * 65535.0f, 0.f, 65535.f)));
}

}   // namespace jam