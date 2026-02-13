#pragma once
#include "Entity.h"
#include "MemoryPool.h"
#include "BoundingVolume.h"

namespace Jam
{

enum class eOctreeRegion
{
    NearLeftTop = 0,
    NearRightTop,
    NearLeftBottom,
    NearRightBottom,
    FarLeftTop,
    FarRightTop,
    FarLeftBottom,
    FarRightBottom,
};
constexpr static eOctreeRegion k_invalidOctreeRegion = static_cast<eOctreeRegion>(-1);

struct OctreeDesc
{
    Vec3 minBaseExtents     = Vec3(4.f);      // 가장 작은 베이스 aabb extents. 이것보다 작게 분할하지 않음
    Vec3 initialBaseExtents = Vec3(1024.f);   // 초기 베이스 aabb extents

    Int32 maxDepth                  = 8;
    bool  bUseMaxDepthPolicy        = true;   // true: enforce max depth during subdivision; false: no max depth limit, split until minBaseExtents is reached
    bool  bIncreaseMaxDepthOnExpand = true;   // true: increase max depth when octree expands; false: keep max depth unchanged (this policy block merges nodes that exceed max depth after expansion)

    Int32 subdivisionThreshold = 16;    // number of items per node to trigger subdivision
    float looseFactor          = 2.f;   // loose factor (>= 1.f). 1.f = tight octree
};

// template<typename T>
class Octree
{
    using T = int;
    struct OcNode;
    struct OcItem;

    struct OcNodeDeletor
    {
        void operator()(OcNode* _pNode) const
        {
            pPool->Delete(_pNode);
        }

        ObjectPool<OcNode>* pPool = nullptr;
    };

    struct OcItemDeletor
    {
        void operator()(OcItem* _pItem) const
        {
            pPool->Delete(_pItem);
        }

        ObjectPool<OcItem>* pPool = nullptr;
    };

    using OcNodeScoped     = Scoped<OcNode, OcNodeDeletor>;
    using OcItemScoped     = Scoped<OcItem, OcItemDeletor>;
    using OcItemScopedList = Vector<OcItemScoped>;

    struct OcItem
    {
        [[nodiscard]] Vec3 GetCenter() const
        {
            return aabb.GetCenter();
        }

        [[nodiscard]] Vec3 GetExtents() const
        {
            return aabb.GetExtents();
        }

        Entity  entity     = Entity::k_invalid;   // owner entity
        Aabb    aabb       = {};                  // bounding box
        OcNode* pOwnerNode = nullptr;             // owner node
        Int32   index      = -1;                  // pos in owner's item list
        T       userdata   = {};                  // user data
    };

    struct ChildrenNodes
    {
        void Set(
            const eOctreeRegion _region,
            OcNodeScoped&&      _pNode)
        {
            nodes[UnderlyingFrom(_region)] = std::move(_pNode);
        }

        [[nodiscard]] OcNode* Get(
            const eOctreeRegion _region) const
        {
            return nodes[UnderlyingFrom(_region)].GetPtr();
        }

        [[nodiscard]] OcNodeScoped Pop(
            const eOctreeRegion _region)
        {
            return std::move(nodes[UnderlyingFrom(_region)]);
        }

        [[nodiscard]] bool IsEmpty() const
        {
            return nodes[0] == nullptr;   // if one is null, all are null
        }

        void Clear()
        {
            for (OcNodeScoped& pNode: nodes)
            {
                pNode.Reset();
            }
        }

        auto begin()
        {
            return nodes.begin();
        }

        auto end()
        {
            return nodes.end();
        }

        auto begin() const
        {
            return nodes.begin();
        }

        auto end() const
        {
            return nodes.end();
        }

        Array<OcNodeScoped, CountOf<eOctreeRegion>()> nodes = {};
    };

    struct OcNode
    {
        [[nodiscard]] bool IsLeaf() const
        {
            return children.IsEmpty();
        }

        [[nodiscard]] bool HasChildren() const
        {
            return !children.IsEmpty();
        }

