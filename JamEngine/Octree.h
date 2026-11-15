#pragma once
#include "Entity.h"
#include "MemoryPool.h"
#include "BoundingVolume.h"

namespace Jam
{

template<typename TBoundingVolume, typename TIntersectFunc>
concept IsOctreeIntersectFunc = requires(TBoundingVolume _bv, const Aabb& _aabb, TIntersectFunc&& _func) { { _func(_bv, _aabb) } -> std::convertible_to<bool>; };

template<typename TBoundingVolume, typename TClassifyFunc>
concept IsOctreeClassifyFunc = requires(const TBoundingVolume& _bv, const Aabb& _aabb, TClassifyFunc&& _func) { { _func(_bv, _aabb) } -> std::convertible_to<eContainment>; };

template<typename TUserData, typename TCollectFunc>
concept IsOctreeCollectFunc = requires(Entity _entity, const TUserData& _userdata, TCollectFunc&& _func) { { _func(_entity, _userdata) }; };

template<typename TUserData, typename TTraverseFunc>
concept IsOctreeTraverseFunc = requires(Entity _entity, const Aabb& _aabb, const TUserData& _userdata, TTraverseFunc&& _func) { { _func(_entity, _aabb, _userdata) }; };

struct OctreeInfo
{
    Vec3  minBaseExtents            = Vec3 { 4.f };      // 가장 작은 베이스 aabb extents. 이것보다 작게 분할하지 않음.
    Vec3  initBaseExtents           = Vec3 { 1024.f };   // 초기 베이스 aabb extents.
    Int32 maxDepth                  = 8;                 // 최대 깊이. 루트 노드의 깊이는 0. 이 값에 도달하면 더 이상 분할하지 않음.
    bool  bUseMaxDepthPolicy        = true;              // true. 분할 시 maxDepth 강제, false. minBaseExtents 도달 시까지 분할
    bool  bIncreaseMaxDepthOnExpand = true;              // true. 확장 시 maxDepth 증가, false. maxDepth 유지, 확장 후 깊이 초과 노드 병합
    Int32 subdivisionThreshold      = 16;                // 노드당 아이템 수가 이 값을 초과하면 분할
    float looseFactor               = 2.f;               // loose factor (>= 1.f). 1.f = tight octree
};

// Insert / Build 반환 핸들. Remove 이후 무효, 이후 Insert에서 재사용될 수 있음
using OctreeItemHandle = Int32;

constexpr OctreeItemHandle kInvalidOctreeItemHandle = -1;

template<typename T>
class Octree
{
public:
    struct BuildItem
    {
        Entity entity   = Entity::kNull;
        Aabb   aabb     = {};
        T      userdata = {};
    };

private:
    static constexpr Int32 kQueryStackSize = 512;

    struct OcNode;

    struct OcItem
    {
        Entity           entity   = Entity::kNull;
        T                userdata = {};
        OctreeItemHandle handle   = kInvalidOctreeItemHandle;
    };

    struct OcSlot
    {
        OcNode* pNode = nullptr;
        Int32   index = -1;
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

        [[nodiscard]] Vec3 GetCenter() const
        {
            return aabb.center;
        }

        [[nodiscard]] Vec3 GetExtents() const
        {
            return aabb.extents;
        }

        [[nodiscard]] bool Contains(const Aabb& _aabb) const
        {
            return aabb.Contains(_aabb);
        }

        Array<OcNode*, 8> children       = {};
        OcNode*           pParent        = nullptr;
        Aabb              aabb           = {};
        Int32             depth          = -1;
        Int32             numItemsInTree = 0;   // 이 노드 + 모든 자손의 아이템 수
        Vector<Aabb>      aabbs;                // items와 같은 인덱스. 쿼리용 연속 메모리
        Vector<OcItem>    items;
    };

public:
    explicit Octree(const OctreeInfo& _info = {})
        : m_nodePool(512)
        , m_info(_info)
        , m_initInfo(_info)
    {
        JAM_ASSERT(_info.looseFactor >= 1.f, "looseFactor must be >= 1.f");
        JAM_ASSERT(_info.maxDepth >= 2, "maxDepth must be >= 2");
        JAM_ASSERT(_info.subdivisionThreshold > 0, "subdivisionThreshold must be > 0");
        JAM_ASSERT(_info.minBaseExtents.x > 1.f && _info.minBaseExtents.y > 1.f && _info.minBaseExtents.z > 1.f, "minBaseExtents must be > 1.f");
        JAM_ASSERT(_info.initBaseExtents.x >= _info.minBaseExtents.x && _info.initBaseExtents.y >= _info.minBaseExtents.y && _info.initBaseExtents.z >= _info.minBaseExtents.z, "initBaseExtents must be >= minBaseExtents");

        CreateRoot_();
    }

