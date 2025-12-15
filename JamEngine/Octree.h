#pragma once
#include "Entity.h"
#include "MemoryPool.h"

namespace jam
{

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

template<typename T>
class Octree
{
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

    using OcNodePtr  = std::unique_ptr<OcNode, OcNodeDeletor>;
    using OcItemPtr  = std::unique_ptr<OcItem, OcItemDeletor>;
    using OcItemList = Vector<OcItemPtr>;

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

        Entity  entity     = Entity::s_null;   // owner entity
        Aabb    aabb       = {};               // bounding box
        OcNode* pOwnerNode = nullptr;          // owner node
        Int32   index      = -1;               // index in owner's item list
        T       userdata   = {};               // user data
    };

    enum class eRegion
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

    constexpr static eRegion k_invalidRegion = static_cast<eRegion>(-1);

    struct OcNode
    {
        [[nodiscard]] bool IsLeaf() const
        {
            return childNodes[0] == nullptr;
        }

        [[nodiscard]] bool HasChildren() const
        {
            return childNodes[0] != nullptr;
        }

        [[nodiscard]] bool HasGrandChildren() const
        {
            return HasChildren() && childNodes[0]->HasChildren();
        }

        [[nodiscard]] bool HasParent() const
        {
            return pParentNode != nullptr;
        }

        [[nodiscard]] bool Contain(const Aabb& _aabb) const
        {
            return aabb.Contain(_aabb);
        }

        [[nodiscard]] bool Contain(const OcItem* _pItem) const
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

        OcNodePtr childNodes[EnumCount<eRegion>()] = { nullptr };
        OcNode*   pParentNode                      = nullptr;

        OcItemList items          = {};
        Aabb       aabb           = {};
        Int32      depth          = -1;
        Int32      numItemsInTree = 0;   // total number of items in this node and all children
    };

public:
    explicit Octree(
        const OctreeDesc& _desc = {})
        : m_desc(_desc)
        , m_itemPool(2048)
        , m_nodePool(512)
    {
        JAM_ASSERT(_desc.looseFactor >= 1.f, "looseFactor must be >= 1.f");
        JAM_ASSERT(_desc.maxDepth >= 2, "maxDepth must be >= 2");
        JAM_ASSERT(_desc.subdivisionThreshold > 0, "subdivisionThreshold must be > 0");
        JAM_ASSERT(_desc.minBaseExtents.x > 1.f && _desc.minBaseExtents.y > 1.f && _desc.minBaseExtents.z > 1.f, "minBaseExtents must be > 1.f");
        JAM_ASSERT(_desc.initialBaseExtents.x >= _desc.minBaseExtents.x && _desc.initialBaseExtents.y >= _desc.minBaseExtents.y && _desc.initialBaseExtents.z >= _desc.minBaseExtents.z, "initialBaseExtents must be >= minBaseExtents");

        const Vec3 ext  = m_desc.initialBaseExtents * m_desc.looseFactor;
        const Aabb aabb = Aabb::FromCenterExtents(Vec3::Zero, ext);

        m_pRoot        = MakeNode_(aabb);
        m_pRoot->depth = 0;
    }

    void Insert(
        const Entity _entity,
        const Aabb&  _aabb,
        const T&     _userdata)
    {
        JAM_ASSERT(_entity, "Invalid entity");
        JAM_ASSERT(m_itemCache.contains(_entity.GetUID()) == false, "Entity already exists");

        // _aabb를 포함할 때까지 확장
        while (!m_pRoot->Contain(_aabb))
        {
            ExpandOctree_();
        }

        // while 문을 통과헀다면, 반드시 insert 가능
        OcItemPtr pItem = MakeItem_(_entity, _aabb, _userdata);
        Insert_Recursive_(m_pRoot.get(), std::move(pItem));
    }