        [[nodiscard]] bool HasGrandChildren() const
        {
            return HasChildren()
                && children.nodes[0]->HasChildren();
        }

        [[nodiscard]] bool HasParent() const
        {
            return pParentNode != nullptr;
        }

        [[nodiscard]] bool Contain(
            const Aabb& _aabb) const
        {
            return aabb.Contain(_aabb);
        }

        [[nodiscard]] bool Contain(
            const OcItem* _pItem) const
        {
            return aabb.Contain(_pItem->aabb);
        }

        [[nodiscard]] Vec3 GetCenter() const
        {
            return aabb.GetCenter();
        }

        [[nodiscard]] Vec3 GetExtents() const
        {
            return aabb.GetExtents();
        }

        ChildrenNodes children    = {};
        OcNode*       pParentNode = nullptr;

        OcItemScopedList items          = {};
        Aabb             aabb           = {};
        Int32            depth          = -1;
        Int32            numItemsInTree = 0;   // total number of items in this node and all children
    };

public:
    explicit Octree(
        const OctreeDesc& _desc = {})
        : m_itemPool(2048)
        , m_nodePool(512)
        , m_desc(_desc)
    {
        JAM_ASSERT(_desc.looseFactor >= 1.f, "looseFactor must be >= 1.f");
        JAM_ASSERT(_desc.maxDepth >= 2, "maxDepth must be >= 2");
        JAM_ASSERT(_desc.subdivisionThreshold > 0, "subdivisionThreshold must be > 0");
        JAM_ASSERT(_desc.minBaseExtents.x > 1.f && _desc.minBaseExtents.y > 1.f && _desc.minBaseExtents.z > 1.f, "minBaseExtents must be > 1.f");
        JAM_ASSERT(_desc.initialBaseExtents.x >= _desc.minBaseExtents.x && _desc.initialBaseExtents.y >= _desc.minBaseExtents.y && _desc.initialBaseExtents.z >= _desc.minBaseExtents.z, "initialBaseExtents must be >= minBaseExtents");

        const Vec3 ext  = m_desc.initialBaseExtents * m_desc.looseFactor;
        const Aabb aabb = Aabb::FromCenterExtents(vec3::k_zero, ext);

        m_pRootNode        = MakeNode_(aabb);
        m_pRootNode->depth = 0;
    }

    void Insert(
        const Entity _entity,
        const Aabb&  _aabb,
        const T&     _userdata)
    {
        JAM_ASSERT(_entity, "Invalid entity");
        JAM_ASSERT(!m_itemCache.contains(_entity.GetUID()), "Entity already exists");

        // _aabb를 포함할 때까지 확장
        while (!m_pRootNode->Contain(_aabb))
        {
            ExpandOctree_();
        }

        // while 문을 통과헀다면, 반드시 insert 가능
        OcItemScoped pItem = MakeItem_(_entity, _aabb, _userdata);
        Insert_Recursive_(m_pRootNode.GetPtr(), std::move(pItem));
    }

