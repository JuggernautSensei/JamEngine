#pragma once

struct OctreeDesc
{
    Vec3 minBaseExtents     = Vec3(4.f);
    Vec3 initialBaseExtents = Vec3(1024.f);

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
    using OcItemList = std::vector<OcItemPtr>;

    struct OcItem
    {
        [[nodiscard]] Vec3 GetCenter() const { return aabb.Center(); }
        [[nodiscard]] Vec3 GetExtents() const { return aabb.Extents(); }

        Entity  entity   = Entity::s_null;   // owner entity
        Aabb    aabb     = {};               // bounding box
        OcNode* pOwner   = nullptr;          // owner node
        Int32   index    = -1;               // index in owner's item list
        T       userdata = {};               // user data
    };

    enum Region
    {
        NEAR_LEFT_TOP,
        NEAR_RIGHT_TOP,
        NEAR_LEFT_BOTTOM,
        NEAR_RIGHT_BOTTOM,
        FAR_LEFT_TOP,
        FAR_RIGHT_TOP,
        FAR_LEFT_BOTTOM,
        FAR_RIGHT_BOTTOM,

        REGION_COUNT   = 8,
        UNKNOWN_REGION = -1
    };

    inline static Region k_regionRanges[] = {
        NEAR_LEFT_TOP,
        NEAR_RIGHT_TOP,
        NEAR_LEFT_BOTTOM,
        NEAR_RIGHT_BOTTOM,
        FAR_LEFT_TOP,
        FAR_RIGHT_TOP,
        FAR_LEFT_BOTTOM,
        FAR_RIGHT_BOTTOM
    };

    struct OcNode
    {
        [[nodiscard]] bool IsLeaf() const { return children[0] == nullptr; }
        [[nodiscard]] bool HasChildren() const { return children[0] != nullptr; }
        [[nodiscard]] bool HasGrandChildren() const { return HasChildren() && children[0]->HasChildren(); }
        [[nodiscard]] bool HasParent() const { return pParent != nullptr; }
        [[nodiscard]] bool Contain(const Aabb& _aabb) const { return aabb.Contains(_aabb); }
        [[nodiscard]] bool Contain(const OcItem* _pItem) const { return aabb.Contains(_pItem->aabb); }
        [[nodiscard]] Vec3 GetCenter() const { return aabb.Center(); }
        [[nodiscard]] Vec3 GetExtents() const { return aabb.Extents(); }

        OcItemList items                  = {};
        OcNodePtr  children[REGION_COUNT] = { nullptr };
        OcNode*    pParent                = nullptr;
        Aabb       aabb                   = {};
        Int32      depth                  = -1;
        Int32      numItemsInTree         = 0;   // total number of items in this node and all children
    };

public:
    explicit Octree(
        const OctreeDesc& _desc = {})
        : m_desc(_desc)
        , m_itemPool(2048)
        , m_nodePool(512)
    {
        JAM_ASSERT(_desc.looseFactor >= 1.f, "looseFactor must be >= 1.f");
        JAM_ASSERT(_desc.maxDepth > 0, "maxDepth must be > 0");
        JAM_ASSERT(_desc.subdivisionThreshold > 0, "subdivisionThreshold must be > 0");

        JAM_ASSERT(_desc.minBaseExtents.x > 1.f
                       && _desc.minBaseExtents.y > 1.f
                       && _desc.minBaseExtents.z > 1.f,
                   "minBaseExtents must be > 1.f");

        JAM_ASSERT(_desc.initialBaseExtents.x >= _desc.minBaseExtents.x
                       && _desc.initialBaseExtents.y >= _desc.minBaseExtents.y
                       && _desc.initialBaseExtents.z >= _desc.minBaseExtents.z,
                   "initialBaseExtents must be >= minBaseExtents");

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

        // expand octree until root contains the item
        while (!m_pRoot->Contain(_aabb))
        {
            ExpandOctree_();
        }

        // item is ensured to fit in the octree
        OcItemPtr pItem = MakeItem_(_entity, _aabb, _userdata);
        Insert_Recursive_(m_pRoot.get(), std::move(pItem));
    }