    void Update(
        const Entity _entity,
        const Aabb&  _aabb)
    {
        JAM_ASSERT(_entity, "Invalid entity");

        const auto it = m_itemCache.find(_entity.GetUID());
        JAM_ASSERT(it != m_itemCache.end(), "Entity does not exist");

        // update aabb
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
        OcNode* pFitRoot   = pOwnerNode;
        while (pFitRoot && !pFitRoot->Contain(pItem))
        {
            --pFitRoot->numItemsInTree;   // 부모로 올라가기 전에 기존 트리의 numItemsInTree 감소
            pFitRoot = pFitRoot->pParentNode;
        }

        // 만약 기존과 같은 트리에 머무른다면
        if (pFitRoot == pOwnerNode)
        {
            // 자식이 있다면 다식한테 넣어봄
            if (pOwnerNode->HasChildren())
            {
                // 맞는 자식이 있다면, 그 자식 노드로 이동
                // numItemsInTree 는 변하지 않음
                if (OcNode* pChildRoot = FindBestChildNodeOrNull_(pOwnerNode, _aabb))
                {
                    Insert_Recursive_(pChildRoot, PopItemFromList_(pItem));
                }
            }
            // 자식이 없다면, 혹은 맞는 자식이 없다면
            // 그대로 현재 노드에 머무름 (할 일 없음)
        }
        else   // 다른 트리로 이동해야 한다면
        {
            // 아이템이 아예 현재 옥트리 밖에 존재
            if (pFitRoot == nullptr)
            {
                // expand octree에서 재삽입이 꼬일 수 있기 때문에, 미리 아이템을 제거
                OcItemPtr popedItem = PopItemFromList_(pItem);

                // 옥트리 확장
                while (!m_pRoot->Contain(_aabb))
                {
                    ExpandOctree_();
                }

                // 재삽입
                Insert_Recursive_(m_pRoot.get(), std::move(popedItem));
            }
            else
            {
                --pFitRoot->numItemsInTree;   // Insert_Recursive_ 에서 증가시켜주기 때문에 미리 감소
                Insert_Recursive_(pFitRoot, PopItemFromList_(pItem));
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
        OcItem* pItem = it->second;
        OcNode* pNode = pItem->pOwnerNode;
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
        // shrink root if possible
        if (m_pRoot->items.empty())   // if root have any items, cannot shrink
        {
            OcNode* pCurNode  = m_pRoot.get();
            eRegion fitRegion = k_invalidRegion;

            // find child that contains all items
            while (pCurNode->HasChildren())
            {
                // 너무 작아질 경우 스킵
                const Vec3 baseExt = pCurNode->GetExtents() / m_desc.looseFactor;
                if (baseExt.x <= m_desc.initialBaseExtents.x
                    && baseExt.y <= m_desc.initialBaseExtents.y
                    && baseExt.z <= m_desc.initialBaseExtents.z)
                {
                    break;
                }

                // 현재 트리의 모든 아이템을 포함하는 자식 노드 탐색
                // 최종적으로 발견한 자식 노드가 재로운 루트가 될것
                eRegion nextFitRegion = k_invalidRegion;
                for (eRegion region: EnumRanges<eRegion>())
                {
                    const Int32 iRegion    = IntFrom(region);
                    OcNode*     pChildNode = pCurNode->childNodes[iRegion].get();
                    if (pChildNode->numItemsInTree == m_pRoot->numItemsInTree)
                    {
                        pCurNode      = pChildNode;
                        nextFitRegion = region;
                        break;
                    }
                    else if (pChildNode->numItemsInTree != 0)
                    {
                        // 만약, 하나의 자식 노드라도 일부 아이템을 가지고 있다면, 더 이상 진행하지 않음
                        break;
                    }
                }

                // true: 각각의 자식이 모두 조금씩 가지고 있거나, 아예 없는 경우 -> 즉, 하나의 자식이 모든 아이템을 포함하지 않는 경우
                if (nextFitRegion == k_invalidRegion)
                {
                    break;
                }

                // continue to next level
                fitRegion = nextFitRegion;
            }

            // shrink 할 노드를 찾은 경우. 루트로 승격
            if (fitRegion != k_invalidRegion)
            {
                const Int32 iRegion   = IntFrom(fitRegion);
                OcNodePtr   pNewRoot  = std::move(pCurNode->pParentNode->childNodes[iRegion]);
                pNewRoot->pParentNode = nullptr;
                m_pRoot               = std::move(pNewRoot);
            }
        }

        // 빈 자식 노드 병합
        ShrinkToFitMergeEmptyChildren_Recursive_(m_pRoot.get());
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
        Query_Recursive_(m_pRoot.get(), _bv, _intersactFunc, _collectFunc);
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
        for (const OcItemPtr& pItem: _pNode->items)
        {
            if (_intersactFunc(_bv, pItem->aabb))
            {
                _collectFunc(pItem->entity, pItem->userdata);
            }
        }

        // check children
        if (_pNode->HasChildren())
        {
            for (eRegion region: EnumRanges<eRegion>())
            {
                const Int32 iRegion = IntFrom(region);
                Query_Recursive_(_pNode->childNodes[iRegion].get(), _bv, _intersactFunc, _collectFunc);
            }
        }
    }

    void SplitNode_(
        OcNode* _pNode)
    {
        JAM_ASSERT(_pNode->IsLeaf(), "Node already has children");

        const Vec3 center       = _pNode->GetCenter();
        const Vec3 childExt     = _pNode->GetExtents() * 0.5f;
        const Vec3 childBaseExt = childExt / m_desc.looseFactor;

        for (eRegion region: EnumRanges<eRegion>())
        {
            const Int32 iRegion = IntFrom(region);

            Vec3 childCenter = center;
            childCenter.x += (iRegion & 0b0001) ? childBaseExt.x : -childBaseExt.x;
            childCenter.y += (iRegion & 0b0010) ? -childBaseExt.y : childBaseExt.y;
            childCenter.z += (iRegion & 0b0100) ? childBaseExt.z : -childBaseExt.z;

            const Aabb aabb             = Aabb::FromCenterExtents(childCenter, childExt);
            OcNodePtr  pChildNode       = MakeNode_(aabb);
            pChildNode->pParentNode     = _pNode;
            pChildNode->depth           = _pNode->depth + 1;
            _pNode->childNodes[iRegion] = std::move(pChildNode);
        }
    }

    void SubdivideIfNeed_(
        OcNode* _pNode)
    {
        if (!_pNode->IsLeaf())
        {
            return;   // already subdivided
        }

        const Int32 numItems = NarrowCast<Int32>(_pNode->items.size());
        if (numItems <= m_desc.subdivisionThreshold)
        {
            return;   // below threshold
        }

        if (m_desc.bUseMaxDepthPolicy && _pNode->depth >= m_desc.maxDepth)
        {
            return;   // max depth reached
        }

        // check if children would be too small
        const Vec3 ext          = _pNode->GetExtents();
        const Vec3 childExt     = ext * 0.5f;
        const Vec3 childBaseExt = childExt / m_desc.looseFactor;

        if (childBaseExt.x < m_desc.minBaseExtents.x
            || childBaseExt.y < m_desc.minBaseExtents.y
            || childBaseExt.z < m_desc.minBaseExtents.z)
        {
            return;   // would violate min size constraint
        }

        // perform split
        SplitNode_(_pNode);

        // redistribute items to children
        OcItemList existingItems = std::move(_pNode->items);
        _pNode->numItemsInTree   = 0;   // will be recalculated in Insert_Recursive_
        for (OcItemPtr& pItem: existingItems)
        {
            Insert_Recursive_(_pNode, std::move(pItem));
        }
    }

    void AddItemToNode_(
        OcNode*     _pNode,
        OcItemPtr&& _pItem)
    {
        const UID uid      = _pItem->entity.GetUID();
        m_itemCache[uid]   = _pItem.get();
        _pItem->pOwnerNode = _pNode;

        PushItemToList_(_pNode->items, std::move(_pItem));
        SubdivideIfNeed_(_pNode);
    }

    void PushItemToList_(
        OcItemList& _items,
        OcItemPtr&& _pItem) const
    {
        _pItem->index = NarrowCast<Int32>(_items.size());
        _items.emplace_back(std::move(_pItem));
    }

    OcItemPtr PopItemFromList_(
        const OcItem* _pItem) const
    {
        OcItemList& items      = _pItem->pOwnerNode->items;
        const Int32 itemIdx    = _pItem->index;
        const Int32 lastIdx    = NarrowCast<Int32>(items.size() - 1);
        OcItemPtr   pPopedItem = std::move(items[itemIdx]);

        if (itemIdx != lastIdx)
        {
            // swap with last and update index
            items[itemIdx]        = std::move(items[lastIdx]);
            items[itemIdx]->index = itemIdx;
        }

        items.pop_back();

        pPopedItem->pOwnerNode = nullptr;
        pPopedItem->index      = -1;
        return pPopedItem;
    }

    void Insert_Recursive_(
        OcNode*     _pRoot,
        OcItemPtr&& _pItem)
    {
        JAM_ASSERT(_pRoot->Contain(_pItem->aabb), "Item does not fit in node");

        // Insert_Recursive_ 호출 전체조건, _pRoot는 반드시 _pItem를 포함해야 함
        // 삽입은 보장되므로 numItemsInTree 증가
        ++_pRoot->numItemsInTree;

        if (_pRoot->HasChildren())
        {
            if (OcNode* pBestChild = FindBestChildNodeOrNull_(_pRoot, _pItem->aabb))
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
        const Vec3 center = m_pRoot->GetCenter();
        const Vec3 ext    = m_pRoot->GetExtents() * 2.f;
        const Aabb aabb   = Aabb::FromCenterExtents(center, ext);

        OcNodePtr pNewRoot = MakeNode_(aabb);
        pNewRoot->depth    = 0;

        if (m_pRoot->HasChildren())   // 기존 root가 자식이 있으면..
        {
            // 이중 분할 후. 중심은 유지하고, 기존 root의 자식들을 손자 노드로 이동
            SplitNode_(pNewRoot.get());
            for (eRegion region: EnumRanges<eRegion>())
            {
                const Int32 iRegion = IntFrom(region);
                SplitNode_(pNewRoot->childNodes[iRegion].get());
            }

            // 매핑 테이블, 기존 root의 자식 노드들을 새로운 손자 노드 위치로 이동
            constexpr eRegion k_grandchildMapping[EnumCount<eRegion>()] = {
                eRegion::FarRightBottom,
                eRegion::FarLeftBottom,
                eRegion::FarRightTop,
                eRegion::FarLeftTop,
                eRegion::NearRightBottom,
                eRegion::NearLeftBottom,
                eRegion::NearRightTop,
                eRegion::NearLeftTop
            };

            // move old children to new grandchildren positions
            Int32 numOldRootChildrenItems = 0;
            for (eRegion region: EnumRanges<eRegion>())
            {
                const Int32   iRegion           = IntFrom(region);
                const eRegion grandchildRegion  = k_grandchildMapping[iRegion];
                const Int32   iGrandchildRegion = IntFrom(grandchildRegion);

                OcNode*     pNewRootChildNode  = pNewRoot->childNodes[iRegion].get();
                OcNodePtr&  pOldRootChildNode  = m_pRoot->childNodes[iRegion];
                const Int32 numItemsInOldChild = pOldRootChildNode->numItemsInTree;

                // transfer ownership
                pOldRootChildNode->pParentNode                   = pNewRootChildNode;
                pOldRootChildNode->depth                         = pNewRootChildNode->depth + 1;
                pNewRootChildNode->childNodes[iGrandchildRegion] = std::move(pOldRootChildNode);
                numOldRootChildrenItems += numItemsInOldChild;   // accumulate item count
            }
            pNewRoot->numItemsInTree = numOldRootChildrenItems;   // old root's numItemsInTree will be recalculated below

            // 구 루트 자체에 있던 아이템은 모두 새로 삽입.
            // 이로써 신 루트의 numItemsInTree가 정확히 계산됨
            OcItemList oldRootItems = std::move(m_pRoot->items);
            for (OcItemPtr& pItem: oldRootItems)
            {
                Insert_Recursive_(pNewRoot.get(), std::move(pItem));
            }

            // policy: increase max depth on expand
            if (m_desc.bIncreaseMaxDepthOnExpand)
            {
                m_desc.maxDepth += 1;
            }

            // update all depths
            UpdateDepth_Recursive_(pNewRoot.get());

            // policy: merge children that exceed max depth
            if (m_desc.bUseMaxDepthPolicy && !m_desc.bIncreaseMaxDepthOnExpand)
            {
                MergeTooDeepChildren_Recursive_(pNewRoot.get());
            }
        }
        else
        {
            // 기존 root가 자식이 없으면.. 다순히 기존 root의 아이템들을 새 루트에 옮기기만 하면 됨
            OcItemList oldRootItems = std::move(m_pRoot->items);
            for (OcItemPtr& pItem: oldRootItems)
            {
                Insert_Recursive_(pNewRoot.get(), std::move(pItem));
            }
        }

        // set new root
        m_pRoot = std::move(pNewRoot);
    }

    [[nodiscard]] OcNodePtr NewNode_()
    {
        OcNodeDeletor deletor { &m_nodePool };
        return OcNodePtr(m_nodePool.New(), deletor);
    }

    [[nodiscard]] OcItemPtr NewItem_()
    {
        OcItemDeletor deletor { &m_itemPool };
        return OcItemPtr(m_itemPool.New(), deletor);
    }

    [[nodiscard]] OcItemPtr MakeItem_(
        const Entity _entity,
        const Aabb&  _aabb,
        const T&     _userdata)
    {
        OcItemPtr pItem = NewItem_();
        pItem->entity   = _entity;
        pItem->aabb     = _aabb;
        pItem->userdata = _userdata;
        return pItem;
    }

    [[nodiscard]] OcNodePtr MakeNode_(
        const Aabb& _aabb)
    {
        OcNodePtr pNode = NewNode_();
        pNode->aabb     = _aabb;
        return pNode;
    }

    void UpdateDepth_Recursive_(
        const OcNode* _pRoot)
    {
        // 탑-다운 방식. 부모 노드가 자식의 깊이를 결정
        if (_pRoot->HasChildren())
        {
            for (eRegion region: EnumRanges<eRegion>())
            {
                const Int32 iRegion = IntFrom(region);
                OcNode*     pChild  = _pRoot->childNodes[iRegion].get();
                pChild->depth       = _pRoot->depth + 1;
                UpdateDepth_Recursive_(pChild);
            }
        }
    }

    void MergeTooDeepChildren_Recursive_(
        OcNode* _pRoot)
    {
        if (_pRoot->HasChildren())
        {
            // 바텀-업으로 깊이가 너무 깊은 자식은 병합
            for (eRegion region: EnumRanges<eRegion>())
            {
                const Int32 iRegion = IntFrom(region);
                MergeTooDeepChildren_Recursive_(_pRoot->childNodes[iRegion].get());
            }

            // then merge this node's children if too deep
            if (_pRoot->depth >= m_desc.maxDepth)
            {
                // 자식들의 아이템들을 모두 현재 노드로 이동
                // numItemsInTree는 이미 정확하므로, 아이템들만 이동시키면 됨
                for (eRegion region: EnumRanges<eRegion>())
                {
                    const Int32 iRegion    = IntFrom(region);
                    OcNode*     pChild     = _pRoot->childNodes[iRegion].get();
                    OcItemList  childItems = std::move(pChild->items);

                    for (OcItemPtr& pItem: childItems)
                    {
                        AddItemToNode_(_pRoot, std::move(pItem));
                    }

                    _pRoot->childNodes[iRegion] = nullptr;
                }
            }
        }
    }

    void ShrinkToFitMergeEmptyChildren_Recursive_(
        OcNode* _pNode)
    {
        if (_pNode->HasChildren())
        {
            // 바텀-업으로 빈 자식 노드들을 제거
            for (eRegion region: EnumRanges<eRegion>())
            {
                const Int32 iRegion = IntFrom(region);
                ShrinkToFitMergeEmptyChildren_Recursive_(_pNode->childNodes[iRegion].get());
            }

            // numItemsInTree 확인하여 빈 자식 노드 제거
            const Int32 numItemsInCurNode = NarrowCast<Int32>(_pNode->items.size());
            if (_pNode->numItemsInTree - numItemsInCurNode == 0)   // 현재 노드에만 아이템이 있고, 모든 자식 노드가 비어있음
            {
                // all children empty, remove them
                for (eRegion region: EnumRanges<eRegion>())
                {
                    const Int32 iRegion         = IntFrom(region);
                    _pNode->childNodes[iRegion] = nullptr;
                }
            }
        }
    }

    [[nodiscard]] OcNode* FindBestChildNodeOrNull_(
        const OcNode* _pRoot,
        const Aabb&   _aabb) const
    {
        JAM_ASSERT(_pRoot->HasChildren(), "Node has no children");

        // determine quadrant based on center point
        const Vec3 rootCenter = _pRoot->GetCenter();
        const Vec3 aabbCenter = _aabb.GetCenter();

        Int32 quadrant = 0;
        if (aabbCenter.x > rootCenter.x) quadrant |= 0b0001;   // Right
        if (aabbCenter.y < rootCenter.y) quadrant |= 0b0010;   // Bottom
        if (aabbCenter.z > rootCenter.z) quadrant |= 0b0100;   // Far

        // check if candidate child contains the aabb
        OcNode* pCandidateNode = _pRoot->childNodes[quadrant].get();
        return pCandidateNode->Contain(_aabb) ? pCandidateNode : nullptr;
    }

    OctreeDesc         m_desc = {};
    ObjectPool<OcItem> m_itemPool;
    ObjectPool<OcNode> m_nodePool;

    OcNodePtr             m_pRoot     = nullptr;
    HashMap<UID, OcItem*> m_itemCache = {};
};
}   // namespace jam
