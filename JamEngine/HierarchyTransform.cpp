#include "pch.h"

#include "HierarchyTransform.h"

#include "Components.h"

namespace jam
{

HierarchyTransform::HierarchyTransform(const Entity _root, const Entity _owner)
    : m_root(_root)
    , m_owner(_owner)
{
    JAM_ASSERT(_root.IsValid(), "Root entity is invalid");
    SetParent(m_root);
}

HierarchyTransform::HierarchyTransform(const Entity _root)
    : m_root(_root)
    , m_owner(_root)
{
    JAM_ASSERT(_root.IsValid(), "Root entity is invalid");
}

void HierarchyTransform::SetLocal(const Transform& _local)
{
    m_local     = _local;
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

const Transform& HierarchyTransform::GetLocal() const
{
    return m_local;
}

void HierarchyTransform::SetWorld(const Transform& _world)
{
    m_world     = _world;
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

const Transform& HierarchyTransform::GetWorld() const
{
    return m_world;
}

void HierarchyTransform::SetParent(const Entity _parent)
{
    JAM_ASSERT(_parent.IsValid(), "Parent entity is invalid");
    JAM_ASSERT(_parent != m_owner, "Cannot set self as parent");

    if (m_parent == _parent)
    {
        return;
    }

    // 기존 계층 정리
    if (m_parent.IsValid())
    {
        HierarchyTransform& parentT = m_parent.GetHierarchyTransform();
        if (!m_prevSibling.IsValid())   // 내가 첫 번째 자식
        {
            parentT.m_firstChild = m_nextSibling;
        }
        else   // 내가 첫 번째 자식이 아님
        {
            HierarchyTransform& prevSiblingT = m_prevSibling.GetHierarchyTransform();
            prevSiblingT.m_nextSibling       = m_nextSibling;   // 다음 형제 혹은 null
        }

        if (m_nextSibling.IsValid())
        {
            // 다음 형제와 이전 형제(혹은 null)를 연결
            HierarchyTransform& nextSiblingT = m_nextSibling.GetHierarchyTransform();
            nextSiblingT.m_prevSibling       = m_prevSibling;
        }

        m_parent      = Entity::s_null;
        m_nextSibling = Entity::s_null;
        m_prevSibling = Entity::s_null;
    }

    // 새로운 부모 설정
    m_parent                     = _parent;
    HierarchyTransform& nParentT = m_parent.GetHierarchyTransform();

    Entity nParentFChild  = nParentT.m_firstChild;
    nParentT.m_firstChild = m_owner;
    m_nextSibling         = nParentFChild;
    m_prevSibling         = Entity::s_null;
    if (nParentFChild.IsValid())
    {
        HierarchyTransform& nParentFChildT = nParentFChild.GetHierarchyTransform();
        nParentFChildT.m_prevSibling       = m_owner;
    }

    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;   // 로컬 재게산
    NotifyDirtyToParent_();
}

void HierarchyTransform::ReleaseParent()
{
    SetParent(m_root);
}

void HierarchyTransform::Update_Recursive()
{
    if (m_dirtyFlag == eDirtyFlag::Noop)
        return;

    if (m_dirtyFlag == eDirtyFlag::LocalNeedUpdate)
    {
        // 월드를 기준으로 로컬 업데이트
        if (m_parent.IsValid())
        {
            HierarchyTransform& parentT = m_parent.GetHierarchyTransform();
            m_local                     = m_world.FromParent(parentT.m_world);
        }
        else   // 루트 노드
        {
            m_local = m_world;
        }
    }
    else if (m_dirtyFlag == eDirtyFlag::WorldNeedUpdate)
    {
        // 로컬을 기준으로 월드 업데이트
        if (m_parent.IsValid())
        {
            HierarchyTransform& parentT = m_parent.GetHierarchyTransform();
            m_world                     = m_local.ToParent(parentT.m_world);
        }
        else   // 루트 노드
        {
            m_world = m_local;
        }
    }

    // 자식이 더티면 나는 아무것도 안하고. update 만 전파
    m_dirtyFlag = eDirtyFlag::Noop;
    for (Entity child = m_firstChild; child.IsValid();)
    {
        HierarchyTransform& childT = child.GetHierarchyTransform();
        childT.Update_Recursive();
        child = childT.m_nextSibling;
    }
}

void HierarchyTransform::NotifyDirtyToParent_()
{
    if (!m_parent.IsValid())   // 루트 노드
    {
        return;
    }

    HierarchyTransform& parentT = m_parent.GetHierarchyTransform();
    if (parentT.m_dirtyFlag == eDirtyFlag::Noop)
    {
        parentT.m_dirtyFlag = eDirtyFlag::ChildDirty;
        parentT.NotifyDirtyToParent_();
    }

    // 이미 부모가 더티 플래그가 설정되어 있으므로 더 이상 전파할 필요 없음
}

}   // namespace jam