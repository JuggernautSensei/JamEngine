#pragma once
#include "Entity.h"
#include "MemoryPool.h"
#include "BoundingVolume.h"

namespace Jam
{

struct OctreeDesc
{
    Vec3 minBaseExtents  = Vec3(4.f);      // 가장 작은 베이스 aabb extents. 이것보다 작게 분할하지 않음
    Vec3 initBaseExtents = Vec3(1024.f);   // 초기 베이스 aabb extents

    Int32 maxDepth                  = 8;
    bool  bUseMaxDepthPolicy        = true;   // true: 분할 시 maxDepth 강제; false: minBaseExtents 도달 시까지 분할
    bool  bIncreaseMaxDepthOnExpand = true;   // true: 확장 시 maxDepth 증가; false: maxDepth 유지 (확장 후 초과 노드 병합)

    Int32 subdivisionThreshold = 16;    // 노드당 아이템 수가 이 값을 초과하면 분할
    float looseFactor          = 2.f;   // loose factor (>= 1.f). 1.f = tight octree
};

template<typename TBoundingVolume, typename TIntersectFunc>
concept IsOctreeIntersectFunc = requires(TBoundingVolume _bv, const Aabb& _aabb, TIntersectFunc&& _func) { { _func(_bv, _aabb) } -> std::convertible_to<bool>; };

template<typename TUserData, typename TCollectFunc>
concept IsOctreeCollectFunc = requires(Entity _entity, const TUserData& _userdata, TCollectFunc&& _func) { { _func(_entity, _userdata) }; };

template<typename TUserData, typename TTraverseFunc>
concept IsOctreeTraverseFunc = requires(Entity _entity, const Aabb& _aabb, const TUserData& _userdata, TTraverseFunc&& _func) { { _func(_entity, _aabb, _userdata) }; };

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

    using OcNodeScoped = Scoped<OcNode, OcNodeDeletor>;
    using OcItemScoped = Scoped<OcItem, OcItemDeletor>;

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

        Entity  entity   = Entity::k_null;
        Aabb    aabb     = {};
        OcNode* pOwner   = nullptr;
        Int32   index    = -1;
        T       userdata = {};
    };

    struct OcNode
    {
        [[nodiscard]] bool IsLeaf() const
        {
            return children[0] == nullptr;
        }

        [[nodiscard]] bool HasChildren() const
        {
            return children[0] != nullptr;
        }

        [[nodiscard]] bool HasGrandChildren() const
        {
            return HasChildren() && children[0]->HasChildren();
        }

        [[nodiscard]] bool HasParent() const
        {
            return pParent != nullptr;
        }

        void ClearChildren()
        {
            for (OcNodeScoped& child: children)
            {
                child.Reset();
            }
        }

        [[nodiscard]] bool Contains(const Aabb& _aabb) const
        {
            return aabb.Contains(_aabb);
        }

        [[nodiscard]] bool Contains(const OcItem* _pItem) const
        {
            return aabb.Contains(_pItem->aabb);
        }

        [[nodiscard]] Vec3 GetCenter() const
        {
            return aabb.GetCenter();
        }

        [[nodiscard]] Vec3 GetExtents() const
        {
            return aabb.GetExtents();
        }

        EnumArray<eRegion, OcNodeScoped> children       = {};
        OcNode*                          pParent        = nullptr;
        Vector<OcItemScoped>             items          = {};
        Aabb                             aabb           = {};
        Int32                            depth          = -1;
        Int32                            numItemsInTree = 0;   // 이 노드 + 모든 자손의 아이템 수
    };