    void Update(
        const Entity _entity,
        const Aabb&  _aabb)
    {
        JAM_ASSERT(_entity, "Invalid entity");

        auto it = m_itemCache.find(_entity.GetUID());
        JAM_ASSERT(it != m_itemCache.end(), "Entity does not exist");

        // update aabb
        OcItem* pItem = it->second;
        pItem->aabb   = _aabb;

        // find the nearest parent node in the hierarchy that contains the item
        OcNode* pOwnerNode = pItem->pOwner;
        OcNode* pDstRoot   = pOwnerNode;
        while (pDstRoot && !pDstRoot->Contain(pItem))
        {
            --pDstRoot->numItemsInTree;
            pDstRoot = pDstRoot->pParent;
        }

        if (pDstRoot == pOwnerNode)   // item stays in same subtree
        {
            if (pOwnerNode->HasChildren())
            {
                if (OcNode* pChildRoot = FindBestChildNodeOrNull_(pOwnerNode, _aabb))   // can fit into child node
                {
                    OcItemPtr pPopedItem = PopItemFromList_(pOwnerNode->items, pItem);
                    Insert_Recursive_(pChildRoot, std::move(pPopedItem));
                }
            }
            // else: stay in the same node (no children or doesn't fit in any child)
        }
        else
        {
            // item moved to different subtree or outside octree
            if (pDstRoot == nullptr)
            {
                // expand octree until root contains the item
                while (!m_pRoot->Contain(_aabb))
                {
                    ExpandOctree_();
                }
                pDstRoot = m_pRoot.get();
            }

            OcItemPtr pPopedItem = PopItemFromList_(pOwnerNode->items, pItem);
            Insert_Recursive_(pDstRoot, std::move(pPopedItem));
        }
    }

    void Remove(
        const Entity _entity)
    {
        JAM_ASSERT(_entity, "Invalid entity");

        auto it = m_itemCache.find(_entity.GetUID());
        JAM_ASSERT(it != m_itemCache.end(), "Entity does not exist");

        OcItem* pItem = it->second;

        // update numItemsInTree in all parent nodes
        OcNode* pCurNode = pItem->pOwner;
        while (pCurNode)
        {
            --pCurNode->numItemsInTree;
            pCurNode = pCurNode->pParent;
        }

        // remove from owner node and cache
        OcNode* pOwner = pItem->pOwner;
        PopItemFromList_(pOwner->items, pItem);
        m_itemCache.erase(it);
    }