    void Update(
        const Entity _entity,
        const Aabb&  _aabb)
    {
        JAM_ASSERT(_entity, "Invalid entity");

        const auto it = m_itemCache.find(_entity.GetUID());
        JAM_ASSERT(it != m_itemCache.end(), "Entity does not exist");

        // aabb 갱신
        OcItem* pItem = it->second;
        pItem->aabb   = _aabb;

        // 시나리오:
        //   1. 아이템이 같은 서브트리에 머무름
        //      ㄴ 1.1 자식 노드에 맞음 -> 자식 노드로 이동
        //      ㄴ 1.2 어떤 자식에도 맞지 않음 -> 같은 노드에 머무름 (할 일 없음)
        //   2. 아이템이 다른 서브트리로 이동
        //      ㄴ 2.1 옥트리 밖으로 이동 -> 옥트리 확장 후 루트로 이동 -> 2.2
        //      ㄴ 2.2 옥트리 안으로 이동 -> 목적지 노드로 이동

        // 자신을 포함하는 가장 작은 노드를 바텀-업으로 찾기
        OcNode* pOwnerNode = pItem->pOwnerNode;
        OcNode* pNode      = pOwnerNode;
        while (pNode && !pNode->Contain(pItem))
        {
            --pNode->numItemsInTree;   // 부모로 올라가기 전에 기존 트리의 numItemsInTree 감소
            pNode = pNode->pParentNode;
        }

        // 만약 기존과 같은 트리에 머무른다면
        if (pNode == pOwnerNode)
        {
            // pInsertNode의 numItemsInTree 는 변하지 않음

            // 자식이 있다면 자식한테 넣어봄
            if (pOwnerNode->HasChildren())
            {
                // 맞는 자식이 있다면, 그 자식 노드로 이동
                if (OcNode* pChildNode = FindBestChildNode_(pOwnerNode, _aabb))
                {
                    Insert_Recursive_(pChildNode, PopItemFromList_(pItem));
                }
            }
            // 자식이 없다면, 혹은 맞는 자식이 없다면
            // 그대로 현재 노드에 머무름 (할 일 없음)
        }
        else   // 다른 트리로 이동해야 한다면
        {
            // 아이템이 아예 현재 옥트리 밖에 존재
            if (pNode == nullptr)
            {
                // expand octree에서 재삽입이 꼬일 수 있기 때문에, 미리 아이템을 제거
                OcItemScoped pPopedItem = PopItemFromList_(pItem);

                // 옥트리 확장
                while (!m_pRootNode->Contain(_aabb))
                {
                    ExpandOctree_();
                }

                // 재삽입
                Insert_Recursive_(m_pRootNode.GetPtr(), std::move(pPopedItem));
            }
            else
            {
                // Insert_Recursive_ 에서 증가시켜주기 때문에 미리 감소
                --pNode->numItemsInTree;
                Insert_Recursive_(pNode, PopItemFromList_(pItem));
            }
        }
    }

    void Remove(
        const Entity _entity)
    {
        JAM_ASSERT(_entity, "Invalid entity");

        const UID  uid = _entity.GetUID();
        const auto it  = m_itemCache.find(uid);
        JAM_ASSERT(it != m_itemCache.end(), "Entity does not exist");

        // 바텀-업으로 numItemsInTree 감소
        const OcItem* pItem = it->second;
        OcNode*       pNode = pItem->pOwnerNode;
        while (pNode)
        {
            --pNode->numItemsInTree;
            pNode = pNode->pParentNode;
        }

        // 아이템 제거
        PopItemFromList_(pItem);
        m_itemCache.erase(it);
    }

    void ShrinkToFit()
    {
        // 1. 모든 아이템을 포함하는 자식 노드 탐색 -> 새로운 루트로 설정
        // root에 일부 아이템이 있거나, 빈 옥트리의 경우 shrink root 불가
        if (m_pRootNode->items.empty() && m_pRootNode->numItemsInTree > 0)
        {
            OcNode*       pRootNode    = m_pRootNode.GetPtr();
            OcNode*       pTargetNode  = pRootNode;               // shrink 할 노드 후보
            eOctreeRegion targetRegion = k_invalidOctreeRegion;   // shrink 할 노드의 부모 노드로 부터의 위치

            // find child that contains all items
            while (pTargetNode->HasChildren())
            {
                // 너무 작아질 경우 스킵
                const Vec3 baseExt = pTargetNode->GetExtents() / m_desc.looseFactor;
                if (baseExt.x <= m_desc.minBaseExtents.x
                    && baseExt.y <= m_desc.minBaseExtents.y
                    && baseExt.z <= m_desc.minBaseExtents.z)
                {
                    break;
                }

                // 현재 트리의 모든 아이템을 포함하는 자식 노드 탐색
                // 최종적으로 발견한 자식 노드가 재로운 루트가 될것
                OcNode*       pNextNode  = nullptr;
                eOctreeRegion nextRegion = k_invalidOctreeRegion;
                for (const eOctreeRegion region: EnumRanges<eOctreeRegion>())
                {
                    OcNode* pChildNode = pTargetNode->children.Get(region);
                    if (pChildNode->numItemsInTree == m_pRootNode->numItemsInTree)   // 발견
                    {
                        pNextNode  = pChildNode;
                        nextRegion = region;
                        break;
                    }
                    else if (pChildNode->numItemsInTree != 0)   // 일부라도 가지고 있는 경우. shrink 불가
                    {
                        break;
                    }
                }

                // 1. 아이템들이 흩어져 있음
                // 2. 모든 아이템들이 pTargetNode 에 있음
                if (!pNextNode)
                {
                    break;
                }

                // 다음 단계로 진행
                pTargetNode  = pNextNode;
                targetRegion = nextRegion;
            }

            // shrink 할 노드를 찾은 경우
            if (pTargetNode != pRootNode)
            {
                OcNodeScoped pNewRoot = pTargetNode->pParentNode->children.Pop(targetRegion);
                pNewRoot->pParentNode = nullptr;
                m_pRootNode           = std::move(pNewRoot);
            }
        }

        // 2. 빈 자식 노드 병합
        ShrinkToFitMergeEmptyChildren_Recursive_(m_pRootNode.GetPtr());
    }

