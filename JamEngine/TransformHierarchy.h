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

    JAM_NODISCARD const Transform& GetLocal() const;
    void                           SetLocal(const Transform& _local);

    ////////// WORLD //////////

    JAM_NODISCARD const Transform& GetWorld() const;
    void                           SetWorld(const Transform& _world);

    ////////// HIERARCHY //////////

    void SetParent(Entity _parent, bool _bKeepLocal = false);
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

    ////////// LOCAL TRANSFORM GETTERS //////////

    JAM_NODISCARD Mat4 GetLocalMatrix() const;
    JAM_NODISCARD Mat4 GetLocalView() const;

    JAM_NODISCARD Vec3 GetLocalRight() const;
    JAM_NODISCARD Vec3 GetLocalLeft() const;
    JAM_NODISCARD Vec3 GetLocalUp() const;
    JAM_NODISCARD Vec3 GetLocalDown() const;
    JAM_NODISCARD Vec3 GetLocalForward() const;
    JAM_NODISCARD Vec3 GetLocalBackward() const;

    JAM_NODISCARD Vec3  GetLocalTranslation() const;
    JAM_NODISCARD float GetLocalTranslationX() const;
    JAM_NODISCARD float GetLocalTranslationY() const;
    JAM_NODISCARD float GetLocalTranslationZ() const;

    JAM_NODISCARD Quat      GetLocalQuaternion() const;
    JAM_NODISCARD AxisAngle GetLocalAxisAngle() const;
    JAM_NODISCARD Vec3      GetLocalPitchYawRoll() const;

    JAM_NODISCARD Vec3  GetLocalScale() const;
    JAM_NODISCARD float GetLocalScaleX() const;
    JAM_NODISCARD float GetLocalScaleY() const;
    JAM_NODISCARD float GetLocalScaleZ() const;

    ////////// LOCAL TRANSFORM SETTERS //////////

    void SetLocalMatrix(const Mat4& _mat);

    void SetLocalTranslation(const Vec3& _translation);
    void SetLocalTranslation(float _x, float _y, float _z);
    void SetLocalTranslationX(float _x);
    void SetLocalTranslationY(float _y);
    void SetLocalTranslationZ(float _z);

    void SetLocalRotation(const Quat& _rotation);
    void SetLocalRotation(const Vec3& _axis, float _angleRad);
    void SetLocalRotation(const Vec3& _pitchYawRollRad);
    void SetLocalRotation(float _pitchRad, float _yawRad, float _rollRad);
    void SetLocalLookAt(const Vec3& _point);
    void SetLocalLookTo(const Vec3& _lookDir);

    void SetLocalScale(const Vec3& _scale);
    void SetLocalScale(float _x, float _y, float _z);
    void SetLocalScale(float _uniform);
    void SetLocalScaleX(float _x);
    void SetLocalScaleY(float _y);
    void SetLocalScaleZ(float _z);

    ////////// LOCAL TRANSFORM ADDERS //////////

    void AddLocalTranslation(const Vec3& _translation);
    void AddLocalTranslation(float _x, float _y, float _z);
    void AddLocalTranslationX(float _x);
    void AddLocalTranslationY(float _y);
    void AddLocalTranslationZ(float _z);

    void AddLocalRotation(const Quat& _rotation);
    void AddLocalRotation(const Vec3& _axis, float _angleRad);

    void AddLocalScale(const Vec3& _scale);
    void AddLocalScale(float _x, float _y, float _z);
    void AddLocalScale(float _uniform);
    void AddLocalScaleX(float _x);
    void AddLocalScaleY(float _y);
    void AddLocalScaleZ(float _z);

        ////////// WORLD TRANSFORM GETTERS //////////

    JAM_NODISCARD Mat4 GetWorldMatrix() const;
    JAM_NODISCARD Mat4 GetWorldView() const;

    JAM_NODISCARD Vec3 GetWorldRight() const;
    JAM_NODISCARD Vec3 GetWorldLeft() const;
    JAM_NODISCARD Vec3 GetWorldUp() const;
    JAM_NODISCARD Vec3 GetWorldDown() const;
    JAM_NODISCARD Vec3 GetWorldForward() const;
    JAM_NODISCARD Vec3 GetWorldBackward() const;

    JAM_NODISCARD Vec3  GetWorldTranslation() const;
    JAM_NODISCARD float GetWorldTranslationX() const;
    JAM_NODISCARD float GetWorldTranslationY() const;
    JAM_NODISCARD float GetWorldTranslationZ() const;

    JAM_NODISCARD Quat      GetWorldQuaternion() const;
    JAM_NODISCARD AxisAngle GetWorldAxisAngle() const;
    JAM_NODISCARD Vec3      GetWorldPitchYawRoll() const;

    JAM_NODISCARD Vec3  GetWorldScale() const;
    JAM_NODISCARD float GetWorldScaleX() const;
    JAM_NODISCARD float GetWorldScaleY() const;
    JAM_NODISCARD float GetWorldScaleZ() const;

    ////////// WORLD TRANSFORM SETTERS //////////

    void SetWorldMatrix(const Mat4& _mat);

    void SetWorldTranslation(const Vec3& _translation);
    void SetWorldTranslation(float _x, float _y, float _z);
    void SetWorldTranslationX(float _x);
    void SetWorldTranslationY(float _y);
    void SetWorldTranslationZ(float _z);

    void SetWorldRotation(const Quat& _rotation);
    void SetWorldRotation(const Vec3& _axis, float _angleRad);
    void SetWorldRotation(const Vec3& _pitchYawRollRad);
    void SetWorldRotation(float _pitchRad, float _yawRad, float _rollRad);
    void SetWorldLookAt(const Vec3& _point);
    void SetWorldLookTo(const Vec3& _lookDir);

    void SetWorldScale(const Vec3& _scale);
    void SetWorldScale(float _x, float _y, float _z);
    void SetWorldScale(float _uniform);
    void SetWorldScaleX(float _x);
    void SetWorldScaleY(float _y);
    void SetWorldScaleZ(float _z);

    ////////// WORLD TRANSFORM ADDERS //////////

    void AddWorldTranslation(const Vec3& _translation);
    void AddWorldTranslation(float _x, float _y, float _z);
    void AddWorldTranslationX(float _x);
    void AddWorldTranslationY(float _y);
    void AddWorldTranslationZ(float _z);

    void AddWorldRotation(const Quat& _rotation);
    void AddWorldRotation(const Vec3& _axis, float _angleRad);

    void AddWorldScale(const Vec3& _scale);
    void AddWorldScale(float _x, float _y, float _z);
    void AddWorldScale(float _uniform);
    void AddWorldScaleX(float _x);
    void AddWorldScaleY(float _y);
    void AddWorldScaleZ(float _z);

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