    ~Octree()
    {
        DestroySubtree_(m_pRoot);
    }

    Octree(const Octree&)                = delete;
    Octree& operator=(const Octree&)     = delete;
    Octree(Octree&&) noexcept            = delete;
    Octree& operator=(Octree&&) noexcept = delete;

    OctreeItemHandle Insert(
        const Entity _entity,
        const Aabb&  _aabb,
        const T&     _userdata)
    {
        JAM_ASSERT(_entity, "Invalid entity");
        EnsureEntitySlots_();
        JAM_ASSERT(!m_entitySlots.contains(_entity.GetUID()), "Entity already exists");

        while (!m_pRoot->Contains(_aabb))   // 필요시 확장
        {
            Expand_(_aabb);
        }

        const OctreeItemHandle handle   = AllocSlot_();
        m_entitySlots[_entity.GetUID()] = handle;
        InsertItem_(m_pRoot, OcItem { _entity, _userdata, handle }, _aabb);
        return handle;
    }

    // 아이템 배열로 트리를 새로 구성. 기존 내용은 버림
    // 개별 Insert보다 빠름. 핸들은 입력 인덱스
    // Entity API는 처음 호출 시 Entity 맵 생성. 핸들만 쓰면 비용 없음
    void Build(
        const Span<const BuildItem> _items,
        Vector<OctreeItemHandle>*   _pOutHandlesOrNull = nullptr)
    {
        Build_(_items.data(), static_cast<Int32>(_items.size()), _pOutHandlesOrNull);
    }

    void Build(
        const Vector<BuildItem>&  _items,
        Vector<OctreeItemHandle>* _pOutHandlesOrNull = nullptr)
    {
        Build_(_items.data(), static_cast<Int32>(_items.size()), _pOutHandlesOrNull);
    }

    void Clear()
    {
        DestroySubtree_(m_pRoot);
        m_pRoot = nullptr;
        m_slots.clear();
        m_freeSlots.clear();
        m_entitySlots.clear();
        m_bEntitySlotsValid = true;
        m_info              = m_initInfo;
        CreateRoot_();
    }

    void Update(
        const OctreeItemHandle _handle,
        const Aabb&            _aabb)
    {
        JAM_ASSERT(IsValidHandle_(_handle), "Invalid item handle");

        const OcSlot slot         = m_slots[_handle];
        OcNode*      pOwner       = slot.pNode;
        pOwner->aabbs[slot.index] = _aabb;

        // 자신을 포함하는 가장 작은 조상 노드를 바텀-업으로 탐색.
        // 벗어난 노드들의 numItemsInTree 를 감소시키면서 올라감.
        OcNode* pNode = pOwner;
        while (pNode && !pNode->Contains(_aabb))
        {
            --pNode->numItemsInTree;
            pNode = pNode->pParent;
        }

        // 시나리오
        // 1. 같은 노드에 머무르는 경우 (대부분)
        // 2. 옥트리 밖으로 벗어나는 경우
        // 3. 옥트리 내 다른 서브트리로 이동하는 경우

        if (pNode == pOwner)
        {
            if (!pOwner->HasChildren())
            {
                return;
            }

            OcNode* pChild = FindBestChild_(pOwner, _aabb);
            if (!pChild)
            {
                return;   // 적합한 자식 없으면 현재 노드에 그대로 유지
            }

            OcItem item = PopItem_(_handle);
            InsertItem_(pChild, std::move(item), _aabb);
            return;
        }

        OcItem item = PopItem_(_handle);
        if (pNode == nullptr)
        {
            // 옥트리 밖으로 벗어나는 경우. 루트부터 다시 삽입.
            // 확장 코드가 꼬일 수 있기 때문에 item은 제거 후 다시 삽입
            while (!m_pRoot->Contains(_aabb))
            {
                Expand_(_aabb);
            }
            InsertItem_(m_pRoot, std::move(item), _aabb);
        }
        else
        {
            // 옥트리 내 다른 서브트리로 이동.
            // InsertItem_ 이 pNode 부터 시작해 다시 증가시키므로 미리 감소.
            --pNode->numItemsInTree;
            InsertItem_(pNode, std::move(item), _aabb);
        }
    }