    template<
        typename TBoundingVolume,
        typename IntersactFunc,
        typename CollectFunc>
    void Query(
        const TBoundingVolume& _bv,
        IntersactFunc&&        _intersactFunc,
        CollectFunc&&          _collectFunc)
    {
        Query_Recursive_(m_pRootNode.GetPtr(), _bv, _intersactFunc, _collectFunc);
    }

private:
    template<
        typename TBoundingVolume,
        typename IntersactFunc,
        typename CollectFunc>
    void Query_Recursive_(
        const OcNode*          _pNode,
        const TBoundingVolume& _bv,
        IntersactFunc&&        _intersactFunc,
        CollectFunc&&          _collectFunc)
    {
        if (_pNode->numItemsInTree == 0 || !_intersactFunc(_bv, _pNode->aabb))
        {
            return;
        }

        // check items in current node
        for (const OcItemScoped& pItem: _pNode->items)
        {
            if (_intersactFunc(_bv, pItem->aabb))
            {
                _collectFunc(pItem->entity, pItem->userdata);
            }
        }

        // check children
        if (_pNode->HasChildren())
        {
            for (const OcNodeScoped& pChildNode: _pNode->children)
            {
                Query_Recursive_(pChildNode.GetPtr(), _bv, _intersactFunc, _collectFunc);
            }
        }
    }

    void SplitNode_(
        OcNode* _pNode)
    {
        JAM_ASSERT(_pNode->IsLeaf(), "Node already has children");

        const Vec3 center       = _pNode->GetCenter();
        const Vec3 childExt     = _pNode->GetExtents() * 0.5f;     // loose box
        const Vec3 childBaseExt = childExt / m_desc.looseFactor;   // tight box

        for (const eOctreeRegion region: EnumRanges<eOctreeRegion>())
        {
            const Int32 iRegion = UnderlyingFrom(region);

            // 중심은 base box 기준으로 계산
            Vec3 childCenter = center;
            childCenter.x += (iRegion & 0b0001) ? childBaseExt.x : -childBaseExt.x;
            childCenter.y += (iRegion & 0b0010) ? -childBaseExt.y : childBaseExt.y;
            childCenter.z += (iRegion & 0b0100) ? childBaseExt.z : -childBaseExt.z;

            // 자식 노드 생성
            const Aabb   aabb       = Aabb::FromCenterExtents(childCenter, childExt);
            OcNodeScoped pChildNode = MakeNode_(aabb);
            pChildNode->pParentNode = _pNode;
            pChildNode->depth       = _pNode->depth + 1;
            _pNode->children.Set(region, std::move(pChildNode));
        }
    }

