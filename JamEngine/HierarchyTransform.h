#pragma once
#include "Entity.h"
#include "Transform.h"

namespace jam
{

class HierarchyTransform
{
    enum class eDirtyFlag : Int8
    {
        Noop,
        WorldNeedUpdate,
        LocalNeedUpdate,
        ChildDirty
    };

public:
    HierarchyTransform() = default;
    HierarchyTransform(Entity _root, Entity _owner);
    explicit HierarchyTransform(Entity _root);

    ////////// LOCAL TRANSFORM //////////

    void                           SetLocal(const Transform& _local);
    JAM_NODISCARD const Transform& GetLocal() const;

    ////////// WORLD TRANSFORM //////////

    void                           SetWorld(const Transform& _world);
    JAM_NODISCARD const Transform& GetWorld() const;

    ////////// HEIRARCHY //////////

    void SetParent(Entity _parent);
    void ReleaseParent();

    JAM_NODISCARD bool   IsRoot() const { return m_root == m_owner; }
    JAM_NODISCARD Entity GetRoot() const { return m_root; }
    JAM_NODISCARD Entity GetOwner() const { return m_owner; }
    JAM_NODISCARD Entity GetParent() const { return m_parent; }
    JAM_NODISCARD Entity GetFirstChild() const { return m_firstChild; }
    JAM_NODISCARD Entity GetNextSibling() const { return m_nextSibling; }
    JAM_NODISCARD Entity GetPrevSibling() const { return m_prevSibling; }

    ////////// UPDATE //////////

    // 루트 노드가 매프레임 딱 한 번 호출할 것
    void Update_Recursive();

private:
    void NotifyDirtyToParent_();

    eDirtyFlag m_dirtyFlag = eDirtyFlag::Noop;

    Transform m_local;
    Transform m_world;

    Entity m_root        = Entity::s_null;
    Entity m_owner       = Entity::s_null;
    Entity m_parent      = Entity::s_null;
    Entity m_firstChild  = Entity::s_null;
    Entity m_nextSibling = Entity::s_null;
    Entity m_prevSibling = Entity::s_null;
};

}   // namespace jam
