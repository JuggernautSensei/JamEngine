#include "pch.h"

#include "TransformHierarchy.h"

#include "Components.h"

namespace jam
{

TransformHierarchy::TransformHierarchy(const Entity _root, const Entity _owner)
    : m_root(_root)
    , m_owner(_owner)
{
    JAM_ASSERT(_root.IsValid(), "Root entity is invalid");
    JAM_ASSERT(_owner.IsValid(), "Owner entity is invalid");
    SetParent(m_root);
}

TransformHierarchy::TransformHierarchy(const Entity _root)
    : m_root(_root)
{
    JAM_ASSERT(_root.IsValid(), "Root entity is invalid");
}

void TransformHierarchy::SetLocal(const Transform& _local)
{
    m_local     = _local;
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

const Transform& TransformHierarchy::GetLocal() const
{
    return m_local;
}

void TransformHierarchy::SetLocalTranslate(const Vec3& _translate)
{
    m_local.translate = _translate;
    m_dirtyFlag       = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalTranslateX(const float _x)
{
    m_local.translate.x = _x;
    m_dirtyFlag         = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalTranslateY(const float _y)
{
    m_local.translate.y = _y;
    m_dirtyFlag         = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalTranslateZ(const float _z)
{
    m_local.translate.z = _z;
    m_dirtyFlag         = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

Vec3 TransformHierarchy::GetLocalTranslate() const
{
    return m_local.translate;
}

float TransformHierarchy::GetLocalTranslateX() const
{
    return m_local.translate.x;
}

float TransformHierarchy::GetLocalTranslateY() const
{
    return m_local.translate.y;
}

float TransformHierarchy::GetLocalTranslateZ() const
{
    return m_local.translate.z;
}

void TransformHierarchy::AddLocalTranslate(const Vec3& _translate)
{
    m_local.translate += _translate;
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalTranslateX(const float _x)
{
    m_local.translate.x += _x;
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalTranslateY(const float _y)
{
    m_local.translate.y += _y;
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalTranslateZ(const float _z)
{
    m_local.translate.z += _z;
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalRotate(const Quat& _rotate)
{
    m_local.rotate = _rotate;
    m_dirtyFlag    = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalRotate(const Vec3& _axis, const float _angleRad)
{
    m_local.rotate = Quat::CreateFromAxisAngle(_axis, _angleRad);
    m_dirtyFlag    = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalRotateEuler(const Vec3& _eulerRad)
{
    m_local.rotate = Quat::CreateFromYawPitchRoll(_eulerRad);
    m_dirtyFlag    = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalRotateYaw(const float _yawRad)
{
    Vec3 euler     = m_local.rotate.ToEuler();
    euler.y        = _yawRad;
    m_local.rotate = Quat::CreateFromYawPitchRoll(euler);
    m_dirtyFlag    = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalRotatePitch(const float _pitchRad)
{
    Vec3 euler     = m_local.rotate.ToEuler();
    euler.x        = _pitchRad;
    m_local.rotate = Quat::CreateFromYawPitchRoll(euler);
    m_dirtyFlag    = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalRotateRoll(const float _rollRad)
{
    Vec3 euler     = m_local.rotate.ToEuler();
    euler.z        = _rollRad;
    m_local.rotate = Quat::CreateFromYawPitchRoll(euler);
    m_dirtyFlag    = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

Quat TransformHierarchy::GetLocalRotate() const
{
    return m_local.rotate;
}

AxisAngle TransformHierarchy::GetLocalRotateAxisAngle() const
{
    return QuatToAxisAngle(m_local.rotate);
}

Vec3 TransformHierarchy::GetLocalRotateEuler() const
{
    return m_local.rotate.ToEuler();
}

float TransformHierarchy::GetLocalRotateYaw() const
{
    return m_local.rotate.ToEuler().y;
}

float TransformHierarchy::GetLocalRotatePitch() const
{
    return m_local.rotate.ToEuler().x;
}

float TransformHierarchy::GetLocalRotateRoll() const
{
    return m_local.rotate.ToEuler().z;
}

void TransformHierarchy::AddLocalRotate(const Quat& _rotate)
{
    m_local.rotate = m_local.rotate * _rotate;
    m_dirtyFlag    = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalRotate(const Vec3& _axis, const float _angleRad)
{
    Quat deltaRotation = Quat::CreateFromAxisAngle(_axis, _angleRad);
    m_local.rotate     = m_local.rotate * deltaRotation;
    m_dirtyFlag        = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalRotateEuler(const Vec3& _yawPitchRollRad)
{
    Quat deltaRotation = Quat::CreateFromYawPitchRoll(_yawPitchRollRad);
    m_local.rotate     = m_local.rotate * deltaRotation;
    m_dirtyFlag        = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalRotateYaw(const float _yawRad)
{
    Quat deltaRotation = Quat::CreateFromAxisAngle(Vec3::Up, _yawRad);
    m_local.rotate     = m_local.rotate * deltaRotation;
    m_dirtyFlag        = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalRotatePitch(const float _pitchRad)
{
    Quat deltaRotation = Quat::CreateFromAxisAngle(Vec3::Right, _pitchRad);
    m_local.rotate     = m_local.rotate * deltaRotation;
    m_dirtyFlag        = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalRotateRoll(const float _rollRad)
{
    Quat deltaRotation = Quat::CreateFromAxisAngle(Vec3::Forward, _rollRad);
    m_local.rotate     = m_local.rotate * deltaRotation;
    m_dirtyFlag        = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalScale(const Vec3& _scale)
{
    m_local.scale = _scale;
    m_dirtyFlag   = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalScaleX(const float _x)
{
    m_local.scale.x = _x;
    m_dirtyFlag     = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalScaleY(const float _y)
{
    m_local.scale.y = _y;
    m_dirtyFlag     = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalScaleZ(const float _z)
{
    m_local.scale.z = _z;
    m_dirtyFlag     = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

Vec3 TransformHierarchy::GetLocalScale() const
{
    return m_local.scale;
}

void TransformHierarchy::AddLocalScale(const Vec3& _scale)
{
    m_local.scale += _scale;
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalScaleX(const float _x)
{
    m_local.scale.x += _x;
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalScaleY(const float _y)
{
    m_local.scale.y += _y;
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalScaleZ(const float _z)
{
    m_local.scale.z += _z;
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

Vec3 TransformHierarchy::GetLocalForward() const
{
    return m_local.Forward();
}

Vec3 TransformHierarchy::GetLocalBackward() const
{
    return m_local.Backward();
}

Vec3 TransformHierarchy::GetLocalRight() const
{
    return m_local.Right();
}

Vec3 TransformHierarchy::GetLocalLeft() const
{
    return m_local.Left();
}

Vec3 TransformHierarchy::GetLocalUp() const
{
    return m_local.Up();
}

Vec3 TransformHierarchy::GetLocalDown() const
{
    return m_local.Down();
}

void TransformHierarchy::SetWorld(const Transform& _world)
{
    m_world     = _world;
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

const Transform& TransformHierarchy::GetWorld() const
{
    return m_world;
}

void TransformHierarchy::SetParent(const Entity _parent)
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
        TransformHierarchy& parentT = m_parent.GetTransform();
        if (!m_prevSibling.IsValid())   // 내가 첫 번째 자식
        {
            parentT.m_firstChild = m_nextSibling;
        }
        else   // 내가 첫 번째 자식이 아님
        {
            TransformHierarchy& prevSiblingT = m_prevSibling.GetTransform();
            prevSiblingT.m_nextSibling       = m_nextSibling;   // 다음 형제 혹은 null
        }

        if (m_nextSibling.IsValid())
        {
            // 다음 형제와 이전 형제(혹은 null)를 연결
            TransformHierarchy& nextSiblingT = m_nextSibling.GetTransform();
            nextSiblingT.m_prevSibling       = m_prevSibling;
        }

        m_parent      = Entity::s_null;
        m_nextSibling = Entity::s_null;
        m_prevSibling = Entity::s_null;
    }

    // 새로운 부모 설정
    m_parent                     = _parent;
    TransformHierarchy& nParentT = m_parent.GetTransform();

    Entity nParentFChild  = nParentT.m_firstChild;
    nParentT.m_firstChild = m_owner;
    m_nextSibling         = nParentFChild;
    m_prevSibling         = Entity::s_null;
    if (nParentFChild.IsValid())
    {
        TransformHierarchy& nParentFChildT = nParentFChild.GetTransform();
        nParentFChildT.m_prevSibling       = m_owner;
    }

    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;   // 로컬 재게산
    NotifyDirtyToParent_();
}

void TransformHierarchy::ReleaseParent()
{
    SetParent(m_root);
}

void TransformHierarchy::Update_Recursive()
{
    if (m_dirtyFlag == eDirtyFlag::Noop)
        return;

    if (m_dirtyFlag == eDirtyFlag::LocalNeedUpdate)
    {
        // 월드를 기준으로 로컬 업데이트
        if (m_parent.IsValid())
        {
            TransformHierarchy& parentT = m_parent.GetTransform();
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
            TransformHierarchy& parentT = m_parent.GetTransform();
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
        TransformHierarchy& childT = child.GetTransform();
        childT.Update_Recursive();
        child = childT.m_nextSibling;
    }
}

void TransformHierarchy::NotifyDirtyToParent_()
{
    if (!m_parent.IsValid())   // 루트 노드
    {
        return;
    }

    TransformHierarchy& parentT = m_parent.GetTransform();
    if (parentT.m_dirtyFlag == eDirtyFlag::Noop)
    {
        parentT.m_dirtyFlag = eDirtyFlag::ChildDirty;
        parentT.NotifyDirtyToParent_();
    }

    // 이미 부모가 더티 플래그가 설정되어 있으므로 더 이상 전파할 필요 없음
}

void TransformHierarchy::SetWorldTranslate(const Vec3& _translate)
{
    m_world.translate = _translate;
    m_dirtyFlag       = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldTranslateX(const float _x)
{
    m_world.translate.x = _x;
    m_dirtyFlag         = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldTranslateY(const float _y)
{
    m_world.translate.y = _y;
    m_dirtyFlag         = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldTranslateZ(const float _z)
{
    m_world.translate.z = _z;
    m_dirtyFlag         = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

Vec3 TransformHierarchy::GetWorldTranslate() const
{
    return m_world.translate;
}

float TransformHierarchy::GetWorldTranslateX() const
{
    return m_world.translate.x;
}

float TransformHierarchy::GetWorldTranslateY() const
{
    return m_world.translate.y;
}

float TransformHierarchy::GetWorldTranslateZ() const
{
    return m_world.translate.z;
}

void TransformHierarchy::AddWorldTranslate(const Vec3& _translate)
{
    m_world.translate += _translate;
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldTranslateX(const float _x)
{
    m_world.translate.x += _x;
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldTranslateY(const float _y)
{
    m_world.translate.y += _y;
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldTranslateZ(const float _z)
{
    m_world.translate.z += _z;
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldRotate(const Quat& _rotate)
{
    m_world.rotate = _rotate;
    m_dirtyFlag    = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldRotate(const Vec3& _axis, const float _angleRad)
{
    m_world.rotate = Quat::CreateFromAxisAngle(_axis, _angleRad);
    m_dirtyFlag    = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldRotateEuler(const Vec3& _eulerRad)
{
    m_world.rotate = Quat::CreateFromYawPitchRoll(_eulerRad);
    m_dirtyFlag    = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldRotateYaw(const float _yawRad)
{
    Vec3 euler     = m_world.rotate.ToEuler();
    euler.y        = _yawRad;
    m_world.rotate = Quat::CreateFromYawPitchRoll(euler);
    m_dirtyFlag    = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldRotatePitch(const float _pitchRad)
{
    Vec3 euler     = m_world.rotate.ToEuler();
    euler.x        = _pitchRad;
    m_world.rotate = Quat::CreateFromYawPitchRoll(euler);
    m_dirtyFlag    = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldRotateRoll(const float _rollRad)
{
    Vec3 euler     = m_world.rotate.ToEuler();
    euler.z        = _rollRad;
    m_world.rotate = Quat::CreateFromYawPitchRoll(euler);
    m_dirtyFlag    = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

Quat TransformHierarchy::GetWorldRotate() const
{
    return m_world.rotate;
}

AxisAngle TransformHierarchy::GetWorldRotateAxisAngle() const
{
    return QuatToAxisAngle(m_world.rotate);
}

Vec3 TransformHierarchy::GetWorldRotateEuler() const
{
    return m_world.rotate.ToEuler();
}

float TransformHierarchy::GetWorldRotateYaw() const
{
    return m_world.rotate.ToEuler().y;
}

float TransformHierarchy::GetWorldRotatePitch() const
{
    return m_world.rotate.ToEuler().x;
}

float TransformHierarchy::GetWorldRotateRoll() const
{
    return m_world.rotate.ToEuler().z;
}

void TransformHierarchy::AddWorldRotate(const Quat& _rotate)
{
    m_world.rotate = m_world.rotate * _rotate;
    m_dirtyFlag    = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldRotate(const Vec3& _axis, const float _angleRad)
{
    Quat deltaRotation = Quat::CreateFromAxisAngle(_axis, _angleRad);
    m_world.rotate     = m_world.rotate * deltaRotation;
    m_dirtyFlag        = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldRotateEuler(const Vec3& _yawPitchRollRad)
{
    Quat deltaRotation = Quat::CreateFromYawPitchRoll(_yawPitchRollRad);
    m_world.rotate     = m_world.rotate * deltaRotation;
    m_dirtyFlag        = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldRotateYaw(const float _yawRad)
{
    Quat deltaRotation = Quat::CreateFromAxisAngle(Vec3::Up, _yawRad);
    m_world.rotate     = m_world.rotate * deltaRotation;
    m_dirtyFlag        = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldRotatePitch(const float _pitchRad)
{
    Quat deltaRotation = Quat::CreateFromAxisAngle(Vec3::Right, _pitchRad);
    m_world.rotate     = m_world.rotate * deltaRotation;
    m_dirtyFlag        = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldRotateRoll(const float _rollRad)
{
    Quat deltaRotation = Quat::CreateFromAxisAngle(Vec3::Forward, _rollRad);
    m_world.rotate     = m_world.rotate * deltaRotation;
    m_dirtyFlag        = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldScale(const Vec3& _scale)
{
    m_world.scale = _scale;
    m_dirtyFlag   = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldScaleX(const float _x)
{
    m_world.scale.x = _x;
    m_dirtyFlag     = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldScaleY(const float _y)
{
    m_world.scale.y = _y;
    m_dirtyFlag     = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldScaleZ(const float _z)
{
    m_world.scale.z = _z;
    m_dirtyFlag     = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

Vec3 TransformHierarchy::GetWorldScale() const
{
    return m_world.scale;
}

void TransformHierarchy::AddWorldScale(const Vec3& _scale)
{
    m_world.scale += _scale;
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldScaleX(const float _x)
{
    m_world.scale.x += _x;
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldScaleY(const float _y)
{
    m_world.scale.y += _y;
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldScaleZ(const float _z)
{
    m_world.scale.z += _z;
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

Vec3 TransformHierarchy::GetWorldForward() const
{
    return m_world.Forward();
}

Vec3 TransformHierarchy::GetWorldBackward() const
{
    return m_world.Backward();
}

Vec3 TransformHierarchy::GetWorldRight() const
{
    return m_world.Right();
}

Vec3 TransformHierarchy::GetWorldLeft() const
{
    return m_world.Left();
}

Vec3 TransformHierarchy::GetWorldUp() const
{
    return m_world.Up();
}

Vec3 TransformHierarchy::GetWorldDown() const
{
    return m_world.Down();
}
}   // namespace jam