    void SubdivideIfNeed_(
        OcNode* _pNode)
    {
        if (!_pNode->IsLeaf())
        {
            // 이미 분할됨
            return;
        }

        const Int32 numItems = NumericCast<Int32>(_pNode->items.size());
        if (numItems <= m_desc.subdivisionThreshold)
        {
            // 임계치에 도달하지 못함
            return;
        }

        if (m_desc.bUseMaxDepthPolicy && _pNode->depth >= m_desc.maxDepth)
        {
            // 최대 깊이 도달
            return;
        }

        // check if children would be too small
        const Vec3 ext          = _pNode->GetExtents();
        const Vec3 childExt     = ext * 0.5f;
        const Vec3 childBaseExt = childExt / m_desc.looseFactor;

        // 너무 작아지면 분할하지 않음
        if (childBaseExt.x < m_desc.minBaseExtents.x
            || childBaseExt.y < m_desc.minBaseExtents.y
            || childBaseExt.z < m_desc.minBaseExtents.z)
        {
            return;   // would violate min size constraint
        }

        // 분할 수행
        SplitNode_(_pNode);

        // 기본 노드에 있던 아이템을 재분배
        OcItemScopedList existItems = std::move(_pNode->items);
        _pNode->numItemsInTree      = 0;   // will be recalculated in Insert_Recursive_
        for (OcItemScoped& pItem: existItems)
        {
            Insert_Recursive_(_pNode, std::move(pItem));
        }
    }

    void AddItemToNode_(
        OcNode*        _pNode,
        OcItemScoped&& _pItem)
    {
        const UID uid      = _pItem->entity.GetUID();
        m_itemCache[uid]   = _pItem.GetPtr();
        _pItem->pOwnerNode = _pNode;

        PushItemToList_(_pNode->items, std::move(_pItem));
        SubdivideIfNeed_(_pNode);
    }

    void PushItemToList_(
        OcItemScopedList& _items,
        OcItemScoped&&    _pItem) const
    {
        _pItem->index = NumericCast<Int32>(_items.size());
        _items.emplace_back(std::move(_pItem));
    }

    OcItemScoped PopItemFromList_(
        const OcItem* _pItem) const
    {
        OcItemScopedList& items      = _pItem->pOwnerNode->items;
        const Int32       itemIdx    = _pItem->index;
        const Int32       lastIdx    = NumericCast<Int32>(items.size() - 1);
        OcItemScoped      pPopedItem = std::move(items[itemIdx]);

        if (itemIdx != lastIdx)
        {
            // swap with last and update pos
            items[itemIdx]        = std::move(items[lastIdx]);
            items[itemIdx]->index = itemIdx;
        }

        items.pop_back();

        pPopedItem->pOwnerNode = nullptr;
        pPopedItem->index      = -1;
        return pPopedItem;
    }

    void Insert_Recursive_(
        OcNode*        _pRoot,
        OcItemScoped&& _pItem)
    {
        JAM_ASSERT(_pRoot->Contain(_pItem->aabb), "Item does not fit in node");

        // Insert_Recursive_ 호출 전체조건, _pRoot는 반드시 _pItem를 포함해야 함
        // 삽입은 보장되므로 numItemsInTree 증가
        ++_pRoot->numItemsInTree;

        if (_pRoot->HasChildren())
        {
            if (OcNode* pBestChild = FindBestChildNode_(_pRoot, _pItem->aabb))
            {
                Insert_Recursive_(pBestChild, std::move(_pItem));
            }
            else
            {
                // doesn't fit in any child, add to current node
                AddItemToNode_(_pRoot, std::move(_pItem));
            }
        }
        else
        {
            // leaf node, add here
            AddItemToNode_(_pRoot, std::move(_pItem));
        }
    }

