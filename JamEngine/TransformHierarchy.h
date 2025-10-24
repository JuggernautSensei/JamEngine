#pragma once
#include "Entity.h"
#include "Transform.h"

namespace jam
{

class TransformHierarchy
{
    enum class eDirtyFlag : Int8
    {
        Noop,
        WorldNeedUpdate,
        LocalNeedUpdate,
        ChildDirty
    };

public:
    TransformHierarchy() = default;                    // null constructor
    TransformHierarchy(Entity _root, Entity _owner);   // create regular node
    explicit TransformHierarchy(Entity _root);         // create root node

    ////////// LOCAL //////////

    void                           SetLocal(const Transform& _local);
    JAM_NODISCARD const Transform& GetLocal() const;

    // local translate

    void SetLocalTranslate(const Vec3& _translate);
    void SetLocalTranslateX(float _x);
    void SetLocalTranslateY(float _y);
    void SetLocalTranslateZ(float _z);

    JAM_NODISCARD Vec3  GetLocalTranslate() const;
    JAM_NODISCARD float GetLocalTranslateX() const;
    JAM_NODISCARD float GetLocalTranslateY() const;
    JAM_NODISCARD float GetLocalTranslateZ() const;

    void AddLocalTranslate(const Vec3& _translate);
    void AddLocalTranslateX(float _x);
    void AddLocalTranslateY(float _y);
    void AddLocalTranslateZ(float _z);

    // local rotate

    void SetLocalRotate(const Quat& _rotate);
    void SetLocalRotate(const Vec3& _axis, float _angleRad);
    void SetLocalRotateEuler(const Vec3& _eulerRad);
    void SetLocalRotateYaw(float _yawRad);
    void SetLocalRotatePitch(float _pitchRad);
    void SetLocalRotateRoll(float _rollRad);

    JAM_NODISCARD Quat      GetLocalRotate() const;
    JAM_NODISCARD AxisAngle GetLocalRotateAxisAngle() const;
    JAM_NODISCARD Vec3      GetLocalRotateEuler() const;
    JAM_NODISCARD float     GetLocalRotateYaw() const;
    JAM_NODISCARD float     GetLocalRotatePitch() const;
    JAM_NODISCARD float     GetLocalRotateRoll() const;

    void AddLocalRotate(const Quat& _rotate);
    void AddLocalRotate(const Vec3& _axis, float _angleRad);
    void AddLocalRotateEuler(const Vec3& _yawPitchRollRad);
    void AddLocalRotateYaw(float _yawRad);
    void AddLocalRotatePitch(float _pitchRad);
    void AddLocalRotateRoll(float _rollRad);

    // local scale

    void SetLocalScale(const Vec3& _scale);
    void SetLocalScaleX(float _x);
    void SetLocalScaleY(float _y);
    void SetLocalScaleZ(float _z);

    JAM_NODISCARD Vec3 GetLocalScale() const;

    void AddLocalScale(const Vec3& _scale);
    void AddLocalScaleX(float _x);
    void AddLocalScaleY(float _y);
    void AddLocalScaleZ(float _z);

    ////////// WORLD //////////

    void                           SetWorld(const Transform& _world);
    JAM_NODISCARD const Transform& GetWorld() const;

    // world translate

    void SetWorldTranslate(const Vec3& _translate);
    void SetWorldTranslateX(float _x);
    void SetWorldTranslateY(float _y);
    void SetWorldTranslateZ(float _z);

    JAM_NODISCARD Vec3  GetWorldTranslate() const;
    JAM_NODISCARD float GetWorldTranslateX() const;
    JAM_NODISCARD float GetWorldTranslateY() const;
    JAM_NODISCARD float GetWorldTranslateZ() const;

    void AddWorldTranslate(const Vec3& _translate);
    void AddWorldTranslateX(float _x);
    void AddWorldTranslateY(float _y);
    void AddWorldTranslateZ(float _z);

    // world rotate

    void SetWorldRotate(const Quat& _rotate);
    void SetWorldRotate(const Vec3& _axis, float _angleRad);
    void SetWorldRotateEuler(const Vec3& _eulerRad);
    void SetWorldRotateYaw(float _yawRad);
    void SetWorldRotatePitch(float _pitchRad);
    void SetWorldRotateRoll(float _rollRad);

    JAM_NODISCARD Quat      GetWorldRotate() const;
    JAM_NODISCARD AxisAngle GetWorldRotateAxisAngle() const;
    JAM_NODISCARD Vec3      GetWorldRotateEuler() const;
    JAM_NODISCARD float     GetWorldRotateYaw() const;
    JAM_NODISCARD float     GetWorldRotatePitch() const;
    JAM_NODISCARD float     GetWorldRotateRoll() const;

    void AddWorldRotate(const Quat& _rotate);
    void AddWorldRotate(const Vec3& _axis, float _angleRad);
    void AddWorldRotateEuler(const Vec3& _yawPitchRollRad);
    void AddWorldRotateYaw(float _yawRad);
    void AddWorldRotatePitch(float _pitchRad);
    void AddWorldRotateRoll(float _rollRad);

    // world scale

    void SetWorldScale(const Vec3& _scale);
    void SetWorldScaleX(float _x);
    void SetWorldScaleY(float _y);
    void SetWorldScaleZ(float _z);

    JAM_NODISCARD Vec3 GetWorldScale() const;

    void AddWorldScale(const Vec3& _scale);
    void AddWorldScaleX(float _x);
    void AddWorldScaleY(float _y);
    void AddWorldScaleZ(float _z);

    ////////// HIERARCHY //////////

    void SetParent(Entity _parent);
    void ReleaseParent();

    JAM_NODISCARD bool IsRoot() const { return m_owner == m_root; }
    JAM_NODISCARD bool HasParent() const { return m_parent.IsValid(); }

    JAM_NODISCARD Entity GetRoot() const { return m_root; }
    JAM_NODISCARD Entity GetOwner() const { return m_owner; }
    JAM_NODISCARD Entity GetParent() const { return m_parent; }
    JAM_NODISCARD Entity GetFirstChild() const { return m_firstChild; }
    JAM_NODISCARD Entity GetNextSibling() const { return m_nextSibling; }
    JAM_NODISCARD Entity GetPrevSibling() const { return m_prevSibling; }

    /// 최상위 노드가 매프레임 딱 한 번 호출할 것
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