    void Update(
        const Entity _entity,
        const Aabb&  _aabb)
    {
        Update(FindHandle_(_entity), _aabb);
    }

    void Remove(
        const OctreeItemHandle _handle)
    {
        JAM_ASSERT(IsValidHandle_(_handle), "Invalid item handle");

        // 자신을 포함한 모든 조상의 numItemsInTree 를 감소
        for (OcNode* pNode = m_slots[_handle].pNode; pNode; pNode = pNode->pParent)
        {
            --pNode->numItemsInTree;
        }

        const OcItem item = PopItem_(_handle);
        if (m_bEntitySlotsValid)
        {
            m_entitySlots.erase(item.entity.GetUID());
        }
        m_freeSlots.push_back(_handle);
    }

    void Remove(
        const Entity _entity)
    {
        Remove(FindHandle_(_entity));
    }

    [[nodiscard]] OctreeItemHandle GetHandle(
        const Entity _entity) const
    {
        EnsureEntitySlots_();
        const auto it = m_entitySlots.find(_entity.GetUID());
        return it != m_entitySlots.end() ? it->second : kInvalidOctreeItemHandle;
    }

    void ShrinkToFit()
    {
        // 모든 아이템을 포함하는 서브트리를 탐색하고 새 루트로 승격
        // 옥트리는 initBaseExtents 보다 작아질 수 없음
        const Vec3 minLooseExt = m_info.initBaseExtents * m_info.looseFactor;
        if (m_pRoot->items.empty() && m_pRoot->numItemsInTree > 0)
        {
            OcNode* pRootNode   = m_pRoot;
            OcNode* pTargetNode = pRootNode;
            Int32   targetIndex = 0;

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
                for (Int32 i = 0; i < 8; ++i)
                {
                    OcNode* pChild = pTargetNode->children[i];
                    if (pChild->numItemsInTree == m_pRoot->numItemsInTree)
                    {
                        pTargetNode = pChild;
                        targetIndex = i;
                        bExit       = false;
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
                pTargetNode->pParent->children[targetIndex] = nullptr;
                pTargetNode->pParent                        = nullptr;
                DestroySubtree_(pRootNode);   // 대상 서브트리는 이미 분리됨. 나머지는 빈 노드
                m_pRoot = pTargetNode;
                UpdateDepth_();   // 깊이 다시 계산
            }
        }

        // 아이템이 없는 빈 노드 병합
        MergeEmptyChildren_Recursive_(m_pRoot);
    }

    // 어떤 _bv와 충돌하는 모든 Entity 탐색
    template<typename TBoundingVolume, typename IntersectFunc, typename CollectFunc>
        requires(IsOctreeIntersectFunc<TBoundingVolume, IntersectFunc> && IsOctreeCollectFunc<T, CollectFunc>)
    void Query(const TBoundingVolume& _bv, IntersectFunc&& _intersectFunc, CollectFunc&& _collectFunc) const
    {
        QueryClassified(
            _bv,
            [&](const TBoundingVolume& _volume, const Aabb& _aabb) { return _intersectFunc(_volume, _aabb) ? eContainment::Intersects : eContainment::Outside; },
            _collectFunc);
    }

    // Inside 판정 노드는 하위 아이템을 판정 없이 모두 수집
    template<typename TBoundingVolume, typename ClassifyFunc, typename CollectFunc>
        requires(IsOctreeClassifyFunc<TBoundingVolume, ClassifyFunc> && IsOctreeCollectFunc<T, CollectFunc>)
    void QueryClassified(const TBoundingVolume& _bv, ClassifyFunc&& _classifyFunc, CollectFunc&& _collectFunc) const
    {
        Array<const OcNode*, kQueryStackSize> stack;
        Int32                                 top = 0;
        stack[top++]                              = m_pRoot;

        while (top > 0)
        {
            const OcNode* pNode = stack[--top];
            if (pNode->numItemsInTree == 0)
            {
                continue;
            }

            const eContainment containment = _classifyFunc(_bv, pNode->aabb);
            if (containment == eContainment::Outside)
            {
                continue;
            }

            if (containment == eContainment::Inside)
            {
                CollectSubtree_(pNode, _collectFunc);
                continue;
            }

            const Int32 numItems = static_cast<Int32>(pNode->items.size());
            for (Int32 i = 0; i < numItems; ++i)
            {
                if (_classifyFunc(_bv, pNode->aabbs[i]) != eContainment::Outside)
                {
                    _collectFunc(pNode->items[i].entity, pNode->items[i].userdata);
                }
            }

            if (pNode->HasChildren())
            {
                JAM_ASSERT(top + 8 <= kQueryStackSize, "Octree query stack overflow");
                for (OcNode* pChild: pNode->children)
                {
                    stack[top++] = pChild;
                }
            }
        }
    }

    template<typename TraverseFunc>
        requires(IsOctreeTraverseFunc<T, TraverseFunc>)
    void TopDownTraverse(TraverseFunc&& _func) const
    {
        TopDownTraverse_Recursive_(m_pRoot, _func);
    }

    template<typename TraverseFunc>
        requires(IsOctreeTraverseFunc<T, TraverseFunc>)
    void BottomUpTraverse(TraverseFunc&& _func) const
    {
        BottomUpTraverse_Recursive_(m_pRoot, _func);
    }

    // _func(const Aabb& _looseAabb, Int32 _depth, Int32 _numItemsInTree, bool _bLeaf)
    template<typename TraverseFunc>
        requires std::invocable<TraverseFunc&, const Aabb&, Int32, Int32, bool>
    void TraverseNodes(TraverseFunc&& _func) const
    {
        TraverseNodes_Recursive_(m_pRoot, _func);
    }

    [[nodiscard]] SizeT GetNumItems() const
    {
        return m_pRoot->numItemsInTree;
    }

private:
    void Build_(
        const BuildItem*          _pItems,
        const Int32               _numItems,
        Vector<OctreeItemHandle>* _pOutHandlesOrNull)
    {
        Clear();
        m_bEntitySlotsValid = false;

        m_slots.resize(_numItems);

        if (_pOutHandlesOrNull)
        {
            _pOutHandlesOrNull->resize(_numItems);
        }

        m_buildOrder.clear();
        m_buildOrder.reserve(_numItems);
        Vector<Int32> outsideRoot;
        for (Int32 i = 0; i < _numItems; ++i)
        {
            JAM_ASSERT(_pItems[i].entity, "Invalid entity");

            if (_pOutHandlesOrNull)
            {
                (*_pOutHandlesOrNull)[i] = i;
            }

            if (m_pRoot->Contains(_pItems[i].aabb))
            {
                m_buildOrder.push_back(i);
            }
            else
            {
                outsideRoot.push_back(i);
            }
        }

        m_buildScratch.resize(m_buildOrder.size());
        m_buildBucket.resize(m_buildOrder.size());
        BuildNode_(m_pRoot, _pItems, 0, static_cast<Int32>(m_buildOrder.size()));

        // 루트 밖 아이템은 일반 삽입으로 확장
        for (const Int32 index: outsideRoot)
        {
            while (!m_pRoot->Contains(_pItems[index].aabb))
            {
                Expand_(_pItems[index].aabb);
            }
            InsertItem_(m_pRoot, OcItem { _pItems[index].entity, _pItems[index].userdata, index }, _pItems[index].aabb);
        }

        m_buildOrder.clear();
        m_buildScratch.clear();
        m_buildBucket.clear();
    }

    [[nodiscard]] bool IsValidHandle_(const OctreeItemHandle _handle) const
    {
        return 0 <= _handle && _handle < static_cast<Int32>(m_slots.size()) && m_slots[_handle].pNode != nullptr;
    }

    [[nodiscard]] OctreeItemHandle FindHandle_(const Entity _entity) const
    {
        JAM_ASSERT(_entity, "Invalid entity");

        EnsureEntitySlots_();
        const auto it = m_entitySlots.find(_entity.GetUID());
        JAM_ASSERT(it != m_entitySlots.end(), "Entity does not exist");
        return it->second;
    }

    // Build 이후 첫 Entity API 호출 시 Entity 맵 생성
    void EnsureEntitySlots_() const
    {
        if (m_bEntitySlotsValid)
        {
            return;
        }

        m_entitySlots.clear();
        m_entitySlots.reserve(m_pRoot->numItemsInTree);

        Array<const OcNode*, kQueryStackSize> stack;
        Int32                                 top = 0;
        stack[top++]                              = m_pRoot;
        while (top > 0)
        {
            const OcNode* pNode = stack[--top];
            if (pNode->numItemsInTree == 0)
            {
                continue;
            }

            for (const OcItem& item: pNode->items)
            {
                const bool bInserted = m_entitySlots.emplace(item.entity.GetUID(), item.handle).second;
                JAM_ASSERT(bInserted, "Entity already exists");
            }

            if (pNode->HasChildren())
            {
                for (OcNode* pChild: pNode->children)
                {
                    stack[top++] = pChild;
                }
            }
        }

        m_bEntitySlotsValid = true;
    }

    [[nodiscard]] OctreeItemHandle AllocSlot_()
    {
        if (!m_freeSlots.empty())
        {
            const OctreeItemHandle handle = m_freeSlots.back();
            m_freeSlots.pop_back();
            return handle;
        }

        m_slots.emplace_back();
        return static_cast<OctreeItemHandle>(m_slots.size() - 1);
    }

    void CreateRoot_()
    {
        const Vec3 ext = m_info.initBaseExtents * m_info.looseFactor;
        m_pRoot        = MakeNode_(Aabb { Vec3::kZero, ext });
        m_pRoot->depth = 0;
    }

    [[nodiscard]] OcNode* MakeNode_(const Aabb& _aabb)
    {
        OcNode* pNode = m_nodePool.New();
        pNode->aabb   = _aabb;
        return pNode;
    }

    void DestroySubtree_(OcNode* _pNode)
    {
        if (!_pNode)
        {
            return;
        }

        for (OcNode*& pChild: _pNode->children)
        {
            DestroySubtree_(pChild);
            pChild = nullptr;
        }
        m_nodePool.Delete(_pNode);
    }

    void DestroyChildren_(OcNode* _pNode)
    {
        for (OcNode*& pChild: _pNode->children)
        {
            DestroySubtree_(pChild);
            pChild = nullptr;
        }
    }

    template<typename CollectFunc>
    void CollectSubtree_(const OcNode* _pRoot, CollectFunc& _collectFunc) const
    {
        Array<const OcNode*, kQueryStackSize> stack;
        Int32                                 top = 0;
        stack[top++]                              = _pRoot;

        while (top > 0)
        {
            const OcNode* pNode = stack[--top];
            if (pNode->numItemsInTree == 0)
            {
                continue;
            }

            for (const OcItem& item: pNode->items)
            {
                _collectFunc(item.entity, item.userdata);
            }

            if (pNode->HasChildren())
            {
                JAM_ASSERT(top + 8 <= kQueryStackSize, "Octree query stack overflow");
                for (OcNode* pChild: pNode->children)
                {
                    stack[top++] = pChild;
                }
            }
        }
    }

    template<typename TraverseFunc>
    void TopDownTraverse_Recursive_(const OcNode* _pNode, TraverseFunc& _func) const
    {
        if (_pNode->numItemsInTree == 0)
        {
            return;
        }

        const Int32 numItems = static_cast<Int32>(_pNode->items.size());
        for (Int32 i = 0; i < numItems; ++i)
        {
            _func(_pNode->items[i].entity, _pNode->aabbs[i], _pNode->items[i].userdata);
        }

        if (_pNode->HasChildren())
        {
            for (const OcNode* pChild: _pNode->children)
            {
                TopDownTraverse_Recursive_(pChild, _func);
            }
        }
    }

    template<typename TraverseFunc>
    void TraverseNodes_Recursive_(const OcNode* _pNode, TraverseFunc& _func) const
    {
        if (_pNode->numItemsInTree == 0)
        {
            return;
        }

        _func(_pNode->aabb, _pNode->depth, _pNode->numItemsInTree, _pNode->IsLeaf());

        if (_pNode->HasChildren())
        {
            for (const OcNode* pChild: _pNode->children)
            {
                TraverseNodes_Recursive_(pChild, _func);
            }
        }
    }

    template<typename TraverseFunc>
    void BottomUpTraverse_Recursive_(const OcNode* _pNode, TraverseFunc& _func) const
    {
        if (_pNode->numItemsInTree == 0)
        {
            return;
        }

        if (_pNode->HasChildren())
        {
            for (const OcNode* pChild: _pNode->children)
            {
                BottomUpTraverse_Recursive_(pChild, _func);
            }
        }

        const Int32 numItems = static_cast<Int32>(_pNode->items.size());
        for (Int32 i = 0; i < numItems; ++i)
        {
            _func(_pNode->items[i].entity, _pNode->aabbs[i], _pNode->items[i].userdata);
        }
    }

    void SplitNode_(OcNode* _pNode)
    {
        JAM_ASSERT(_pNode->IsLeaf(), "Node already has children");

        const Vec3 center        = _pNode->GetCenter();
        const Vec3 childLooseExt = _pNode->GetExtents() * 0.5f;
        const Vec3 childBaseExt  = childLooseExt / m_info.looseFactor;

        for (Int32 i = 0; i < 8; ++i)
        {
            const eRegion region      = static_cast<eRegion>(i);
            Vec3          childCenter = center;
            childCenter.x += IsRight(region) ? childBaseExt.x : -childBaseExt.x;
            childCenter.y += IsTop(region) ? childBaseExt.y : -childBaseExt.y;
            childCenter.z += IsFar(region) ? childBaseExt.z : -childBaseExt.z;

            OcNode* pChild      = MakeNode_(Aabb { childCenter, childLooseExt });
            pChild->pParent     = _pNode;
            pChild->depth       = _pNode->depth + 1;
            _pNode->children[i] = pChild;
        }
    }

    [[nodiscard]] bool CanSubdivide_(const OcNode* _pNode, const Int32 _numItems) const
    {
        if (!_pNode->IsLeaf())
        {
            return false;
        }

        if (_numItems <= m_info.subdivisionThreshold)
        {
            return false;
        }

        if (m_info.bUseMaxDepthPolicy && _pNode->depth >= m_info.maxDepth)
        {
            return false;
        }

        const Vec3 childBaseExt = (_pNode->GetExtents() * 0.5f) / m_info.looseFactor;
        return childBaseExt.x >= m_info.minBaseExtents.x && childBaseExt.y >= m_info.minBaseExtents.y && childBaseExt.z >= m_info.minBaseExtents.z;
    }

    void SubdivideIfNeed_(OcNode* _pNode)
    {
        if (!CanSubdivide_(_pNode, static_cast<Int32>(_pNode->items.size())))
        {
            return;
        }

        // 현재 노드를 8개의 자식들로 분할
        SplitNode_(_pNode);

        // 기존 노드에 있던 아이템들을 자식 노드들로 재삽입
        Vector<Aabb>   aabbs = std::move(_pNode->aabbs);
        Vector<OcItem> items = std::move(_pNode->items);
        _pNode->aabbs.clear();
        _pNode->items.clear();
        _pNode->numItemsInTree = 0;   // InsertItem_ 에서 재계산됨

        const Int32 numItems = static_cast<Int32>(items.size());
        for (Int32 i = 0; i < numItems; ++i)
        {
            InsertItem_(_pNode, std::move(items[i]), aabbs[i]);
        }

        // 사후조건: 분할전 Node의 아이템 수 = 분할후 Node들의 아이템 수의 합 + 현재 Node의 아이템 수
    }

    void AddItem_(OcNode* _pNode, OcItem&& _item, const Aabb& _aabb)
    {
        m_slots[_item.handle] = OcSlot { _pNode, static_cast<Int32>(_pNode->items.size()) };
        _pNode->aabbs.push_back(_aabb);
        _pNode->items.push_back(std::move(_item));
        SubdivideIfNeed_(_pNode);
    }

    // _pStart부터 내려가며 가장 깊은 적합 노드에 삽입. _pStart는 _aabb를 포함해야 함
    void InsertItem_(OcNode* _pStart, OcItem&& _item, const Aabb& _aabb)
    {
        JAM_ASSERT(_pStart->Contains(_aabb), "Item does not fit in node");

        OcNode* pNode = _pStart;
        for (;;)
        {
            ++pNode->numItemsInTree;   // 삽입 보장. numItemsInTree 즉시 증가.

            if (pNode->HasChildren())
            {
                if (OcNode* pChild = FindBestChild_(pNode, _aabb))
                {
                    pNode = pChild;
                    continue;
                }
            }
            break;
        }

        AddItem_(pNode, std::move(_item), _aabb);
    }

    // swap and pop (삽입 순서 보장 불필요)
    [[nodiscard]] OcItem PopItem_(const OctreeItemHandle _handle)
    {
        const OcSlot slot    = m_slots[_handle];
        OcNode*      pNode   = slot.pNode;
        const Int32  lastIdx = static_cast<Int32>(pNode->items.size()) - 1;
        OcItem       popped  = std::move(pNode->items[slot.index]);

        if (slot.index != lastIdx)
        {
            pNode->items[slot.index]                       = std::move(pNode->items[lastIdx]);
            pNode->aabbs[slot.index]                       = pNode->aabbs[lastIdx];
            m_slots[pNode->items[slot.index].handle].index = slot.index;
        }
        pNode->items.pop_back();
        pNode->aabbs.pop_back();

        m_slots[_handle] = OcSlot {};
        return popped;
    }

    // [_begin, _end) 아이템을 _pNode 아래에 배치. 모두 _pNode에 들어가야 함
    // m_buildOrder는 인덱스 순열 (키 32B 대신 인덱스 4B를 이동)
    void BuildNode_(OcNode* _pNode, const BuildItem* _pItems, const Int32 _begin, const Int32 _end)
    {
        const Int32 count      = _end - _begin;
        _pNode->numItemsInTree = count;

        if (!CanSubdivide_(_pNode, count))
        {
            AddBuildItems_(_pNode, _pItems, _begin, _end);
            return;
        }

        SplitNode_(_pNode);

        // 카운팅 정렬. 버킷 0~7은 자식, 8은 현재 노드에 남는 아이템
        const Vec3 center    = _pNode->aabb.center;
        Int32      counts[9] = {};
        for (Int32 p = _begin; p < _end; ++p)
        {
            const Aabb&   aabb   = _pItems[m_buildOrder[p]].aabb;
            const eRegion region = CalcRegion(aabb.center.x > center.x, aabb.center.y > center.y, aabb.center.z > center.z);
            const Int32   bucket = _pNode->children[static_cast<Int32>(region)]->Contains(aabb) ? static_cast<Int32>(region) : 8;

            m_buildBucket[p] = static_cast<UInt8>(bucket);
            ++counts[bucket];
        }

        Int32 starts[9] = {};
        Int32 cursor    = _begin;
        starts[8]       = cursor;   // 남는 아이템을 맨 앞에 둔다
        cursor += counts[8];
        for (Int32 b = 0; b < 8; ++b)
        {
            starts[b] = cursor;
            cursor += counts[b];
        }

        Int32 writes[9];
        for (Int32 b = 0; b < 9; ++b)
        {
            writes[b] = starts[b];
        }
        for (Int32 p = _begin; p < _end; ++p)
        {
            m_buildScratch[writes[m_buildBucket[p]]++] = m_buildOrder[p];
        }
        for (Int32 p = _begin; p < _end; ++p)
        {
            m_buildOrder[p] = m_buildScratch[p];
        }

        AddBuildItems_(_pNode, _pItems, starts[8], starts[8] + counts[8]);
        for (Int32 b = 0; b < 8; ++b)
        {
            BuildNode_(_pNode->children[b], _pItems, starts[b], starts[b] + counts[b]);
        }
    }

    void AddBuildItems_(OcNode* _pNode, const BuildItem* _pItems, const Int32 _begin, const Int32 _end)
    {
        const Int32 count = _end - _begin;
        _pNode->aabbs.reserve(_pNode->aabbs.size() + count);
        _pNode->items.reserve(_pNode->items.size() + count);

        for (Int32 p = _begin; p < _end; ++p)
        {
            const Int32      index  = m_buildOrder[p];
            const BuildItem& source = _pItems[index];

            m_slots[index] = OcSlot { _pNode, static_cast<Int32>(_pNode->items.size()) };
            _pNode->aabbs.push_back(source.aabb);
            _pNode->items.push_back(OcItem { source.entity, source.userdata, index });
        }
    }

    void Expand_(const Aabb& _aabb)
    {
        // 옥트리 확장로직.
        // _aabb는 옥트리 외부에 있어야하며, _aabb가 확장의 힌트가 된다.
        // 기존 루트는 _aabb의 반대 방향 슬롯에 배치되고, 새 루트는 기존 루트와 _aabb를 모두 포함하도록 확장된다.
        // 반대 방향에 기존 루트를 배치하는 이유는, _aabb 방향으로 확장하기 위함.

        JAM_ASSERT(!m_pRoot->Contains(_aabb), "Root already contains the target AABB");
        const Vec3 center      = m_pRoot->GetCenter();
        const Vec3 oldBaseExt  = m_pRoot->GetExtents() / m_info.looseFactor;
        const Vec3 newLooseExt = m_pRoot->GetExtents() * 2.f;

        // _aabb 의 반대 방향 슬롯에 기존 루트를 배치.
        // bFar, bBottom, bRight 는 _aabb 가 새 루트의 어느 슬롯에 위치하는지 계산하기 위함.
        bool bRight = false;
        bool bTop   = false;
        bool bFar   = false;

        Vec3 newCenter = center;

        if (center.x < _aabb.center.x)
        {
            newCenter.x += oldBaseExt.x;
        }
        else
        {
            newCenter.x -= oldBaseExt.x;
            bRight = true;
        }

        if (center.y < _aabb.center.y)
        {
            newCenter.y += oldBaseExt.y;
        }
        else
        {
            newCenter.y -= oldBaseExt.y;
            bTop = true;
        }

        if (center.z < _aabb.center.z)
        {
            newCenter.z += oldBaseExt.z;
        }
        else
        {
            newCenter.z -= oldBaseExt.z;
            bFar = true;
        }

        OcNode* pNewRoot         = MakeNode_(Aabb { newCenter, newLooseExt });
        pNewRoot->depth          = 0;
        pNewRoot->numItemsInTree = m_pRoot->numItemsInTree;

        SplitNode_(pNewRoot);   // 8개 더미 자식 생성

        // 새 루트 슬롯의 자식을 기존 루트로 교체
        OcNode*     pOldRoot = m_pRoot;
        const Int32 index    = static_cast<Int32>(CalcRegion(bRight, bTop, bFar));
        DestroySubtree_(pNewRoot->children[index]);
        pNewRoot->children[index] = pOldRoot;
        pOldRoot->pParent         = pNewRoot;
        pOldRoot->depth           = 1;
        UpdateDepth_Recursive_(pOldRoot);

        m_pRoot = pNewRoot;

        // maxDepth 정책 처리
        if (m_info.bIncreaseMaxDepthOnExpand)
        {
            ++m_info.maxDepth;
        }
        else if (m_info.bUseMaxDepthPolicy)
        {
            MergeTooDeepChildren_Recursive_(m_pRoot);
        }
    }

    void UpdateDepth_()
    {
        m_pRoot->depth = 0;
        UpdateDepth_Recursive_(m_pRoot);
    }

    void UpdateDepth_Recursive_(const OcNode* _pNode)
    {
        // 부모 depth 기준으로 자식 depth 결정
        if (_pNode->HasChildren())
        {
            for (OcNode* pChild: _pNode->children)
            {
                pChild->depth = _pNode->depth + 1;
                UpdateDepth_Recursive_(pChild);
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
        for (OcNode* pChild: _pNode->children)
        {
            MergeTooDeepChildren_Recursive_(pChild);
        }

        if (_pNode->depth >= m_info.maxDepth)
        {
            for (OcNode* pChild: _pNode->children)
            {
                const Int32 numItems = static_cast<Int32>(pChild->items.size());
                for (Int32 i = 0; i < numItems; ++i)
                {
                    m_slots[pChild->items[i].handle] = OcSlot { _pNode, static_cast<Int32>(_pNode->items.size()) };
                    _pNode->aabbs.push_back(pChild->aabbs[i]);
                    _pNode->items.push_back(std::move(pChild->items[i]));
                }
            }
            DestroyChildren_(_pNode);
        }
    }

    void MergeEmptyChildren_Recursive_(OcNode* _pNode)
    {
        if (!_pNode->HasChildren())
        {
            return;
        }

        // 자식부터 먼저 처리
        for (OcNode* pChild: _pNode->children)
        {
            MergeEmptyChildren_Recursive_(pChild);
        }

        const Int32 numItemsInChildren = _pNode->numItemsInTree - static_cast<Int32>(_pNode->items.size());
        if (numItemsInChildren == 0)
        {
            DestroyChildren_(_pNode);
        }
    }

    [[nodiscard]] OcNode* FindBestChild_(   // or nullptr
        const OcNode* _pRoot,
        const Aabb&   _aabb) const
    {
        JAM_ASSERT(_pRoot->HasChildren(), "Node has no children");

        const Vec3    rootCenter = _pRoot->aabb.center;
        const Vec3    center     = _aabb.center;
        const eRegion region     = CalcRegion(center.x > rootCenter.x, center.y > rootCenter.y, center.z > rootCenter.z);
        OcNode*       pNode      = _pRoot->children[static_cast<Int32>(region)];
        return pNode->Contains(_aabb) ? pNode : nullptr;
    }

    ObjectPool<OcNode>          m_nodePool;
    OctreeInfo                  m_info;
    OctreeInfo                  m_initInfo;
    OcNode*                     m_pRoot = nullptr;
    Vector<OcSlot>              m_slots;
    Vector<OctreeItemHandle>    m_freeSlots;
    mutable HashMap<UID, Int32> m_entitySlots;
    mutable bool                m_bEntitySlotsValid = true;

    // Build 작업 버퍼
    Vector<Int32> m_buildOrder;
    Vector<Int32> m_buildScratch;
    Vector<UInt8> m_buildBucket;
};

}   // namespace Jam
