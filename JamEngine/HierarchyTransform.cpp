#include "pch.h"

#include "HierarchyTransform.h"

namespace jam
{

void HierarchyTransform::SetLocal(const Transform& _local)
{
    m_local       = _local;
    m_updateState = eUpdateState::WorldNeedUpdate;
}

const Transform& HierarchyTransform::GetLocal() const
{
    return m_local;
}

void HierarchyTransform::SetWorld(const Transform& _world)
{
    m_world       = _world;
    m_updateState = eUpdateState::LocalNeedUpdate;
}

const Transform& HierarchyTransform::GetWorld() const
{
    return m_world;
}

void HierarchyTransform::SetParent(HierarchyTransform& _parent)
{
    HierarchyTransform* pParent = &_parent;
    JAM_ASSERT(pParent != this, "Cannot set self as parent");

    // 이미 부모가 동일하면 무시
    if (m_pParent == pParent)
        return;   

    // 기존 부모에서 해제
    ReleaseParent();   

    // 새로운 부모에 연결
    m_pParent                             = pParent;
    HierarchyTransform* pParentFirstChild = m_pParent->m_pFirstChild;
    m_pParent->m_pFirstChild              = this;
    m_pNextSibling                        = pParentFirstChild;
    m_pPrevSibling                        = nullptr;
    if (pParentFirstChild)
    {
        pParentFirstChild->m_pPrevSibling = this;
    }

    // 부모를 기준으로 로컬 재계산
    m_updateState = eUpdateState::LocalNeedUpdate;
}

void HierarchyTransform::ReleaseParent()
{
    if (m_pParent)   // 부모가 있을 때만 실행
    {
        if (m_pPrevSibling)
        {
            // 이전 형제와 다음 형제(혹은 nullptr)를 연결
            // m_pNextSibling이 nullptr인 경우는 해당 노드가 마지막 자식인 경우
            m_pPrevSibling->m_pNextSibling = m_pNextSibling;
        }
        else
        {
            // 이전 형제가 없는 경우는 해당 노드가 첫 자식인 경우
            m_pParent->m_pFirstChild = m_pNextSibling;
        }

        if (m_pNextSibling)
        {
            // 다음 형제와 이전 형제(혹은 nullptr)를 연결
            // m_pPrevSibling이 nullptr인 경우는 해당 노드가 첫 자식인 경우
            m_pNextSibling->m_pPrevSibling = m_pPrevSibling;
        }

        // 고아 노드
        m_pParent      = nullptr;
        m_pNextSibling = nullptr;
        m_pPrevSibling = nullptr;

        m_world = m_local;   // 부모가 없으면 로컬과 월드가 동일
    }
}

void HierarchyTransform::Update_Recursive()
{
    if (m_updateState == eUpdateState::LocalNeedUpdate)
    {
        // 월드를 기준으로 로컬 업데이트
        if (m_pParent)
        {
            m_local = m_world.FromParent(m_pParent->m_world);
        }
        else
        {
            m_local = m_world;   // 부모가 없으면 로컬과 월드가 동일
        }
    }
    else if (m_updateState == eUpdateState::WorldNeedUpdate)
    {
        // 로컬을 기준으로 월드 업데이트
        if (m_pParent)
        {
            m_world = m_local.ToParent(m_pParent->m_world);
        }
        else
        {
            m_world = m_local;   // 부모가 없으면 로컬과 월드가 동일
        }
    }

    bool bDirty   = (m_updateState != eUpdateState::Noop);
    m_updateState = eUpdateState::Noop;

    // 만약 자신이 더티였다면 자식들도 더티
    HierarchyTransform* pChild = m_pFirstChild;
    while (pChild)
    {
        if (bDirty && (pChild->m_updateState == eUpdateState::Noop))
        {
            // 자식이 월드 변경이 발생했으면 그것은 유지
            // 부모가 변경되었으면 자식은 월드를 갱신해야함
            pChild->m_updateState = eUpdateState::WorldNeedUpdate;
        }

        // 자식도 업데이트
        pChild->Update_Recursive();
        pChild = pChild->m_pNextSibling;
    }
}

}   // namespace jam