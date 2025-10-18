#include "pch.h"

#include "DrawItemCollector.h"

namespace jam
{

//////////////////////////////////////////////

void DrawItemCollector::Build(const Scene* _pScene)
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

    for (auto e: view)
    {
        const TransformComponent& t = view.get<TransformComponent>(e);
        const MeshRenderer&       m = view.get<MeshRenderer>(e);

        if (!m.pMesh || !m.pMaterial)
            continue;

        const Mat4 world   = t.transform.GetWorld().CreateWorld();
        const AABB aabbInL = m.pMesh->GetBoundingBox();
        const AABB aabbInW = AABB::Transform(aabbInL, world);

        m_boxes.emplace_back(ToV3_(aabbInW.min), ToV3_(aabbInW.max));
        m_centers.emplace_back(ToV3_(aabbInW.Center()));

        DrawItemEx itemEx;
        itemEx.item.pMesh     = m.pMesh.get();
        itemEx.item.pMaterial = m.pMaterial.get();
        itemEx.item.world     = world;
        itemEx.aabbInW        = aabbInW;
        itemEx.entity         = t.owner;
        m_drawItems.emplace_back(std::move(itemEx));
        m_drawItemIDs.emplace_back(NarrowCast<UInt32>(m_drawItems.size() - 1));
    }

    BuilderConfig cfg;
    cfg.quality = Builder::Quality::Low;
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

void DrawItemCollector::Clear()
{
    m_boxes.clear();
    m_centers.clear();
    m_drawItems.clear();
    m_drawItemIDs.clear();
    m_bvh = BVH {};
}

bool DrawItemCollector::IsIntersectWithFrustum_(const Node& _node, const Frustum& _frustumInW) const
{
    const Box& box = _node.get_bbox();
    const AABB aabbInW { ToVec3_(box.min), ToVec3_(box.max) };
    return _frustumInW.Intersects(aabbInW) != ePlaneIntersactType::Outside;
}

DrawItemCollector::V3 DrawItemCollector::ToV3_(const Vec3& _v) const
{
    return V3 { _v.x, _v.y, _v.z };
}

Vec3 DrawItemCollector::ToVec3_(const V3& _v) const
{
    return Vec3 { _v[0], _v[1], _v[2] };
}

UInt16 DrawItemCollector::EncodeDepth_(const float _depth, const float _nearZ, const float _invRange) const
{
    return static_cast<UInt16>(std::lroundf(Clamp(((_depth - _nearZ) * _invRange) * 65535.0f, 0.f, 65535.f)));
}

}   // namespace jam