public:
    explicit Octree(const OctreeDesc& _desc = {})
        : m_itemPool(2048)
        , m_nodePool(512)
        , m_desc(_desc)
    {
        JAM_ASSERT(_desc.looseFactor >= 1.f, "looseFactor must be >= 1.f");
        JAM_ASSERT(_desc.maxDepth >= 2, "maxDepth must be >= 2");
        JAM_ASSERT(_desc.subdivisionThreshold > 0, "subdivisionThreshold must be > 0");
        JAM_ASSERT(_desc.minBaseExtents.x > 1.f && _desc.minBaseExtents.y > 1.f && _desc.minBaseExtents.z > 1.f, "minBaseExtents must be > 1.f");
        JAM_ASSERT(_desc.initBaseExtents.x >= _desc.minBaseExtents.x && _desc.initBaseExtents.y >= _desc.minBaseExtents.y && _desc.initBaseExtents.z >= _desc.minBaseExtents.z, "initBaseExtents must be >= minBaseExtents");

        const Vec3 ext  = m_desc.initBaseExtents * m_desc.looseFactor;
        const Aabb aabb = Aabb::FromCenterExtents(Numbers<Vec3>::k_zero, ext);
        m_pRoot         = MakeNode_(aabb);
        m_pRoot->depth  = 0;
    }

    void Insert(
        const Entity _entity,
        const Aabb&  _aabb,
        const T&     _userdata)
    {
        JAM_ASSERT(_entity, "Invalid entity");
        JAM_ASSERT(!m_itemCache.contains(_entity.GetUID()), "Entity already exists");

        while (!m_pRoot->Contains(_aabb))   // 필요시 확장
        {
            Expand_(_aabb);
        }

        OcItemScoped pItem = MakeItem_(_entity, _aabb, _userdata);
        Insert_Recursive_(m_pRoot.GetPtr(), std::move(pItem));
    }

    void Update(
        const Entity _entity,
        const Aabb&  _aabb)
    {
        JAM_ASSERT(_entity, "Invalid entity");

        const auto it = m_itemCache.find(_entity.GetUID());
        JAM_ASSERT(it != m_itemCache.end(), "Entity does not exist");

        OcItem* pItem = it->second;
        pItem->aabb   = _aabb;

        // 자신을 포함하는 가장 작은 조상 노드를 바텀-업으로 탐색.
        // 벗어난 노드들의 numItemsInTree 를 감소시키면서 올라감.
        OcNode* pOwnerNode = pItem->pOwner;
        OcNode* pNode      = pOwnerNode;
        while (pNode && !pNode->Contains(pItem))
        {
            --pNode->numItemsInTree;
            pNode = pNode->pParent;
        }

        // 시나리오
        // 1. 같은 노드에 머무르는 경우
        // 2. 옥트리 밖으로 벗어나는 경우
        // 3. 옥트리 내 다른 서브트리로 이동하는 경우

        if (pNode == pOwnerNode)
        {
            // 자식 노드로 이동할 수 있는지 확인
            if (pOwnerNode->HasChildren())
            {
                if (OcNode* pChildNode = FindBestChildNodeOrNull_(pOwnerNode, _aabb))
                {
                    Insert_Recursive_(pChildNode, PopItemFromList_(pItem));
                }

                // 적합한 자식 없으면 현재 노드에 그대로 유지
            }
        }
        else if (pNode == nullptr)
        {
            // 옥트리 밖으로 벗어나는 경우. 루트부터 다시 삽입.
            // 확장 코드가 꼬일 수 있기 때문에 pItem은 제거 후 다시 삽입
            OcItemScoped pPoppedItem = PopItemFromList_(pItem);
            while (!m_pRoot->Contains(_aabb))
            {
                Expand_(_aabb);
            }

            Insert_Recursive_(m_pRoot.GetPtr(), std::move(pPoppedItem));
        }
        else
        {
            // 옥트리 내 다른 서브트리로 이동.
            // while 루프에서 pNode 는 감소되지 않았고,
            // Insert_Recursive_ 가 pNode 부터 시작해 다시 증가시키므로 미리 감소.
            --pNode->numItemsInTree;
            Insert_Recursive_(pNode, PopItemFromList_(pItem));
        }
    }

    void Remove(
        const Entity _entity)
    {
        JAM_ASSERT(_entity, "Invalid entity");

        const auto it = m_itemCache.find(_entity.GetUID());
        JAM_ASSERT(it != m_itemCache.end(), "Entity does not exist");

        // 자신을 포함한 모든 조상의 numItemsInTree 를 감소
        const OcItem* pItem = it->second;
        for (OcNode* pNode = pItem->pOwner; pNode; pNode = pNode->pParent)
        {
            --pNode->numItemsInTree;
        }

        PopItemFromList_(pItem);
        m_itemCache.erase(it);
    }

    void ShrinkToFit()
    {
        // 모든 아이템을 포함하는 서브트리를 탐색하고 새 루트로 승격
        // 옥트리는 initBaseExtents 보다 작아질 수 없음
        const Vec3 minLooseExt = m_desc.initBaseExtents * m_desc.looseFactor;
        if (m_pRoot->items.empty() && m_pRoot->numItemsInTree > 0)
        {
            OcNode* pRootNode    = m_pRoot.GetPtr();
            OcNode* pTargetNode  = pRootNode;
            eRegion targetRegion = eRegion::LeftBottomNear;   // 초기값은 의미 없음

            while (pTargetNode->HasChildren())
            {
                // 너무 작으면 더 이상 탐색하지 않음
                const Vec3 looseExt = pTargetNode->GetExtents();
                if (looseExt.x <= minLooseExt.x && looseExt.y <= minLooseExt.y && looseExt.z <= minLooseExt.z)
                {
                    break;
                }

                // 모든 아이템을 포함하는 단일 자식 탐색
                bool bExit = true;
                for (const eRegion region: RangesOf<eRegion>())
                {
                    OcNode* pChild = pTargetNode->children[region].GetPtr();
                    if (pChild->numItemsInTree == m_pRoot->numItemsInTree)
                    {
                        pTargetNode  = pChild;
                        targetRegion = region;
                        bExit        = false;
                        break;
                    }
                    else if (pChild->numItemsInTree != 0)   // 다른 자식에도 아이템이 있으면 shrink 불가
                    {
                        break;
                    }
                }

                if (bExit)
                {
                    break;
                }
            }

            if (pTargetNode != pRootNode)
            {
                OcNodeScoped pNewRoot = std::move(pTargetNode->pParent->children[targetRegion]);
                pNewRoot->pParent     = nullptr;
                m_pRoot               = std::move(pNewRoot);
                UpdateDepth_();   // 깊이 다시 계산
            }
        }

        // 아이템이 없는 빈 노드 병합
        MergeEmptyChildren_Recursive_(m_pRoot.GetPtr());
    }

    // 어떤 _bv와 충돌하는 모든 Entity 탐색
    template<typename TBoundingVolume, typename IntersectFunc, typename CollectFunc>
        requires(IsOctreeIntersectFunc<TBoundingVolume, IntersectFunc> && IsOctreeCollectFunc<T, CollectFunc>)
    void Query(const TBoundingVolume& _bv, IntersectFunc&& _intersectFunc, CollectFunc&& _collectFunc) const
    {
        Query_Recursive_(m_pRoot.GetPtr(), _bv, std::forward<IntersectFunc>(_intersectFunc), std::forward<CollectFunc>(_collectFunc));
    }

    template<typename TraverseFunc>
        requires(IsOctreeTraverseFunc<T, TraverseFunc>)
    void TopDownTraverse(TraverseFunc&& _func) const
    {
        TopDownTraverse_Recursive_(m_pRoot.GetPtr(), std::forward<TraverseFunc>(_func));
    }

    template<typename TraverseFunc>
        requires(IsOctreeTraverseFunc<T, TraverseFunc>)
    void BottomUpTraverse(TraverseFunc&& _func) const
    {
        BottomUpTraverse_Recursive_(m_pRoot.GetPtr(), std::forward<TraverseFunc>(_func));
    }

    [[nodiscard]] SizeT GetNumItems() const
    {
        return m_pRoot->numItemsInTree;
    }

