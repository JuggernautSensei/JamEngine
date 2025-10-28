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

void TransformHierarchy::SetParent(const Entity _parent, const bool _bKeepLocal)
{
    JAM_ASSERT(_parent.IsValid(), "Parent entity is invalid");
    JAM_ASSERT(_parent != m_owner, "Cannot set self as parent");
    JAM_ASSERT(IsRoot() == false, "root cannot has parent");

    if (m_parent == _parent)
        return;

    // 기존 계층 정리
    if (HasParent() && m_parent != m_root)
    {
        TransformHierarchy& parentT = m_parent.GetTransform();
        if (!m_prevSibling.IsValid())   // 내가 첫째
        {
            parentT.m_firstChild = m_nextSibling;   // 다음 형제(혹은 null)을 첫째로
        }
        else   // 내가 첫 번째 자식이 아님
        {
            TransformHierarchy& prevSiblingT = m_prevSibling.GetTransform();
            prevSiblingT.m_nextSibling       = m_nextSibling;   // 다음 형제(혹은 null)과 이전 형제를 연결
        }

        if (m_nextSibling.IsValid())   // 다음 형제가 null이 아니라면
        {
            // 다음 형제와 이전 형제(혹은 null)를 연결
            TransformHierarchy& nextSiblingT = m_nextSibling.GetTransform();
            nextSiblingT.m_prevSibling       = m_prevSibling;
        }

        m_parent      = m_root;
        m_nextSibling = Entity::s_null;
        m_prevSibling = Entity::s_null;
    }

    // 새로운 부모 설정
    m_parent                       = _parent;
    TransformHierarchy& newParentT = m_parent.GetTransform();

    // 내가 부모의 첫 째가 됨
    Entity oldFirstChild    = newParentT.m_firstChild;
    newParentT.m_firstChild = m_owner;
    m_nextSibling           = oldFirstChild;
    m_prevSibling           = Entity::s_null;
    if (oldFirstChild.IsValid())   // 기존 첫째가 null이 아니라면
    {
        TransformHierarchy& oldFirstCHild = oldFirstChild.GetTransform();
        oldFirstCHild.m_prevSibling       = m_owner;   // 나와 기존 첫째를 연결
    }

    if (_bKeepLocal)
        m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;   // 로컬 유지. 월드 재계산
    else
        m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;   // 월드 유지. 로컬 재계산

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
        if (HasParent())
        {
            TransformHierarchy& parentTh  = m_parent.GetTransform();
            const Mat4&         parentMat = parentTh.m_world.GetMatrix();

            m_local = m_world.Multiplied(parentMat.Invert());
        }
        else   // 루트노드
        {
            m_local = m_world;
        }
    }
    else if (m_dirtyFlag == eDirtyFlag::WorldNeedUpdate)
    {
        // 로컬을 기준으로 월드 업데이트
        if (HasParent())
        {
            TransformHierarchy& parentTh  = m_parent.GetTransform();
            const Mat4&         parentMat = parentTh.m_local.GetMatrix();

            m_world = m_local.Multiplied(parentMat);
        }
        else   // 루트노드
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
        return;

    TransformHierarchy& parentT = m_parent.GetTransform();
    if (parentT.m_dirtyFlag == eDirtyFlag::Noop)
    {
        parentT.m_dirtyFlag = eDirtyFlag::ChildDirty;
        parentT.NotifyDirtyToParent_();
    }

    // 이미 부모가 더티 플래그가 설정되어 있으므로 더 이상 전파할 필요 없음
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

////////// LOCAL TRANSFORM GETTERS //////////

Mat4 TransformHierarchy::GetLocalMatrix() const
{
    return m_local.GetMatrix();
}

Mat4 TransformHierarchy::GetLocalView() const
{
    return m_local.GetViewMatrix();
}

Vec3 TransformHierarchy::GetLocalRight() const
{
    return m_local.GetRight();
}

Vec3 TransformHierarchy::GetLocalLeft() const
{
    return m_local.GetLeft();
}

Vec3 TransformHierarchy::GetLocalUp() const
{
    return m_local.GetUp();
}

Vec3 TransformHierarchy::GetLocalDown() const
{
    return m_local.GetDown();
}

Vec3 TransformHierarchy::GetLocalForward() const
{
    return m_local.GetForward();
}

Vec3 TransformHierarchy::GetLocalBackward() const
{
    return m_local.GetBackward();
}

Vec3 TransformHierarchy::GetLocalTranslation() const
{
    return m_local.GetTranslation();
}

float TransformHierarchy::GetLocalTranslationX() const
{
    return m_local.GetTranslationX();
}

float TransformHierarchy::GetLocalTranslationY() const
{
    return m_local.GetTranslationY();
}

float TransformHierarchy::GetLocalTranslationZ() const
{
    return m_local.GetTranslationZ();
}

Quat TransformHierarchy::GetLocalQuaternion() const
{
    return m_local.GetQuaternion();
}

AxisAngle TransformHierarchy::GetLocalAxisAngle() const
{
    return m_local.GetAxisAngle();
}

Vec3 TransformHierarchy::GetLocalPitchYawRoll() const
{
    return m_local.GetPitchYawRoll();
}

Vec3 TransformHierarchy::GetLocalScale() const
{
    return m_local.GetScale();
}

float TransformHierarchy::GetLocalScaleX() const
{
    return m_local.GetScaleX();
}

float TransformHierarchy::GetLocalScaleY() const
{
    return m_local.GetScaleY();
}

float TransformHierarchy::GetLocalScaleZ() const
{
    return m_local.GetScaleZ();
}

////////// LOCAL TRANSFORM SETTERS //////////

void TransformHierarchy::SetLocalMatrix(const Mat4& _mat)
{
    m_local.SetMatrix(_mat);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalTranslation(const Vec3& _translation)
{
    m_local.SetTranslation(_translation);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalTranslation(const float _x, const float _y, const float _z)
{
    m_local.SetTranslation(_x, _y, _z);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalTranslationX(const float _x)
{
    m_local.SetTranslationX(_x);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalTranslationY(const float _y)
{
    m_local.SetTranslationY(_y);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalTranslationZ(const float _z)
{
    m_local.SetTranslationZ(_z);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalRotation(const Quat& _rotation)
{
    m_local.SetRotation(_rotation);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalRotation(const Vec3& _axis, const float _angleRad)
{
    m_local.SetRotation(_axis, _angleRad);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalRotation(const Vec3& _pitchYawRollRad)
{
    m_local.SetRotation(_pitchYawRollRad);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalRotation(const float _pitchRad, const float _yawRad, const float _rollRad)
{
    m_local.SetRotation(_pitchRad, _yawRad, _rollRad);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalLookAt(const Vec3& _point)
{
    m_local.SetLookAt(_point);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalLookTo(const Vec3& _lookDir)
{
    m_local.SetLookTo(_lookDir);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalScale(const Vec3& _scale)
{
    m_local.SetScale(_scale);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalScale(const float _x, const float _y, const float _z)
{
    m_local.SetScale(_x, _y, _z);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalScale(const float _uniform)
{
    m_local.SetScale(_uniform);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalScaleX(const float _x)
{
    m_local.SetScaleX(_x);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalScaleY(const float _y)
{
    m_local.SetScaleY(_y);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalScaleZ(const float _z)
{
    m_local.SetScaleZ(_z);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

////////// LOCAL TRANSFORM ADDERS //////////

void TransformHierarchy::AddLocalTranslation(const Vec3& _translation)
{
    m_local.AddTranslation(_translation);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalTranslation(const float _x, const float _y, const float _z)
{
    m_local.AddTranslation(_x, _y, _z);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalTranslationX(const float _x)
{
    m_local.AddTranslationX(_x);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalTranslationY(const float _y)
{
    m_local.AddTranslationY(_y);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalTranslationZ(const float _z)
{
    m_local.AddTranslationZ(_z);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalRotation(const Quat& _rotation)
{
    m_local.AddRotation(_rotation);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalRotation(const Vec3& _axis, const float _angleRad)
{
    m_local.AddRotation(_axis, _angleRad);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalScale(const Vec3& _scale)
{
    m_local.AddScale(_scale);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalScale(const float _x, const float _y, const float _z)
{
    m_local.AddScale(_x, _y, _z);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalScale(const float _uniform)
{
    m_local.AddScale(_uniform);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalScaleX(const float _x)
{
    m_local.AddScaleX(_x);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalScaleY(const float _y)
{
    m_local.AddScaleY(_y);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalScaleZ(const float _z)
{
    m_local.AddScaleZ(_z);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

////////// WORLD TRANSFORM GETTERS //////////

Mat4 TransformHierarchy::GetWorldMatrix() const
{
    return m_world.GetMatrix();
}

Mat4 TransformHierarchy::GetWorldView() const
{
    return m_world.GetViewMatrix();
}

Vec3 TransformHierarchy::GetWorldRight() const
{
    return m_world.GetRight();
}

Vec3 TransformHierarchy::GetWorldLeft() const
{
    return m_world.GetLeft();
}

Vec3 TransformHierarchy::GetWorldUp() const
{
    return m_world.GetUp();
}

Vec3 TransformHierarchy::GetWorldDown() const
{
    return m_world.GetDown();
}

Vec3 TransformHierarchy::GetWorldForward() const
{
    return m_world.GetForward();
}

Vec3 TransformHierarchy::GetWorldBackward() const
{
    return m_world.GetBackward();
}

Vec3 TransformHierarchy::GetWorldTranslation() const
{
    return m_world.GetTranslation();
}

float TransformHierarchy::GetWorldTranslationX() const
{
    return m_world.GetTranslationX();
}

float TransformHierarchy::GetWorldTranslationY() const
{
    return m_world.GetTranslationY();
}

float TransformHierarchy::GetWorldTranslationZ() const
{
    return m_world.GetTranslationZ();
}

Quat TransformHierarchy::GetWorldQuaternion() const
{
    return m_world.GetQuaternion();
}

AxisAngle TransformHierarchy::GetWorldAxisAngle() const
{
    return m_world.GetAxisAngle();
}

Vec3 TransformHierarchy::GetWorldPitchYawRoll() const
{
    return m_world.GetPitchYawRoll();
}

Vec3 TransformHierarchy::GetWorldScale() const
{
    return m_world.GetScale();
}

float TransformHierarchy::GetWorldScaleX() const
{
    return m_world.GetScaleX();
}

float TransformHierarchy::GetWorldScaleY() const
{
    return m_world.GetScaleY();
}

float TransformHierarchy::GetWorldScaleZ() const
{
    return m_world.GetScaleZ();
}

////////// WORLD TRANSFORM SETTERS //////////

void TransformHierarchy::SetWorldMatrix(const Mat4& _mat)
{
    m_world.SetMatrix(_mat);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldTranslation(const Vec3& _translation)
{
    m_world.SetTranslation(_translation);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldTranslation(const float _x, const float _y, const float _z)
{
    m_world.SetTranslation(_x, _y, _z);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldTranslationX(const float _x)
{
    m_world.SetTranslationX(_x);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldTranslationY(const float _y)
{
    m_world.SetTranslationY(_y);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldTranslationZ(const float _z)
{
    m_world.SetTranslationZ(_z);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldRotation(const Quat& _rotation)
{
    m_world.SetRotation(_rotation);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldRotation(const Vec3& _axis, const float _angleRad)
{
    m_world.SetRotation(_axis, _angleRad);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldRotation(const Vec3& _pitchYawRollRad)
{
    m_world.SetRotation(_pitchYawRollRad);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldRotation(const float _pitchRad, const float _yawRad, const float _rollRad)
{
    m_world.SetRotation(_pitchRad, _yawRad, _rollRad);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldLookAt(const Vec3& _point)
{
    m_world.SetLookAt(_point);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldLookTo(const Vec3& _lookDir)
{
    m_world.SetLookTo(_lookDir);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldScale(const Vec3& _scale)
{
    m_world.SetScale(_scale);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldScale(const float _x, const float _y, const float _z)
{
    m_world.SetScale(_x, _y, _z);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldScale(const float _uniform)
{
    m_world.SetScale(_uniform);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldScaleX(const float _x)
{
    m_world.SetScaleX(_x);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldScaleY(const float _y)
{
    m_world.SetScaleY(_y);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldScaleZ(const float _z)
{
    m_world.SetScaleZ(_z);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

////////// WORLD TRANSFORM ADDERS //////////

void TransformHierarchy::AddWorldTranslation(const Vec3& _translation)
{
    m_world.AddTranslation(_translation);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldTranslation(const float _x, const float _y, const float _z)
{
    m_world.AddTranslation(_x, _y, _z);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldTranslationX(const float _x)
{
    m_world.AddTranslationX(_x);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldTranslationY(const float _y)
{
    m_world.AddTranslationY(_y);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldTranslationZ(const float _z)
{
    m_world.AddTranslationZ(_z);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldRotation(const Quat& _rotation)
{
    m_world.AddRotation(_rotation);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldRotation(const Vec3& _axis, const float _angleRad)
{
    m_world.AddRotation(_axis, _angleRad);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldScale(const Vec3& _scale)
{
    m_world.AddScale(_scale);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldScale(const float _x, const float _y, const float _z)
{
    m_world.AddScale(_x, _y, _z);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldScale(const float _uniform)
{
    m_world.AddScale(_uniform);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldScaleX(const float _x)
{
    m_world.AddScaleX(_x);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldScaleY(const float _y)
{
    m_world.AddScaleY(_y);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldScaleZ(const float _z)
{
    m_world.AddScaleZ(_z);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

}   // namespace jam