    void ExpandOctree_()
    {
        const Vec3 center = m_pRootNode->GetCenter();
        const Vec3 ext    = m_pRootNode->GetExtents() * 2.f;
        const Aabb aabb   = Aabb::FromCenterExtents(center, ext);

        OcNodeScoped pNewRoot    = MakeNode_(aabb);
        pNewRoot->depth          = 0;
        pNewRoot->numItemsInTree = 0;

        if (m_pRootNode->HasChildren())   // 기존 root가 자식이 있으면..
        {
            // 이중 분할 후. 중심은 유지하고, 기존 root의 자식들을 손자 노드로 이동
            SplitNode_(pNewRoot.GetPtr());
            for (const OcNodeScoped& pChildNode: m_pRootNode->children)
            {
                SplitNode_(pChildNode.GetPtr());
            }

            // 매핑 테이블, 기존 root의 자식 노드들을 새로운 손자 노드 위치로 이동
            constexpr eOctreeRegion k_grandchildTable[CountOf<eOctreeRegion>()] = {
                eOctreeRegion::FarRightBottom,
                eOctreeRegion::FarLeftBottom,
                eOctreeRegion::FarRightTop,
                eOctreeRegion::FarLeftTop,
                eOctreeRegion::NearRightBottom,
                eOctreeRegion::NearLeftBottom,
                eOctreeRegion::NearRightTop,
                eOctreeRegion::NearLeftTop
            };

            // move old children to new grandchildren positions
            for (const eOctreeRegion region: EnumRanges<eOctreeRegion>())
            {
                const Int32         iRegion          = UnderlyingFrom(region);
                const eOctreeRegion grandchildRegion = k_grandchildTable[iRegion];

                OcNode*      pNewRootChildNode  = pNewRoot->children.Get(region);
                OcNodeScoped pOldRootChildNode  = m_pRootNode->children.Pop(region);
                const Int32  numItemsInOldChild = pOldRootChildNode->numItemsInTree;

                // update node parent and depth
                pOldRootChildNode->pParentNode = pNewRootChildNode;
                pOldRootChildNode->depth       = pNewRootChildNode->depth + 1;

                // transfer
                pNewRootChildNode->children.Set(grandchildRegion, std::move(pOldRootChildNode));

                // sync numItemsInTree
                pNewRootChildNode->numItemsInTree = numItemsInOldChild;
                pNewRoot->numItemsInTree += numItemsInOldChild;
            }

            // 구 루트 자체에 있던 아이템은 모두 새로 삽입.
            // 이로써 신 루트의 numItemsInTree가 정확히 계산됨
            OcItemScopedList oldRootItems = std::move(m_pRootNode->items);
            for (OcItemScoped& pItem: oldRootItems)
            {
                Insert_Recursive_(pNewRoot.GetPtr(), std::move(pItem));
            }

            // 새로운 루트 설정
            m_pRootNode = std::move(pNewRoot);

            // post process
            {
                // 정책에 따라 max depth 증가
                if (m_desc.bIncreaseMaxDepthOnExpand)
                {
                    m_desc.maxDepth += 1;
                }

                // 모든 노드의 깊이 갱신
                UpdateDepth_();

                // max depth 정책에 따라 너무 깊은 노드 병합
                if (m_desc.bUseMaxDepthPolicy && !m_desc.bIncreaseMaxDepthOnExpand)
                {
                    MergeTooDeepChildren_();
                }
            }
        }
        else
        {
            // 기존 root가 자식이 없으면.. 단순히 기존 root의 아이템들을 새 루트에 옮기기만 하면 됨
            OcItemScopedList oldRootItems = std::move(m_pRootNode->items);
            for (OcItemScoped& pItem: oldRootItems)
            {
                Insert_Recursive_(pNewRoot.GetPtr(), std::move(pItem));
            }

            // 새로운 루트 설정
            m_pRootNode = std::move(pNewRoot);
        }
    }

    [[nodiscard]] OcNodeScoped NewNode_()
    {
        const OcNodeDeletor deletor { &m_nodePool };
        return OcNodeScoped { m_nodePool.New(), deletor };
    }

    [[nodiscard]] OcItemScoped NewItem_()
    {
        const OcItemDeletor deletor { &m_itemPool };
        return OcItemScoped { m_itemPool.New(), deletor };
    }

    [[nodiscard]] OcItemScoped MakeItem_(
        const Entity _entity,
        const Aabb&  _aabb,
        const T&     _userdata)
    {
        OcItemScoped pItem = NewItem_();
        pItem->entity      = _entity;
        pItem->aabb        = _aabb;
        pItem->userdata    = _userdata;
        return pItem;
    }

