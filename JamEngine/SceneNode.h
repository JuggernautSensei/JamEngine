#pragma once
#include "Entity.h"
#include "Transform.h"

namespace jam
{

class SceneNode
{
    friend class Scene;

    enum class eDirty
    {
        None,
        WorldNeedUpdate,
        LocalNeedUpdate,
        ChildDirty
    };

public:
    SceneNode();
    explicit SceneNode(Entity _root, Entity _owner);   // create regular pNode
    explicit SceneNode(Entity _root);                  // create root pNode

    // hierarchy operations
    void SetParent(Entity _parent, bool _bKeepLocal = false);
    void SetParentAsRoot(bool _bKeepLocal = false);
    void Orphanize();

    [[nodiscard]] bool IsDescendantOf(Entity _entity) const;   // check if this is a descendant of _entity
    [[nodiscard]] bool IsAncestorOf(Entity _entity) const;     // check if this is an ancestor of _entity

    [[nodiscard]] bool IsRoot() const;
    [[nodiscard]] bool HasParent() const;

    [[nodiscard]] Entity GetRoot() const;
    [[nodiscard]] Entity GetOwner() const;
    [[nodiscard]] Entity GetParent() const;
    [[nodiscard]] Entity GetFirstChild() const;
    [[nodiscard]] Entity GetNextSibling() const;
    [[nodiscard]] Entity GetPrevSibling() const;

    [[nodiscard]] const Transform& GetLocalTransform() const;
    [[nodiscard]] const Transform& GetWorldTransform() const;
    [[nodiscard]] Transform&       ModifyLocalTransform();
    [[nodiscard]] Transform&       ModifyWorldTransform();

private:
    void NotifyDirty_();
    void ReleaseParent_();
    void UpdateDirty_Recursive_();

    eDirty m_dirty = eDirty::None;

    Transform m_local = {};
    Transform m_world = {};

    Entity m_root        = Entity::s_null;
    Entity m_owner       = Entity::s_null;
    Entity m_parent      = Entity::s_null;
    Entity m_firstChild  = Entity::s_null;
    Entity m_nextSibling = Entity::s_null;
    Entity m_prevSibling = Entity::s_null;
};

}   // namespace jam