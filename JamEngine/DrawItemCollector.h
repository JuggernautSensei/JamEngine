#pragma once
#include "BoundBox.h"
#include "Components.h"
#include "DrawItem.h"
#include "RadixSort.h"
#include "Scene.h"

#include "vendor/bvh/v2/bbox.h"
#include "vendor/bvh/v2/default_builder.h"
#include "vendor/bvh/v2/node.h"
#include "vendor/bvh/v2/stack.h"
#include "vendor/bvh/v2/thread_pool.h"
#include "vendor/bvh/v2/vec.h"

namespace jam
{

struct DefaultSortKeyGenerator
{
    JAM_NODISCARD UInt64 operator()(const Mesh& _mesh, const Material& _material, const UInt16 _minDepth, const UInt16 _maxDepth) const
    {
        // 머테리얼 > 메쉬 > 깊이
        UInt64 sortKey = 0;
        sortKey |= _material.GetSortKey();
        sortKey <<= 16;
        sortKey |= _mesh.GetSortKey();
        sortKey <<= 16;
        sortKey |= _minDepth;
        return sortKey;
    }
};

struct DefaultCollectorSink
{
    void Clear()
    {
        items.clear();
        sortKeys.clear();
        tmpItems.clear();
        tmpSortKeys.clear();
    }

    void Add(DrawItem& _item, Entity)
    {
        items.push_back(&_item);
        sortKeys.emplace_back(_item.sortKey);
    }

    void Sort()
    {
        size_t n = items.size();
        if (n < 200)   // 휴리스틱 값
        {
            std::ranges::sort(items, CompareDesc);
        }
        else
        {
            if (tmpItems.size() < n)
            {
                tmpItems.resize(n);
                tmpSortKeys.resize(n);
            }

            RadixSort(sortKeys.data(), tmpSortKeys.data(), items.data(), tmpItems.data(), NarrowCast<UInt32>(n));
        }
    }

    static bool CompareDesc(const DrawItem* _a, const DrawItem* _b)
    {
        return _a->sortKey > _b->sortKey;
    }

    std::vector<DrawItem*> items;
    std::vector<UInt64>    sortKeys;
    std::vector<DrawItem*> tmpItems;
    std::vector<UInt64>    tmpSortKeys;
};

class DrawItemCollector
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
    void Clear();
    void Build(const Scene* _pScene);

    template<typename TCollectorSink, typename TSortKeyGen>
    void CollectVisible(const Frustum& _frustumInW, TCollectorSink& _sink, TSortKeyGen&& _keyGen = DefaultSortKeyGenerator {})
    {
        _sink.Clear();

        if (m_drawItemIDs.empty())
            return;

        // 카메라 info
        const Vec3  cameraPos     = _frustumInW.position;
        const Vec3  frustumLookTo = _frustumInW.GetLookTo();
        const float nearZ         = _frustumInW.GetNear();
        const float farZ          = _frustumInW.GetFar();
        const float invRange      = IsNearlyEqual(nearZ, farZ) ? 0 : 1.0f / (farZ - nearZ);

        m_stack.clear();
        m_stack.push_back(m_bvh.get_root().index);   // bvh 의 루트 노드
        while (!m_stack.empty())
        {
            BVHIndex top = m_stack.back();
            m_stack.pop_back();

            if (top.is_leaf())   // 리프 노드이면 드로우 아이템 수집
            {
                size_t start = top.first_id();
                size_t end   = start + top.prim_count();
                for (size_t i = start; i < end; ++i)
                {
                    const UInt32 idx    = m_drawItemIDs[i];
                    DrawItemEx&  itemEx = m_drawItems[idx];
                    DrawItem&    item   = itemEx.item;

                    const AABB& aabbInW = itemEx.aabbInW;
                    if (_frustumInW.Intersects(aabbInW) == ePlaneIntersactType::Outside)   // 프러스텀에 포함되지 않으면 스킵
                        continue;

                    // 깊이값 계산 (단순 AABB 중심과 프러스텀 시선 벡터의 내적 + AABB extents 로 근사)
                    const Vec3 centerInW  = aabbInW.Center();
                    const Vec3 extendsInW = aabbInW.Extents();

                    const Vec3  cmrToCenter = centerInW - cameraPos;
                    const float centerZ     = frustumLookTo.Dot(cmrToCenter);
                    const Vec3  absLookDir  = Absolute(frustumLookTo);
                    const float rView       = absLookDir.Dot(extendsInW);

                    const UInt16 minDepth = EncodeDepth_(centerZ - rView, nearZ, invRange);
                    const UInt16 maxDepth = EncodeDepth_(centerZ + rView, nearZ, invRange);
                    item.sortKey          = _keyGen(*item.pMesh, *item.pMaterial, minDepth, maxDepth);
                    _sink.Add(item, itemEx.entity);
                }
            }
            else
            {
                // 프러스텀과 교차하는 노드만 스택에 추가

                const Node& left  = m_bvh.nodes[top.first_id()];
                const Node& right = m_bvh.nodes[top.first_id() + 1];

                if (IsIntersectWithFrustum_(left, _frustumInW))
                    m_stack.push_back(left.index);

                if (IsIntersectWithFrustum_(right, _frustumInW))
                    m_stack.push_back(right.index);
            }
        }

        _sink.Sort();
    }

private:
    JAM_NODISCARD bool   IsIntersectWithFrustum_(const Node& _node, const Frustum& _frustumInW) const;
    JAM_NODISCARD V3     ToV3_(const Vec3& _v) const;
    JAM_NODISCARD Vec3   ToVec3_(const V3& _v) const;
    JAM_NODISCARD UInt16 EncodeDepth_(float _depth, float _nearZ, float _invRange) const;

    std::vector<Box>        m_boxes;
    std::vector<V3>         m_centers;
    std::vector<DrawItemEx> m_drawItems;
    std::vector<UInt32>     m_drawItemIDs;

    std::vector<UInt32>   m_tempIDs;
    std::vector<BVHIndex> m_stack;

    BVH        m_bvh;
    ThreadPool m_threadPool;
    Executor   m_executor { m_threadPool };
};
}   // namespace jam