    [[nodiscard]] OcNodeScoped MakeNode_(
        const Aabb& _aabb)
    {
        OcNodeScoped pNode = NewNode_();
        pNode->aabb        = _aabb;
        return pNode;
    }

    void UpdateDepth_()
    {
        m_pRootNode->depth = 0;
        UpdateDepth_Recursive_(m_pRootNode.GetPtr());
    }

    void UpdateDepth_Recursive_(
        const OcNode* _pRoot)
    {
        // 탑-다운 방식. 부모 노드가 자식의 깊이를 결정
        if (_pRoot->HasChildren())
        {
            for (const OcNodeScoped& pChildNode: _pRoot->children)
            {
                pChildNode->depth = _pRoot->depth + 1;
                UpdateDepth_Recursive_(pChildNode.GetPtr());
            }
        }
    }

    void MergeTooDeepChildren_()
    {
        MergeTooDeepChildren_Recursive_(m_pRootNode.GetPtr());
    }

    void MergeTooDeepChildren_Recursive_(
        OcNode* _pNode)
    {
        if (_pNode->HasChildren())
        {
            // 바텀-업으로 깊이가 너무 깊은 자식은 병합
            for (const OcNodeScoped& pChildNode: _pNode->children)
            {
                MergeTooDeepChildren_Recursive_(pChildNode.GetPtr());
            }

            // then merge this node's children if too deep
            if (_pNode->depth >= m_desc.maxDepth)
            {
                // 자식들의 아이템들을 모두 현재 노드로 이동
                // numItemsInTree는 이미 정확하므로, 아이템들만 이동시키면 됨
                for (const OcNodeScoped& pChildNode: _pNode->children)
                {
                    OcItemScopedList childItems = std::move(pChildNode->items);
                    for (OcItemScoped& pItem: childItems)
                    {
                        AddItemToNode_(_pNode, std::move(pItem));
                    }
                }

                _pNode->children.Clear();
            }
        }
    }

    void ShrinkToFitMergeEmptyChildren_Recursive_(
        OcNode* _pNode)
    {
        if (_pNode->HasChildren())
        {
            // 바텀-업으로 빈 자식 노드들을 제거
            for (const OcNodeScoped& pChildNode: _pNode->children)
            {
                ShrinkToFitMergeEmptyChildren_Recursive_(pChildNode.GetPtr());
            }

            // numItemsInTree 확인하여 빈 자식 노드 제거
            const Int32 numItemsInCurNode = NumericCast<Int32>(_pNode->items.size());
            if (_pNode->numItemsInTree - numItemsInCurNode == 0)   // 현재 노드에만 아이템이 있고, 모든 자식 노드가 비어있는 경우 병합
            {
                // 모든 자식들은 비어있으므로 단순 노드 제거로 충분
                _pNode->children.Clear();
            }
        }
    }

    [[nodiscard]] OcNode* FindBestChildNode_(   // or nullptr
        const OcNode* _pRoot,
        const Aabb&   _aabb) const
    {
        JAM_ASSERT(_pRoot->HasChildren(), "Node has no children");
        JAM_ASSERT(_pRoot->Contain(_aabb), "Aabb does not fit in node");

        // determine quadrant based on center point
        const Vec3 center     = _pRoot->GetCenter();
        const Vec3 aabbCenter = _aabb.GetCenter();

        Int32 iRegion = 0;
        if (aabbCenter.x > center.x) iRegion |= 0b0001;   // Right
        if (aabbCenter.y < center.y) iRegion |= 0b0010;   // Bottom
        if (aabbCenter.z > center.z) iRegion |= 0b0100;   // Far
        const eOctreeRegion region = static_cast<eOctreeRegion>(iRegion);

        // check if candidate child contains the aabb
        OcNode* pNode = _pRoot->children.Get(region);
        return pNode->Contain(_aabb) ? pNode : nullptr;
    }

    ObjectPool<OcItem> m_itemPool;
    ObjectPool<OcNode> m_nodePool;
    OctreeDesc         m_desc = {};

    OcNodeScoped          m_pRootNode = nullptr;
    HashMap<UID, OcItem*> m_itemCache = {};
};
}   // namespace Jam
