#include "pch.h"

#include "SceneNode.h"

namespace jam
{

SceneNode::SceneNode()
{
}

SceneNode::SceneNode(const Entity _root, const Entity _owner)
    : m_root(_root)
    , m_owner(_owner)
{
    JAM_ASSERT(_root.IsValid(), "Root entity is invalid");
    JAM_ASSERT(_owner.IsValid(), "Owner entity is invalid");
    SetParent(m_root);
}

SceneNode::SceneNode(const Entity _root)
    : m_root(_root)
    , m_owner(_root)
{
    JAM_ASSERT(_root.IsValid(), "Root entity is invalid");
}

void SceneNode::SetParent(const Entity _parent, const bool _bKeepLocal)
{
    JAM_ASSERT(_parent, "Parent entity is invalid");
    JAM_ASSERT(_parent != m_owner, "Cannot set self as parent");
    JAM_ASSERT(!IsRoot(), "root cannot has parent");
    JAM_ASSERT(!IsAncestorOf(_parent), "Cannot set descendant as parent");

    if (m_parent != _parent)
    {
        // release existing parent
        ReleaseParent_();

        // set new parent
        m_parent              = _parent;
        SceneNode& parentNode = m_parent.GetSceneNode();

        // new parent's first child becomes me
        Entity oldFirstChild    = parentNode.m_firstChild;
        parentNode.m_firstChild = m_owner;
        m_nextSibling           = oldFirstChild;
        m_prevSibling           = Entity::s_null;
        if (oldFirstChild)
        {
            // if there was an old first child, update its prev sibling to me
            SceneNode& oldFirstChildNode    = oldFirstChild.GetSceneNode();
            oldFirstChildNode.m_prevSibling = m_owner;
        }

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
}

void SceneNode::SetParentAsRoot(const bool _bKeepLocal)
{
    SetParent(m_root, _bKeepLocal);
}

void SceneNode::Orphanize()
{
    ReleaseParent_();
}

void SceneNode::ReleaseParent_()
{
    // only release if has parent
    if (HasParent())
    {
        // detach from parent
        SceneNode& parentNode = m_parent.GetSceneNode();
        if (m_prevSibling)
        {
            // there is a previous sibling
            SceneNode& prevSiblingNode    = m_prevSibling.GetSceneNode();
            prevSiblingNode.m_nextSibling = m_nextSibling;
        }
        else
        {
            // this is the first child
            parentNode.m_firstChild = m_nextSibling;
        }

        if (m_nextSibling)
        {
            // there is a next sibling
            SceneNode& nextSiblingNode    = m_nextSibling.GetSceneNode();
            nextSiblingNode.m_prevSibling = m_prevSibling;
        }

        // clear links
        m_parent      = Entity::s_null;
        m_nextSibling = Entity::s_null;
        m_prevSibling = Entity::s_null;
    }
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

        SceneNode& parentNode = parent.GetSceneNode();
        parent                = parentNode.m_parent;
    }
    return false;
}

bool SceneNode::IsAncestorOf(Entity _entity) const
{
    JAM_ASSERT(_entity, "Entity is invalid");

    SceneNode& node = _entity.GetSceneNode();
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

const Transform& SceneNode::GetLocalTransform() const
{
    return m_local;
}

const Transform& SceneNode::GetWorldTransform() const
{
    return m_world;
}

Transform& SceneNode::ModifyLocalTransform()
{
    m_dirty = eDirty::WorldNeedUpdate;
    NotifyDirty_();
    return m_local;
}

Transform& SceneNode::ModifyWorldTransform()
{
    m_dirty = eDirty::LocalNeedUpdate;
    NotifyDirty_();
    return m_world;
}

void SceneNode::UpdateDirty_Recursive_()
{
    if (m_dirty != eDirty::None)   // else: not dirty
    {
        if (m_dirty == eDirty::LocalNeedUpdate)
        {
            if (HasParent())
            {
                // update local based on world
                SceneNode& parentNode = m_parent.GetSceneNode();
                m_local               = m_world;
                m_local.Mult(parentNode.GetWorldTransform().Inverted());
            }
            else
            {
                // root
                m_local = m_world;
            }
        }
        else if (m_dirty == eDirty::WorldNeedUpdate)
        {
            if (HasParent())
            {
                // update world based on local
                SceneNode& parentNode = m_parent.GetSceneNode();
                m_world               = m_local;
                m_world.Mult(parentNode.GetWorldTransform());
            }
            else
            {
                // root
                m_world = m_local;
            }
        }

        Entity child = m_firstChild;
        while (child)
        {
            // if parent changed directly, mark child as world need update
            // but not if child itself is already marked as world need update
            SceneNode& childNode = child.GetSceneNode();
            if (m_dirty != eDirty::ChildDirty && childNode.m_dirty != eDirty::LocalNeedUpdate)
            {
                childNode.m_dirty = eDirty::WorldNeedUpdate;
            }

            // recursive update
            childNode.UpdateDirty_Recursive_();
            child = childNode.m_nextSibling;
        }

        m_dirty = eDirty::None;   // clear dirty flag
    }
}

void SceneNode::NotifyDirty_()
{
    if (m_parent)   // else: root node
    {
        SceneNode& parentNode = m_parent.GetSceneNode();
        if (parentNode.m_dirty == eDirty::None)
        {
            parentNode.m_dirty = eDirty::ChildDirty;
            parentNode.NotifyDirty_();
        }
        // else: already dirty
    }
}

}   // namespace jam