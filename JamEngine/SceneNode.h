#pragma once
#include "Entity.h"
#include "Transform.h"

namespace Jam
{

class SceneNode
{
    enum class eDirty
    {
        None,
        WorldNeedUpdate,
        LocalNeedUpdate,
        ChildDirty
    };

public:
    SceneNode();
    SceneNode(Entity _root, Entity _owner);   // create regular pNode
    explicit SceneNode(Entity _root);         // create root pNode

    void SetParent(Entity _parent, bool _bKeepLocal = false);
    void SetParentAsRoot(bool _bKeepLocal = false);
    void Orphanize();

    [[nodiscard]] bool IsDescendantOf(Entity _entity) const;   // check if this is a descendant of _entity
    [[nodiscard]] bool IsAncestorOf(Entity _entity) const;     // check if this is an ancestor of _entity

    [[nodiscard]] bool IsRoot() const;
    [[nodiscard]] bool HasParent() const;
    [[nodiscard]] bool HasChildren() const;

    [[nodiscard]] Entity GetRoot() const;
    [[nodiscard]] Entity GetOwner() const;
    [[nodiscard]] Entity GetParent() const;
    [[nodiscard]] Entity GetFirstChild() const;
    [[nodiscard]] Entity GetNextSibling() const;
    [[nodiscard]] Entity GetPrevSibling() const;

    [[nodiscard]] const Transform& GetLocal() const;
    void                           SetLocal(const Transform& _local);
    [[nodiscard]] Transform&       ModifyLocal();

    [[nodiscard]] const Transform& GetWorld() const;
    void                           SetWorld(const Transform& _world);
    [[nodiscard]] Transform&       ModifyWorld();

    void UpdateGraph();
    void UpdateGraph(Vector<Entity>& _updatedEntities);

private:
    void NotifyDirty_();
    void ReleaseParent_();
    void UpdateTree_Recursive_(Vector<Entity>* _pUpdatedEntitiesOrNull);

    eDirty m_dirty = eDirty::None;

    Transform m_local = {};
    Transform m_world = {};

    Entity m_root        = Entity::kNull;
    Entity m_owner       = Entity::kNull;
    Entity m_parent      = Entity::kNull;
    Entity m_firstChild  = Entity::kNull;
    Entity m_nextSibling = Entity::kNull;
    Entity m_prevSibling = Entity::kNull;
};

}   // namespace Jam