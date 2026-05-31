#include "pch.h"

#include "SceneNode.h"
#include "Scene.h"

namespace Jam
{

SceneNode::SceneNode()
{
}

SceneNode::SceneNode(
    const Entity _root,
    const Entity _owner)
    : m_root(_root)
    , m_owner(_owner)
{
    JAM_ASSERT(_root, "Root entity is invalid");
    JAM_ASSERT(_owner, "Owner entity is invalid");
    SetParent(m_root);
}

SceneNode::SceneNode(
    const Entity _root)
    : m_root(_root)
    , m_owner(_root)
{
    JAM_ASSERT(_root, "Root entity is invalid");
}

void SceneNode::SetParent(
    const Entity _parent,
    const bool   _bKeepLocal)
{
    JAM_ASSERT(_parent, "Parent entity is invalid");
    JAM_ASSERT(_parent != m_owner, "Cannot set self as parent");
    JAM_ASSERT(!IsRoot(), "root cannot has parent");
    JAM_ASSERT(!IsAncestorOf(_parent), "Cannot set descendant as parent");

    if (m_parent == _parent)
    {
        return;
    }

    // release existing parent
    ReleaseParent_();

    // set new parent
    m_parent              = _parent;
    SceneNode& parentNode = m_parent.GetSceneNode();

    // 현 노드가 부모의 새로운 첫 번째 노드가 됨
    Entity oldFirstChild    = parentNode.m_firstChild;
    parentNode.m_firstChild = m_owner;
    m_nextSibling           = oldFirstChild;
    m_prevSibling           = Entity::kNull;
    if (oldFirstChild)
    {
        // 기존 자식의 이전 형제를 나로 설정함
        SceneNode& oldFirstChildNode    = oldFirstChild.GetSceneNode();
        oldFirstChildNode.m_prevSibling = m_owner;
    }

    // 부모가 바뀌었으므로 변환 재계산 필요
    if (_bKeepLocal)
    {
        // keep local -> update world
        m_dirty = eDirty::WorldNeedUpdate;
    }
    else
    {
        // keep world -> update local
        m_dirty = eDirty::LocalNeedUpdate;
    }

    NotifyDirty_();
}

void SceneNode::SetParentAsRoot(const bool _bKeepLocal)
{
    SetParent(m_root, _bKeepLocal);
}

void SceneNode::Orphanize()
{
    ReleaseParent_();
}

bool SceneNode::IsDescendantOf(const Entity _entity) const
{
    JAM_ASSERT(_entity, "Entity is invalid");

    Entity parent = m_parent;
    while (parent)
    {
        if (_entity == parent)
        {
            return true;   // 조상임
        }

        const SceneNode& parentNode = parent.GetSceneNode();
        parent                      = parentNode.m_parent;
    }
    return false;
}

bool SceneNode::IsAncestorOf(Entity _entity) const
{
    JAM_ASSERT(_entity, "Entity is invalid");

    const SceneNode& node = _entity.GetSceneNode();
    return node.IsDescendantOf(m_owner);
}

bool SceneNode::IsRoot() const
{
    return m_owner == m_root;
}

bool SceneNode::HasParent() const
{
    return m_parent.IsValid();
}

bool SceneNode::HasChildren() const
{
    return m_firstChild.IsValid();
}

Entity SceneNode::GetRoot() const
{
    return m_root;
}

Entity SceneNode::GetOwner() const
{
    return m_owner;
}

Entity SceneNode::GetParent() const
{
    return m_parent;
}

Entity SceneNode::GetFirstChild() const
{
    return m_firstChild;
}

Entity SceneNode::GetNextSibling() const
{
    return m_nextSibling;
}

Entity SceneNode::GetPrevSibling() const
{
    return m_prevSibling;
}

const Transform& SceneNode::GetLocal() const
{
    return m_local;
}

const Transform& SceneNode::GetWorld() const
{
    return m_world;
}

void SceneNode::SetLocal(const Transform& _local)
{
    m_local = _local;
    m_dirty = eDirty::WorldNeedUpdate;
    NotifyDirty_();
}

void SceneNode::SetWorld(const Transform& _world)
{
    m_world = _world;
    m_dirty = eDirty::LocalNeedUpdate;
    NotifyDirty_();
}

Transform& SceneNode::ModifyLocal()
{
    m_dirty = eDirty::WorldNeedUpdate;
    NotifyDirty_();
    return m_local;
}

Transform& SceneNode::ModifyWorld()
{
    m_dirty = eDirty::LocalNeedUpdate;
    NotifyDirty_();
    return m_world;
}

void SceneNode::UpdateGraph()
{
    JAM_ASSERT(IsRoot(), "UpdateSceneGraph with _updatedEntities parameter should be called on root node only, to avoid duplicates in _updatedEntities");

    UpdateTree_Recursive_(nullptr);
}

void SceneNode::UpdateGraph(Vector<Entity>& _updatedEntities)
{
    JAM_ASSERT(IsRoot(), "UpdateSceneGraph with _updatedEntities parameter should be called on root node only, to avoid duplicates in _updatedEntities");

    _updatedEntities.clear();
    UpdateTree_Recursive_(&_updatedEntities);
}

void SceneNode::NotifyDirty_()
{
    if (m_parent)
    {
        SceneNode& parent = m_parent.GetSceneNode();
        if (parent.m_dirty == eDirty::None)
        {
            parent.m_dirty = eDirty::ChildDirty;
            parent.NotifyDirty_();
        }
    }
}

void SceneNode::ReleaseParent_()
{
    // only release if has parent
    if (!HasParent())
    {
        return;
    }

    // 부모를 제거.
    // 이전 형제에게 다음 형제 정보를 설정
    SceneNode& parentNode = m_parent.GetSceneNode();
    if (m_prevSibling)
    {
        // there is a previous sibling
        SceneNode& prevSiblingNode    = m_prevSibling.GetSceneNode();
        prevSiblingNode.m_nextSibling = m_nextSibling;
    }
    else
    {
        // 만약 이전 형제가 없는 경우는 내가 첫번째 자식.
        // 부모의 첫번째 자식을 다음 형제로 설정
        parentNode.m_firstChild = m_nextSibling;
    }

    // 다음 형제에게 이전 형제 정보를 설정
    if (m_nextSibling)
    {
        // there is a next sibling
        SceneNode& nextSiblingNode    = m_nextSibling.GetSceneNode();
        nextSiblingNode.m_prevSibling = m_prevSibling;
    }

    // 링크 제거
    m_parent      = Entity::kNull;
    m_nextSibling = Entity::kNull;
    m_prevSibling = Entity::kNull;
}

void SceneNode::UpdateTree_Recursive_(Vector<Entity>* _pUpdatedEntitiesOrNull)
{
    if (m_dirty == eDirty::None)
    {
        return;
    }

    bool bChanged = false;
    if (m_dirty == eDirty::LocalNeedUpdate)
    {
        if (HasParent())
        {
            // update local based on world
            const SceneNode& parentNode     = m_parent.GetSceneNode();
            const Transform& parentWorld    = parentNode.GetWorld();
            const Transform  parentWorldInv = parentWorld.Inverse();
            m_local                         = m_world.Mult(parentWorldInv);
        }
        else
        {
            // root
            m_local = m_world;
        }
        bChanged = true;
    }
    else if (m_dirty == eDirty::WorldNeedUpdate)
    {
        if (HasParent())
        {
            // update world based on local
            const SceneNode& parentNode  = m_parent.GetSceneNode();
            const Transform& parentWorld = parentNode.GetWorld();
            m_world                      = m_local.Mult(parentWorld);
        }
        else
        {
            // root
            m_world = m_local;
        }
        bChanged = true;
    }
    // else: ChildDirty -> do nothing for self

    if (bChanged && _pUpdatedEntitiesOrNull)
    {
        _pUpdatedEntitiesOrNull->emplace_back(m_owner);
    }

    Entity child = m_firstChild;
    while (child)
    {
        // 부모가 직접적으로 변경되었을 경우 자식의 월드 갱신이 필요함
        // 단, 자식이 월드를 변경했을 경우는 제외. 자식의 변경을 존중
        SceneNode& childNode = child.GetSceneNode();
        if (m_dirty != eDirty::ChildDirty && childNode.m_dirty != eDirty::LocalNeedUpdate)
        {
            childNode.m_dirty = eDirty::WorldNeedUpdate;
        }

        // recursive update
        childNode.UpdateTree_Recursive_(_pUpdatedEntitiesOrNull);
        child = childNode.m_nextSibling;
    }

    m_dirty = eDirty::None;   // clear dirty flag
}

}   // namespace Jam