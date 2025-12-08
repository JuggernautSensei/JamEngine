#include "pch.h"

#include "TransformHierarchy.h"

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
    , m_owner(_root)
{
    JAM_ASSERT(_root.IsValid(), "Root entity is invalid");
}

void TransformHierarchy::SetLocalTransform(const Transform& _local)
{
    m_local     = _local;
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

const Transform& TransformHierarchy::GetLocalTransform() const
{
    return m_local;
}

void TransformHierarchy::SetParent(const Entity _parent, const bool _bKeepLocal)
{
    JAM_ASSERT(_parent.IsValid(), "Parent entity is invalid");
    JAM_ASSERT(_parent != m_owner, "Cannot set self as parent");
    JAM_ASSERT(!IsRoot(), "root cannot has parent");
    JAM_ASSERT(!IsAncestorOf(_parent), "Cannot set descendant as parent");

    if (m_parent == _parent)
    {
        // same parent
        return;
    }

    // release existing parent
    ReleaseParent_();

    // set new parent
    m_parent                     = _parent;
    TransformHierarchy& parentTh = m_parent.GetTransformHierarchy();

    // new parent's first child becomes me
    Entity oldFirstChild  = parentTh.m_firstChild;
    parentTh.m_firstChild = m_owner;
    m_nextSibling         = oldFirstChild;
    m_prevSibling         = Entity::s_null;
    if (oldFirstChild)
    {
        // if there was an old first child, update its prev sibling to me
        TransformHierarchy& oldFChildTh = oldFirstChild.GetTransformHierarchy();
        oldFChildTh.m_prevSibling       = m_owner;
    }

    if (_bKeepLocal)
    {
        // keep local -> update world
        m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    }
    else
    {
        // keep world -> update local
        m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    }

    NotifyDirtyToParent_();
}

void TransformHierarchy::SetParentAsRoot(const bool _bKeepLocal)
{
    SetParent(m_root, _bKeepLocal);
}

void TransformHierarchy::MakeOrphan()
{
    ReleaseParent_();
}

void TransformHierarchy::ReleaseParent_()
{
    if (!HasParent())
    {
        // already no parent
        return;
    }

    // detach from parent
    TransformHierarchy& parentTh = m_parent.GetTransformHierarchy();
    if (m_prevSibling)
    {
        // there is a previous sibling
        TransformHierarchy& prevSiblingTh = m_prevSibling.GetTransformHierarchy();
        prevSiblingTh.m_nextSibling       = m_nextSibling;
    }
    else
    {
        // this is the first child
        parentTh.m_firstChild = m_nextSibling;
    }

    if (m_nextSibling)
    {
        // there is a next sibling
        TransformHierarchy& nextSiblingT = m_nextSibling.GetTransformHierarchy();
        nextSiblingT.m_prevSibling       = m_prevSibling;
    }

    // clear links
    m_parent      = Entity::s_null;
    m_nextSibling = Entity::s_null;
    m_prevSibling = Entity::s_null;
}

bool TransformHierarchy::IsDescendantOf(const Entity _entity) const
{
    JAM_ASSERT(_entity, "Entity is invalid");

    Entity parent = m_parent;
    while (parent)
    {
        if (_entity == parent)
        {
            return true;   // 조상임
        }

        TransformHierarchy& parentTh = parent.GetTransformHierarchy();
        parent                       = parentTh.m_parent;
    }
    return false;
}

bool TransformHierarchy::IsAncestorOf(Entity _entity) const
{
    JAM_ASSERT(_entity, "Entity is invalid");

    TransformHierarchy& entityTh = _entity.GetTransformHierarchy();
    return entityTh.IsDescendantOf(m_owner);
}

bool TransformHierarchy::IsRoot() const
{
    return m_owner == m_root;
}

bool TransformHierarchy::HasParent() const
{
    return m_parent.IsValid();
}

Entity TransformHierarchy::GetRoot() const
{
    return m_root;
}

Entity TransformHierarchy::GetOwner() const
{
    return m_owner;
}

Entity TransformHierarchy::GetParent() const
{
    return m_parent;
}

Entity TransformHierarchy::GetFirstChild() const
{
    return m_firstChild;
}

Entity TransformHierarchy::GetNextSibling() const
{
    return m_nextSibling;
}

Entity TransformHierarchy::GetPrevSibling() const
{
    return m_prevSibling;
}

void TransformHierarchy::UpdateDirty_Recursive()
{
    if (m_dirtyFlag == eDirtyFlag::Noop)
    {
        // no dirty flags
        return;
    }

    if (m_dirtyFlag == eDirtyFlag::LocalNeedUpdate)
    {
        if (HasParent())
        {
            // update local based on world
            TransformHierarchy& parentTh     = m_parent.GetTransformHierarchy();
            const Mat4&         parentInvMtx = parentTh.GetWorldInvertMatrix();

            Transform newLocal = m_world;
            newLocal.MultMatrix(parentInvMtx);
            m_local = newLocal;
        }
        else
        {
            // root
            m_local = m_world;
        }
    }
    else if (m_dirtyFlag == eDirtyFlag::WorldNeedUpdate)
    {
        if (HasParent())
        {
            // update world based on local
            TransformHierarchy& parentTh  = m_parent.GetTransformHierarchy();
            const Mat4&         parentMtx = parentTh.GetWorldMatrix();

            Transform newWorld = m_local;
            newWorld.MultMatrix(parentMtx);
            m_world = newWorld;
        }
        else
        {
            // root
            m_world = m_local;
        }
    }

    for (Entity child = m_firstChild; child;)
    {

        // 부모의 변환이 직접적으로 변경된 경우 자식은 월드 업데이트 필요
        // 단, 자식의 월드가 변경되었을 경우 제외
        TransformHierarchy& childTh = child.GetTransformHierarchy();
        if (m_dirtyFlag != eDirtyFlag::ChildDirty && childTh.m_dirtyFlag != eDirtyFlag::LocalNeedUpdate)
        {
            childTh.m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
        }

        // 자식들에게 더티 전파 및 업데이트 재귀 호출
        childTh.UpdateDirty_Recursive();
        child = childTh.m_nextSibling;
    }

    m_dirtyFlag = eDirtyFlag::Noop;   // 더티 플래그 초기화
}

void TransformHierarchy::NotifyDirtyToParent_()
{
    if (!m_parent.IsValid())   // 루트 노드
        return;

    TransformHierarchy& parentT = m_parent.GetTransformHierarchy();
    if (parentT.m_dirtyFlag == eDirtyFlag::Noop)
    {
        parentT.m_dirtyFlag = eDirtyFlag::ChildDirty;
        parentT.NotifyDirtyToParent_();
    }

    // 이미 부모가 더티 플래그가 설정되어 있으므로 더 이상 전파할 필요 없음
}

void TransformHierarchy::SetWorldTransform(const Transform& _world)
{
    m_world     = _world;
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

const Transform& TransformHierarchy::GetWorldTransform() const
{
    return m_world;
}

////////// LOCAL TRANSFORM GETTERS //////////

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

const Mat4& TransformHierarchy::GetLocalMatrix() const
{
    return m_local.GetMatrix();
}

Mat4 TransformHierarchy::GetLocalInvertMatrix() const
{
    return m_local.GetInvertMatrix();
}

Mat4 TransformHierarchy::GetLocalViewMatrix() const
{
    return m_local.GetViewMatrix();
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

void TransformHierarchy::SetLocalMatrix(const Mat4& _mtx)
{
    m_local.SetMatrix(_mtx);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalTranslation(const Vec3 _translation)
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

void TransformHierarchy::SetLocalRotation(const Quat _rotation)
{
    m_local.SetRotation(_rotation);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalRotation(const Vec3 _axis, const float _angleRad)
{
    m_local.SetRotation(_axis, _angleRad);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalRotation(const Vec3 _pitchYawRollRad)
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

void TransformHierarchy::SetLocalLookAt(const Vec3 _point)
{
    m_local.SetLookAt(_point);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalLookTo(const Vec3 _lookDir)
{
    m_local.SetLookTo(_lookDir);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetLocalScale(const Vec3 _scale)
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

void TransformHierarchy::MultLocalMatrix(const Mat4& _mtx)
{
    m_local.MultMatrix(_mtx);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

////////// LOCAL TRANSFORM ADDERS //////////

void TransformHierarchy::AddLocalTranslation(const Vec3 _translation)
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

void TransformHierarchy::AddLocalRotation(const Quat _rotation)
{
    m_local.AddRotation(_rotation);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalRotation(const Vec3 _axis, const float _angleRad)
{
    m_local.AddRotation(_axis, _angleRad);
    m_dirtyFlag = eDirtyFlag::WorldNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddLocalScale(const Vec3 _scale)
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

const Mat4& TransformHierarchy::GetWorldMatrix() const
{
    return m_world.GetMatrix();
}

Mat4 TransformHierarchy::GetWorldInvertMatrix() const
{
    return m_world.GetInvertMatrix();
}

Mat4 TransformHierarchy::GetWorldViewMatrix() const
{
    return m_world.GetViewMatrix();
}

////////// WORLD TRANSFORM GETTERS //////////

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

void TransformHierarchy::SetWorldMatrix(const Mat4& _mtx)
{
    m_world.SetMatrix(_mtx);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldTranslation(const Vec3 _translation)
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

void TransformHierarchy::SetWorldRotation(const Quat _rotation)
{
    m_world.SetRotation(_rotation);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldRotation(const Vec3 _axis, const float _angleRad)
{
    m_world.SetRotation(_axis, _angleRad);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldRotation(const Vec3 _pitchYawRollRad)
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

void TransformHierarchy::SetWorldLookAt(const Vec3 _point)
{
    m_world.SetLookAt(_point);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldLookTo(const Vec3 _lookDir)
{
    m_world.SetLookTo(_lookDir);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::SetWorldScale(const Vec3 _scale)
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

void TransformHierarchy::MultWorldMatrix(const Mat4& _mtx)
{
    m_world.MultMatrix(_mtx);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

////////// WORLD TRANSFORM ADDERS //////////

void TransformHierarchy::AddWorldTranslation(const Vec3 _translation)
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

void TransformHierarchy::AddWorldRotation(const Quat _rotation)
{
    m_world.AddRotation(_rotation);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldRotation(const Vec3 _axis, const float _angleRad)
{
    m_world.AddRotation(_axis, _angleRad);
    m_dirtyFlag = eDirtyFlag::LocalNeedUpdate;
    NotifyDirtyToParent_();
}

void TransformHierarchy::AddWorldScale(const Vec3 _scale)
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