    void ShrinkToFit()
    {
        // shrink root if possible
        if (m_pRoot->items.empty())
        {
            OcNode* pCurNode  = m_pRoot.get();
            Region  candidate = UNKNOWN_REGION;

            while (pCurNode->HasChildren())
            {
                // octree cannot be smaller than initial size
                const Vec3 baseExt = pCurNode->GetExtents() / m_desc.looseFactor;
                if (baseExt.x <= m_desc.initialBaseExtents.x
                    && baseExt.y <= m_desc.initialBaseExtents.y
                    && baseExt.z <= m_desc.initialBaseExtents.z)
                {
                    break;
                }

                // find child that contains all items
                Region nextCandidate = UNKNOWN_REGION;
                for (Region region: k_regionRanges)
                {
                    OcNode* pChild = pCurNode->children[region].get();
                    if (pChild->numItemsInTree == m_pRoot->numItemsInTree)
                    {
                        pCurNode      = pChild;
                        nextCandidate = region;
                        break;
                    }
                    else if (pChild->numItemsInTree != 0)
                    {
                        // multiple children contain items, cannot shrink
                        break;
                    }
                }

                if (nextCandidate == UNKNOWN_REGION)
                {
                    break;
                }

                candidate = nextCandidate;
            }

            if (candidate != UNKNOWN_REGION)
            {
                OcNodePtr pNewRoot = std::move(pCurNode->pParent->children[candidate]);
                pNewRoot->pParent  = nullptr;
                m_pRoot            = std::move(pNewRoot);
            }
        }

        // shrink empty children
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
        if (_pNode->numItemsInTree == 0)
        {
            return;
        }

        if (_intersactFunc(_bv, _pNode->aabb))
        {
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
                for (Region region: k_regionRanges)
                {
                    Query_Recursive_(_pNode->children[region].get(), _bv, _intersactFunc, _collectFunc);
                }
            }
        }
    }

    void SplitNode_(
        OcNode* _pNode)
    {
        JAM_ASSERT(_pNode->IsLeaf(), "Node already has children");

        const Vec3 center           = _pNode->GetCenter();
        const Vec3 childExt         = _pNode->GetExtents() * 0.5f;
        const Vec3 halfChildBaseExt = childExt / (2.f * m_desc.looseFactor);

        for (Region region: k_regionRanges)
        {
            Vec3 regionCenter = center;
            regionCenter.x += (region & 0b0001) ? halfChildBaseExt.x : -halfChildBaseExt.x;
            regionCenter.y += (region & 0b0010) ? -halfChildBaseExt.y : halfChildBaseExt.y;
            regionCenter.z += (region & 0b0100) ? halfChildBaseExt.z : -halfChildBaseExt.z;

            const Aabb aabb = Aabb::FromCenterExtents(regionCenter, childExt);

            OcNodePtr pChildNode     = MakeNode_(aabb);
            pChildNode->pParent      = _pNode;
            pChildNode->depth        = _pNode->depth + 1;
            _pNode->children[region] = std::move(pChildNode);
        }
    }

    void TrySubdivide_(
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
        m_itemCache[_pItem->entity.GetUID()] = _pItem.get();
        _pItem->pOwner                       = _pNode;

        PushItemToList_(_pNode->items, std::move(_pItem));
        TrySubdivide_(_pNode);
    }

    void PushItemToList_(
        OcItemList& _items,
        OcItemPtr&& _pItem) const
    {
        _pItem->index = NarrowCast<Int32>(_items.size());
        _items.emplace_back(std::move(_pItem));
    }

    OcItemPtr PopItemFromList_(
        OcItemList&   _items,
        const OcItem* _pItem) const
    {
        JAM_ASSERT(_pItem->index >= 0 && _pItem->index < NarrowCast<Int32>(_items.size()), "Invalid item index");

        const Int32 itemIdx    = _pItem->index;
        const Int32 lastIdx    = NarrowCast<Int32>(_items.size()) - 1;
        OcItemPtr   pExtracted = std::move(_items[itemIdx]);

        if (itemIdx != lastIdx)
        {
            // swap with last and update index
            _items[itemIdx]        = std::move(_items[lastIdx]);
            _items[itemIdx]->index = itemIdx;
        }

        _items.pop_back();

        pExtracted->pOwner = nullptr;
        pExtracted->index  = -1;

        return pExtracted;
    }

    void Insert_Recursive_(
        OcNode*     _pRoot,
        OcItemPtr&& _pItem)
    {
        JAM_ASSERT(_pRoot->Contain(_pItem->aabb), "Item does not fit in node");

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

        if (m_pRoot->HasChildren())
        {
            // old root has children: create double-split structure
            SplitNode_(pNewRoot.get());
            for (Region region: k_regionRanges)
            {
                SplitNode_(pNewRoot->children[region].get());
            }

            // mapping: where each old child goes in new grandchildren
            constexpr Region k_grandchildMapping[REGION_COUNT] = {
                FAR_RIGHT_BOTTOM,    // NEAR_LEFT_TOP
                FAR_LEFT_BOTTOM,     // NEAR_RIGHT_TOP
                FAR_RIGHT_TOP,       // NEAR_LEFT_BOTTOM
                FAR_LEFT_TOP,        // NEAR_RIGHT_BOTTOM
                NEAR_RIGHT_BOTTOM,   // FAR_LEFT_TOP
                NEAR_LEFT_BOTTOM,    // FAR_RIGHT_TOP
                NEAR_RIGHT_TOP,      // FAR_LEFT_BOTTOM
                NEAR_LEFT_TOP        // FAR_RIGHT_BOTTOM
            };

            // move old children to new grandchildren positions
            Int32 numOldRootChildrenItems = 0;
            for (Region region: k_regionRanges)
            {
                const Region grandchildRegion   = k_grandchildMapping[region];
                OcNode*      pNewChild          = pNewRoot->children[region].get();
                OcNodePtr&   pOldChild          = m_pRoot->children[region];
                const Int32  numItemsInOldChild = pOldChild->numItemsInTree;

                // transfer ownership
                pOldChild->pParent                    = pNewChild;
                pOldChild->depth                      = pNewChild->depth + 1;
                pNewChild->children[grandchildRegion] = std::move(pOldChild);
                numOldRootChildrenItems += numItemsInOldChild;
            }
            pNewRoot->numItemsInTree = numOldRootChildrenItems;   // old root's numItemsInTree will be recalculated below

            // re-insert old root's items
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
            // old root is leaf: simply move items
            OcItemList oldRootItems = std::move(m_pRoot->items);
            for (OcItemPtr& pItem: oldRootItems)
            {
                Insert_Recursive_(pNewRoot.get(), std::move(pItem));
            }
        }

        m_pRoot = std::move(pNewRoot);
    }

    [[nodiscard]] OcNodePtr NewNode_()
    {
        OcNodeDeletor deletor;
        deletor.pPool = &m_nodePool;
        return OcNodePtr(m_nodePool.New(), deletor);
    }

    [[nodiscard]] OcItemPtr NewItem_()
    {
        OcItemDeletor deletor;
        deletor.pPool = &m_itemPool;
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
        if (_pRoot->HasChildren())
        {
            for (Region region: k_regionRanges)
            {
                OcNode* pChild = _pRoot->children[region].get();
                pChild->depth  = _pRoot->depth + 1;
                UpdateDepth_Recursive_(pChild);
            }
        }
    }

    void MergeTooDeepChildren_Recursive_(
        OcNode* _pRoot)
    {
        if (_pRoot->HasChildren())
        {
            // bottom-up: merge children first
            for (Region region: k_regionRanges)
            {
                MergeTooDeepChildren_Recursive_(_pRoot->children[region].get());
            }

            // then merge this node's children if too deep
            if (_pRoot->depth >= m_desc.maxDepth)
            {
                for (Region region: k_regionRanges)
                {
                    OcNode*    pChild     = _pRoot->children[region].get();
                    OcItemList childItems = std::move(pChild->items);

                    for (OcItemPtr& pItem: childItems)
                    {
                        // numItemsInTree already correct, just move items
                        AddItemToNode_(_pRoot, std::move(pItem));
                    }

                    _pRoot->children[region] = nullptr;
                }
            }
        }
    }

    void ShrinkToFitMergeEmptyChildren_Recursive_(
        OcNode* _pNode)
    {
        if (_pNode->HasChildren())
        {
            // bottom-up traversal
            for (Region region: k_regionRanges)
            {
                ShrinkToFitMergeEmptyChildren_Recursive_(_pNode->children[region].get());
            }

            // check if all children are empty
            const Int32 numItemsInCurNode  = NarrowCast<Int32>(_pNode->items.size());
            const Int32 numItemsInChildren = _pNode->numItemsInTree - numItemsInCurNode;

            if (numItemsInChildren == 0)
            {
                // all children empty, remove them
                for (Region region: k_regionRanges)
                {
                    _pNode->children[region] = nullptr;
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
        const Vec3 center     = _pRoot->GetCenter();
        const Vec3 aabbCenter = _aabb.Center();

        Int32 quadrant = 0;
        if (aabbCenter.x > center.x) quadrant |= 0b0001;   // Right
        if (aabbCenter.y < center.y) quadrant |= 0b0010;   // Bottom
        if (aabbCenter.z > center.z) quadrant |= 0b0100;   // Far

        OcNode* pCandidate = _pRoot->children[quadrant].get();
        return pCandidate->Contain(_aabb) ? pCandidate : nullptr;
    }

    OctreeDesc         m_desc = {};
    ObjectPool<OcItem> m_itemPool;
    ObjectPool<OcNode> m_nodePool;

    OcNodePtr             m_pRoot     = nullptr;
    HashMap<UID, OcItem*> m_itemCache = {};
};
