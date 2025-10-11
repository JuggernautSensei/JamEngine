#pragma once
#include "Transform.h"

namespace jam
{

class HierarchyTransform
{
    enum class eUpdateState : Int8
    {
        Noop,
        WorldNeedUpdate,
        LocalNeedUpdate
    };

public:
    ////////// LOCAL TRANSFORM //////////

    void                           SetLocal(const Transform& _local);
    JAM_NODISCARD const Transform& GetLocal() const;

    ////////// WORLD TRANSFORM //////////

    void                           SetWorld(const Transform& _world);
    JAM_NODISCARD const Transform& GetWorld() const;

    ////////// HEIRARCHY //////////

    void SetParent(HierarchyTransform& _parent);
    void ReleaseParent();

    JAM_NODISCARD bool                IsRoot() const { return m_pParent == nullptr; }
    JAM_NODISCARD HierarchyTransform* GetParent() const { return m_pParent; }
    JAM_NODISCARD HierarchyTransform* GetFirstChild() const { return m_pFirstChild; }
    JAM_NODISCARD HierarchyTransform* GetNextSibling() const { return m_pNextSibling; }
    JAM_NODISCARD HierarchyTransform* GetPrevSibling() const { return m_pPrevSibling; }

    ////////// UPDATE //////////

    // 자신과 자식들까지 재귀적으로 업데이트
    // 프레임 당 한 번 최상위 부모에서 한 번만 호출하면 됨
    // 아마도 직접 호출할 일은 없을 것
    void Update_Recursive();

private:
    eUpdateState m_updateState = eUpdateState::Noop;
    Transform    m_local;
    Transform    m_world;

    HierarchyTransform* m_pParent      = nullptr;
    HierarchyTransform* m_pFirstChild  = nullptr;
    HierarchyTransform* m_pNextSibling = nullptr;
    HierarchyTransform* m_pPrevSibling = nullptr;
};

}   // namespace jam