private:
    template<typename TBoundingVolume, typename IntersectFunc, typename CollectFunc>
        requires(IsOctreeIntersectFunc<TBoundingVolume, IntersectFunc> && IsOctreeCollectFunc<T, CollectFunc>)
    void Query_Recursive_(
        const OcNode*          _pNode,
        const TBoundingVolume& _bv,
        IntersectFunc&&        _intersectFunc,
        CollectFunc&&          _collectFunc) const
    {
        if (_pNode->numItemsInTree == 0 || !_intersectFunc(_bv, _pNode->aabb))
        {
            return;
        }

        for (const OcItemScoped& pItem: _pNode->items)
        {
            if (_intersectFunc(_bv, pItem->aabb))
            {
                _collectFunc(pItem->entity, pItem->userdata);
            }
        }

        if (_pNode->HasChildren())
        {
            for (const OcNodeScoped& pChild: _pNode->children)
            {
                Query_Recursive_(pChild.GetPtr(), _bv, std::forward<IntersectFunc>(_intersectFunc), std::forward<CollectFunc>(_collectFunc));
            }
        }
    }

    template<typename TraverseFunc>
        requires(IsOctreeTraverseFunc<T, TraverseFunc>)
    void TopDownTraverse_Recursive_(const OcNode* _pNode, TraverseFunc&& _func) const
    {
        if (_pNode->numItemsInTree == 0)
        {
            return;
        }

        for (const OcItemScoped& pItem: _pNode->items)
        {
            _func(pItem->entity, pItem->aabb, pItem->userdata);
        }

        if (_pNode->HasChildren())
        {
            for (const OcNodeScoped& pChild: _pNode->children)
            {
                TopDownTraverse_Recursive_(pChild.GetPtr(), std::forward<TraverseFunc>(_func));
            }
        }
    }

    template<typename TraverseFunc>
        requires(IsOctreeTraverseFunc<T, TraverseFunc>)
    void BottomUpTraverse_Recursive_(const OcNode* _pNode, TraverseFunc&& _func) const
    {
        if (_pNode->numItemsInTree == 0)
        {
            return;
        }

        if (_pNode->HasChildren())
        {
            for (const OcNodeScoped& pChild: _pNode->children)
            {
                BottomUpTraverse_Recursive_(pChild.GetPtr(), std::forward<TraverseFunc>(_func));
            }
        }

        for (const OcItemScoped& pItem: _pNode->items)
        {
            _func(pItem->entity, pItem->aabb, pItem->userdata);
        }
    }

    void SplitNode_(OcNode* _pNode)
    {
        JAM_ASSERT(_pNode->IsLeaf(), "Node already has children");

        const Vec3 center        = _pNode->GetCenter();
        const Vec3 childLooseExt = _pNode->GetExtents() * 0.5f;
        const Vec3 childBaseExt  = childLooseExt / m_desc.looseFactor;

        for (const eRegion region: RangesOf<eRegion>())
        {
            Vec3 childCenter = center;
            childCenter.x += IsRight(region) ? childBaseExt.x : -childBaseExt.x;
            childCenter.y += IsTop(region) ? childBaseExt.y : -childBaseExt.y;
            childCenter.z += IsFar(region) ? childBaseExt.z : -childBaseExt.z;

            OcNodeScoped pChild      = MakeNode_(Aabb::FromCenterExtents(childCenter, childLooseExt));
            pChild->pParent          = _pNode;
            pChild->depth            = _pNode->depth + 1;
            _pNode->children[region] = std::move(pChild);
        }
    }

    void SubdivideIfNeed_(OcNode* _pNode)
    {
        if (!_pNode->IsLeaf())
        {
            return;
        }

        if (NarrowCast<Int32>(_pNode->items.size()) <= m_desc.subdivisionThreshold)
        {
            return;
        }

        if (m_desc.bUseMaxDepthPolicy && _pNode->depth >= m_desc.maxDepth)
        {
            return;
        }

        const Vec3 childBaseExt = (_pNode->GetExtents() * 0.5f) / m_desc.looseFactor;
        if (childBaseExt.x < m_desc.minBaseExtents.x || childBaseExt.y < m_desc.minBaseExtents.y || childBaseExt.z < m_desc.minBaseExtents.z)
        {
            return;
        }

        // 현재 노드를 8개의 자식들로 분할
        SplitNode_(_pNode);

        // 기존 노드에 있던 아이템들을 자식 노드들로 재삽입
        Vector<OcItemScoped> existItems = std::move(_pNode->items);
        _pNode->numItemsInTree          = 0;   // Insert_Recursive_ 에서 재계산됨
        for (OcItemScoped& pItem: existItems)
        {
            Insert_Recursive_(_pNode, std::move(pItem));
        }

        // 사후조건: 분할전 Node의 아이템 수 = 분할후 Node들의 아이템 수의 합 + 현재 Node의 아이템 수
    }

    void AddItemToNode_(OcNode* _pNode, OcItemScoped&& _pItem)
    {
        m_itemCache[_pItem->entity.GetUID()] = _pItem.GetPtr();
        _pItem->pOwner                       = _pNode;
        PushItemToList_(_pNode->items, std::move(_pItem));
        SubdivideIfNeed_(_pNode);
    }

    void AddItemToNode_NoSubdivision_(OcNode* _pNode, OcItemScoped&& _pItem)
    {
        m_itemCache[_pItem->entity.GetUID()] = _pItem.GetPtr();
        _pItem->pOwner                       = _pNode;
        PushItemToList_(_pNode->items, std::move(_pItem));
    }

    void PushItemToList_(Vector<OcItemScoped>& _items, OcItemScoped&& _pItem) const
    {
        _pItem->index = NarrowCast<Int32>(_items.size());
        _items.emplace_back(std::move(_pItem));
    }

    OcItemScoped PopItemFromList_(const OcItem* _pItem) const
    {
        Vector<OcItemScoped>& items   = _pItem->pOwner->items;
        const Int32           itemIdx = _pItem->index;
        const Int32           lastIdx = NarrowCast<Int32>(items.size() - 1);
        OcItemScoped          pPopped = std::move(items[itemIdx]);

        // swap and pop (삽입 순서 보장 불필요)
        if (itemIdx != lastIdx)
        {
            items[itemIdx]        = std::move(items[lastIdx]);
            items[itemIdx]->index = itemIdx;
        }
        items.pop_back();

        pPopped->pOwner = nullptr;
        pPopped->index  = -1;
        return pPopped;
    }

    void Insert_Recursive_(OcNode* _pRoot, OcItemScoped&& _pItem)
    {
        JAM_ASSERT(_pRoot->Contains(_pItem->aabb), "Item does not fit in node");   // 해당 함수는 반드시 삽입을 보장해야 함.

        // 삽입 보장. numItemsInTree 즉시 증가.
        ++_pRoot->numItemsInTree;

        if (_pRoot->HasChildren())
        {
            if (OcNode* pChild = FindBestChildNodeOrNull_(_pRoot, _pItem->aabb))
            {
                Insert_Recursive_(pChild, std::move(_pItem));
                return;
            }
        }
        AddItemToNode_(_pRoot, std::move(_pItem));
    }

    void Expand_(const Aabb& _aabb)
    {
        // 옥트리 확장로직.
        // _aabb는 옥트리 외부에 있어야하며, _aabb가 확장의 힌트가 된다.
        // 기존 루트는 _aabb의 반대 방향 슬롯에 배치되고, 새 루트는 기존 루트와 _aabb를 모두 포함하도록 확장된다.
        // 반대 방향에 기존 루트를 배치하는 이유는, _aabb 방향으로 확장하기 위함.

        JAM_ASSERT(!m_pRoot->Contains(_aabb), "Root already contains the target AABB");
        const Vec3 center      = m_pRoot->GetCenter();
        const Vec3 oldBaseExt  = m_pRoot->GetExtents() / m_desc.looseFactor;
        const Vec3 newLooseExt = m_pRoot->GetExtents() * 2.f;
        const Vec3 aabbCenter  = _aabb.GetCenter();

        // _aabb 의 반대 방향 슬롯에 기존 루트를 배치.
        // bFar, bBottom, bRight 는 _aabb 가 새 루트의 어느 슬롯에 위치하는지 계산하기 위함.
        bool bRight = false;
        bool bTop   = false;
        bool bFar   = false;

        Vec3 newCenter = center;

        if (center.x < aabbCenter.x)
        {
            newCenter.x += oldBaseExt.x;
        }
        else
        {
            newCenter.x -= oldBaseExt.x;
            bRight = true;
        }

        if (center.y < aabbCenter.y)
        {
            newCenter.y += oldBaseExt.y;
        }
        else
        {
            newCenter.y -= oldBaseExt.y;
            bTop = true;
        }

        if (center.z < aabbCenter.z)
        {
            newCenter.z += oldBaseExt.z;
        }
        else
        {
            newCenter.z -= oldBaseExt.z;
            bFar = true;
        }

        OcNodeScoped pNewRoot    = MakeNode_(Aabb::FromCenterExtents(newCenter, newLooseExt));
        pNewRoot->depth          = 0;
        pNewRoot->numItemsInTree = m_pRoot->numItemsInTree;

        SplitNode_(pNewRoot.GetPtr());   // 8개 더미 자식 생성

        // 새 루트 슬롯의 자식을 기존 루트로 교체
        OcNode* pOldRoot  = m_pRoot.GetPtr();
        pOldRoot->pParent = pNewRoot.GetPtr();
        pOldRoot->depth   = 1;
        UpdateDepth_Recursive_(pOldRoot);

        // 교체
        const eRegion region       = CalcRegion(bRight, bTop, bFar);
        pNewRoot->children[region] = std::move(m_pRoot);
        m_pRoot                    = std::move(pNewRoot);

        // maxDepth 정책 처리
        if (m_desc.bIncreaseMaxDepthOnExpand)
        {
            ++m_desc.maxDepth;
        }
        else if (m_desc.bUseMaxDepthPolicy)
        {
            MergeTooDeepChildren_Recursive_(m_pRoot.GetPtr());
        }
    }

    [[nodiscard]] OcNodeScoped MakeNode_(const Aabb& _aabb)
    {
        OcNodeScoped pNode = OcNodeScoped { m_nodePool.New(), OcNodeDeletor { &m_nodePool } };
        pNode->aabb        = _aabb;
        return pNode;
    }

    [[nodiscard]] OcItemScoped MakeItem_(const Entity _entity, const Aabb& _aabb, const T& _userdata)
    {
        OcItemScoped pItem = OcItemScoped { m_itemPool.New(), OcItemDeletor { &m_itemPool } };
        pItem->entity      = _entity;
        pItem->aabb        = _aabb;
        pItem->userdata    = _userdata;
        return pItem;
    }

    void UpdateDepth_()
    {
        m_pRoot->depth = 0;
        UpdateDepth_Recursive_(m_pRoot.GetPtr());
    }

    void UpdateDepth_Recursive_(const OcNode* _pNode)
    {
        // 부모 depth 기준으로 자식 depth 결정
        if (_pNode->HasChildren())
        {
            for (const OcNodeScoped& pChild: _pNode->children)
            {
                pChild->depth = _pNode->depth + 1;
                UpdateDepth_Recursive_(pChild.GetPtr());
            }
        }
    }

    void MergeTooDeepChildren_Recursive_(OcNode* _pNode)
    {
        if (!_pNode->HasChildren())
        {
            return;
        }

        // 바텀-업 자식부터 먼저 처리
        for (const OcNodeScoped& pChild: _pNode->children)
        {
            MergeTooDeepChildren_Recursive_(pChild.GetPtr());
        }

        if (_pNode->depth >= m_desc.maxDepth)
        {
            for (const OcNodeScoped& pChild: _pNode->children)
            {
                Vector<OcItemScoped> childItems = std::move(pChild->items);
                for (OcItemScoped& pItem: childItems)
                {
                    AddItemToNode_NoSubdivision_(_pNode, std::move(pItem));
                }
            }
            _pNode->ClearChildren();
        }
    }

    void MergeEmptyChildren_Recursive_(OcNode* _pNode)
    {
        if (!_pNode->HasChildren())
        {
            return;
        }

        // 자식부터 먼저 처리
        for (const OcNodeScoped& pChild: _pNode->children)
        {
            MergeEmptyChildren_Recursive_(pChild.GetPtr());
        }

        const Int32 numItemsInChildren = _pNode->numItemsInTree - NarrowCast<Int32>(_pNode->items.size());
        if (numItemsInChildren == 0)
        {
            _pNode->ClearChildren();
        }
    }

    [[nodiscard]] OcNode* FindBestChildNodeOrNull_(   // or nullptr
        const OcNode* _pRoot,
        const Aabb&   _aabb) const
    {
        JAM_ASSERT(_pRoot->HasChildren(), "Node has no children");
        JAM_ASSERT(_pRoot->Contains(_aabb), "Aabb does not fit in node");

        const Vec3    center     = _pRoot->GetCenter();
        const Vec3    aabbCenter = _aabb.GetCenter();
        const eRegion region     = CalcRegion(aabbCenter.x > center.x, aabbCenter.y > center.y, aabbCenter.z > center.z);
        OcNode*       pNode      = _pRoot->children[region].GetPtr();
        return pNode->Contains(_aabb) ? pNode : nullptr;
    }

    ObjectPool<OcItem>    m_itemPool;
    ObjectPool<OcNode>    m_nodePool;
    OctreeDesc            m_desc      = {};
    OcNodeScoped          m_pRoot     = nullptr;
    HashMap<UID, OcItem*> m_itemCache = {};
};

}   // namespace Jam