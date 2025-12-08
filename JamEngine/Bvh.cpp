#include "pch.h"

#include "Bvh.h"

#include "RenderableComponent.h"
#include "Scene.h"
#include "TransformComponent.h"
//
//namespace jam
//{
//
//Bvh::Bvh()
//    : m_executor(m_threadPool)
//{
//    m_boxes.reserve(1024);
//    m_centers.reserve(1024);
//    m_items.reserve(1024);
//    m_itemIDs.reserve(1024);
//    m_tmpIDs.reserve(1024);
//    m_tmpStack.reserve(64);
//}
//
//void Bvh::Build(const Scene& _scene)
//{
//    m_timer.Restart();
//
//    // clean up
//    m_boxes.clear();
//    m_centers.clear();
//    m_items.clear();
//    m_itemIDs.clear();
//
//    // view
//    const auto view = _scene.View<TransformComponent, RenderableComponent>();
//
//    // reserve
//    const size_t n = view.size_hint();
//    if (n == 0)
//    {
//        return;
//    }
//
//    m_boxes.reserve(n);
//    m_centers.reserve(n);
//    m_items.reserve(n);
//    m_itemIDs.reserve(n);
//
//    UInt32 index = 0;
//    for (const auto& [_, tc, rc]: view.each())
//    {
//        const Renderable& r = rc.renderable;
//
//        if (r.pMaterial || r.pMesh)
//        {
//            const Transform& th = tc.transformHierarchy.GetWorldTransform();
//
//            // world aabb
//            AABB aabb = r.pMesh->GetBoundingBox();
//            aabb.Transform(th.GetMatrix());
//
//            // bbox
//            BBox bbox;
//            bbox.min = BVec { aabb.min.x, aabb.min.y, aabb.min.z };
//            bbox.max = BVec { aabb.max.x, aabb.max.y, aabb.max.z };
//
//            // center
//            BVec center;
//            center[0] = (aabb.min.x + aabb.max.x) * 0.5f;
//            center[1] = (aabb.min.y + aabb.max.y) * 0.5f;
//            center[2] = (aabb.min.z + aabb.max.z) * 0.5f;
//
//            // collect
//            m_boxes.emplace_back(bbox);
//            m_centers.emplace_back(center);
//            m_items.emplace_back(&r, &th, aabb);
//            m_itemIDs.emplace_back(index++);
//        }
//    }
//
//    // build bvh
//    BBuilderConfig cfg;
//    cfg.quality = BBuilder::Quality::Medium;
//    m_bvh       = BBuilder::build(m_boxes, m_centers, cfg);
//
//    m_tmpIDs.resize(m_itemIDs.size());
//
//    m_executor.for_each(
//        0,
//        m_itemIDs.size(),
//        [&](const size_t begin, const size_t end, Bvh* pThis)
//        {
//            for (size_t i = begin; i < end; ++i)
//            {
//                const size_t primID = pThis->m_bvh.prim_ids[i];
//                pThis->m_tmpIDs[i]  = pThis->m_itemIDs[primID];
//            }
//        });
//
//    m_itemIDs.swap(m_tmpIDs);
//    m_lastBuildSec = static_cast<float>(m_timer.GetTotal(eSIPrefix::Sec));
//}
//
//void Bvh::Refit()
//{
//    m_timer.Restart();
//
//    m_bvh.refit(
//        [&](BNode& _node)
//        {
//            BBox merge;
//            merge.make_empty();
//
//            const size_t start = _node.index.first_id();
//            const size_t end   = start + _node.index.prim_count();
//            for (size_t i = start; i < end; ++i)
//            {
//                const UInt32 itemID = m_itemIDs[i];
//                JAM_ASSERT(itemID < m_items.size(), "Item ID out of range during refit");
//
//                Item&             item        = m_items[itemID];
//                const Renderable* pRenderable = item.pRenderable;
//                const Transform*  pTransform  = item.pTransform;
//
//                // refit aabb
//                const Mat4 mtx  = pTransform->GetMatrix();
//                AABB       aabb = pRenderable->pMesh->GetBoundingBox();
//                aabb.Transform(mtx);
//
//                // Update BVH box
//                m_boxes[i].min = { aabb.min.x, aabb.min.y, aabb.min.z };
//                m_boxes[i].max = { aabb.max.x, aabb.max.y, aabb.max.z };
//                merge.extend(m_boxes[i]);
//            }
//
//            _node.set_bbox(merge);
//        });
//
//    m_lastRebuiltSec = static_cast<float>(m_timer.GetTotal(eSIPrefix::Sec));
//}
//
//void Bvh::QueryFrustum(const Frustum& _frustum, std::vector<VisibleItem>& _outItems)
//{
//    m_timer.Restart();
//    _outItems.clear();
//
//    const Plane& nearPlane = _frustum.nearPlane;   // for calc approximate depth
//    const float  minD      = _frustum.Near();
//    const float  maxD      = _frustum.Far();
//    const float  range     = maxD - minD;
//    const float  invRange  = IsZeroApprox(range) ? 0.0f : 1.0f / range;
//
//    m_tmpStack.clear();
//    m_tmpStack.push_back(m_bvh.get_root().index);
//    while (!m_tmpStack.empty())
//    {
//        BBVHIndex top = m_tmpStack.back();
//        m_tmpStack.pop_back();
//
//        if (top.is_leaf())
//        {
//            const size_t start = top.first_id();
//            const size_t end   = start + top.prim_count();
//            for (size_t i = start; i < end; ++i)
//            {
//                const Item& item = m_items[m_itemIDs[i]];
//
//                // Frustum test
//                if (_frustum.Intersects(item.aabb) != ePlaneIntersectType::Outside)
//                {
//                    // calc approx depth (for sort key)
//                    const Vec3   center       = item.aabb.Center();
//                    const float  approxDepth  = nearPlane.DistanceToPoint(center);
//                    const float  normedDepth  = (approxDepth - minD) * invRange + 0.5f;   // for rounding
//                    const UInt16 encodedDepth = static_cast<UInt16>(Clamp(normedDepth, 0.0f, 1.0f) * 65535.f);
//
//                    // collect
//                    _outItems.emplace_back(item.pRenderable, item.pTransform, encodedDepth);
//                }
//            }
//        }
//        else
//        {
//            const size_t leftIdx  = top.first_id();
//            const size_t rightIdx = top.first_id() + 1;
//
//            // Validate node indices
//            if (rightIdx >= m_bvh.nodes.size())
//            {
//                JAM_ASSERT(false, "BVH node index out of range");
//                continue;
//            }
//
//            const BNode& left  = m_bvh.nodes[leftIdx];
//            const BNode& right = m_bvh.nodes[rightIdx];
//
//            if (Intersect_(left, _frustum))
//            {
//                m_tmpStack.emplace_back(left.index);
//            }
//
//            if (Intersect_(right, _frustum))
//            {
//                m_tmpStack.emplace_back(right.index);
//            }
//        }
//    }
//
//    m_lastFrustumQuerySec = static_cast<float>(m_timer.GetTotal(eSIPrefix::Sec));
//}
//
//bool Bvh::Intersect_(const BNode& _node, const Frustum& _frustum) const
//{
//    const BBox box = _node.get_bbox();
//
//    AABB aabb;
//    aabb.min.x = box.min[0];
//    aabb.min.y = box.min[1];
//    aabb.min.z = box.min[2];
//    aabb.max.x = box.max[0];
//    aabb.max.y = box.max[1];
//    aabb.max.z = box.max[2];
//    return _frustum.Intersects(aabb) != ePlaneIntersectType::Outside;
//}
//
//}   // namespace jam