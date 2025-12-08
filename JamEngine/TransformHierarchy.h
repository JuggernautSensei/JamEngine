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
    TransformHierarchy(Entity _root, Entity _owner);   // create regular pNode
    explicit TransformHierarchy(Entity _root);         // create root pNode

    // do not call this function directly
    void UpdateDirty_Recursive();

    // hierarchy operations
    void SetParent(Entity _parent, bool _bKeepLocal = false);
    void SetParentAsRoot(bool _bKeepLocal = false);
    void MakeOrphan();

    [[nodiscard]] bool IsDescendantOf(Entity _entity) const;   // check if this is a descendant of _entity
    [[nodiscard]] bool IsAncestorOf(Entity _entity) const;     // check if this is an ancestor of _entity

    [[nodiscard]] bool IsRoot() const;
    [[nodiscard]] bool HasParent() const;

    [[nodiscard]] Entity GetRoot() const;
    [[nodiscard]] Entity GetOwner() const;
    [[nodiscard]] Entity GetParent() const;
    [[nodiscard]] Entity GetFirstChild() const;
    [[nodiscard]] Entity GetNextSibling() const;
    [[nodiscard]] Entity GetPrevSibling() const;

    // local transform getter
    [[nodiscard]] const Transform& GetLocalTransform() const;
    [[nodiscard]] const Mat4&      GetLocalMatrix() const;
    [[nodiscard]] Mat4             GetLocalInvertMatrix() const;
    [[nodiscard]] Mat4             GetLocalViewMatrix() const;

    [[nodiscard]] Vec3 GetLocalRight() const;
    [[nodiscard]] Vec3 GetLocalLeft() const;
    [[nodiscard]] Vec3 GetLocalUp() const;
    [[nodiscard]] Vec3 GetLocalDown() const;
    [[nodiscard]] Vec3 GetLocalForward() const;
    [[nodiscard]] Vec3 GetLocalBackward() const;

    [[nodiscard]] Vec3  GetLocalTranslation() const;
    [[nodiscard]] float GetLocalTranslationX() const;
    [[nodiscard]] float GetLocalTranslationY() const;
    [[nodiscard]] float GetLocalTranslationZ() const;

    [[nodiscard]] Quat      GetLocalQuaternion() const;
    [[nodiscard]] AxisAngle GetLocalAxisAngle() const;
    [[nodiscard]] Vec3      GetLocalPitchYawRoll() const;

    [[nodiscard]] Vec3  GetLocalScale() const;
    [[nodiscard]] float GetLocalScaleX() const;
    [[nodiscard]] float GetLocalScaleY() const;
    [[nodiscard]] float GetLocalScaleZ() const;

    void SetLocalTransform(const Transform& _local);
    void SetLocalMatrix(const Mat4& _mtx);

    void SetLocalTranslation(Vec3 _translation);
    void SetLocalTranslation(float _x, float _y, float _z);
    void SetLocalTranslationX(float _x);
    void SetLocalTranslationY(float _y);
    void SetLocalTranslationZ(float _z);

    void SetLocalRotation(Quat _rotation);
    void SetLocalRotation(Vec3 _axis, float _angleRad);
    void SetLocalRotation(Vec3 _pitchYawRollRad);
    void SetLocalRotation(float _pitchRad, float _yawRad, float _rollRad);
    void SetLocalLookAt(Vec3 _point);
    void SetLocalLookTo(Vec3 _lookDir);

    void SetLocalScale(Vec3 _scale);
    void SetLocalScale(float _x, float _y, float _z);
    void SetLocalScale(float _uniform);
    void SetLocalScaleX(float _x);
    void SetLocalScaleY(float _y);
    void SetLocalScaleZ(float _z);

    void MultLocalMatrix(const Mat4& _mtx);

    void AddLocalTranslation(Vec3 _translation);
    void AddLocalTranslation(float _x, float _y, float _z);
    void AddLocalTranslationX(float _x);
    void AddLocalTranslationY(float _y);
    void AddLocalTranslationZ(float _z);

    void AddLocalRotation(Quat _rotation);
    void AddLocalRotation(Vec3 _axis, float _angleRad);

    void AddLocalScale(Vec3 _scale);
    void AddLocalScale(float _x, float _y, float _z);
    void AddLocalScale(float _uniform);
    void AddLocalScaleX(float _x);
    void AddLocalScaleY(float _y);
    void AddLocalScaleZ(float _z);

    [[nodiscard]] const Transform& GetWorldTransform() const;
    [[nodiscard]] const Mat4&      GetWorldMatrix() const;
    [[nodiscard]] Mat4             GetWorldInvertMatrix() const;
    [[nodiscard]] Mat4             GetWorldViewMatrix() const;

    [[nodiscard]] Vec3 GetWorldRight() const;
    [[nodiscard]] Vec3 GetWorldLeft() const;
    [[nodiscard]] Vec3 GetWorldUp() const;
    [[nodiscard]] Vec3 GetWorldDown() const;
    [[nodiscard]] Vec3 GetWorldForward() const;
    [[nodiscard]] Vec3 GetWorldBackward() const;

    [[nodiscard]] Vec3  GetWorldTranslation() const;
    [[nodiscard]] float GetWorldTranslationX() const;
    [[nodiscard]] float GetWorldTranslationY() const;
    [[nodiscard]] float GetWorldTranslationZ() const;

    [[nodiscard]] Quat      GetWorldQuaternion() const;
    [[nodiscard]] AxisAngle GetWorldAxisAngle() const;
    [[nodiscard]] Vec3      GetWorldPitchYawRoll() const;

    [[nodiscard]] Vec3  GetWorldScale() const;
    [[nodiscard]] float GetWorldScaleX() const;
    [[nodiscard]] float GetWorldScaleY() const;
    [[nodiscard]] float GetWorldScaleZ() const;

    void SetWorldTransform(const Transform& _world);
    void SetWorldMatrix(const Mat4& _mtx);

    void SetWorldTranslation(Vec3 _translation);
    void SetWorldTranslation(float _x, float _y, float _z);
    void SetWorldTranslationX(float _x);
    void SetWorldTranslationY(float _y);
    void SetWorldTranslationZ(float _z);

    void SetWorldRotation(Quat _rotation);
    void SetWorldRotation(Vec3 _axis, float _angleRad);
    void SetWorldRotation(Vec3 _pitchYawRollRad);
    void SetWorldRotation(float _pitchRad, float _yawRad, float _rollRad);
    void SetWorldLookAt(Vec3 _point);
    void SetWorldLookTo(Vec3 _lookDir);

    void SetWorldScale(Vec3 _scale);
    void SetWorldScale(float _x, float _y, float _z);
    void SetWorldScale(float _uniform);
    void SetWorldScaleX(float _x);
    void SetWorldScaleY(float _y);
    void SetWorldScaleZ(float _z);

    ////////// WORLD TRANSFORM ADDERS //////////

    void MultWorldMatrix(const Mat4& _mtx);

    void AddWorldTranslation(Vec3 _translation);
    void AddWorldTranslation(float _x, float _y, float _z);
    void AddWorldTranslationX(float _x);
    void AddWorldTranslationY(float _y);
    void AddWorldTranslationZ(float _z);

    void AddWorldRotation(Quat _rotation);
    void AddWorldRotation(Vec3 _axis, float _angleRad);

    void AddWorldScale(Vec3 _scale);
    void AddWorldScale(float _x, float _y, float _z);
    void AddWorldScale(float _uniform);
    void AddWorldScaleX(float _x);
    void AddWorldScaleY(float _y);
    void AddWorldScaleZ(float _z);

private:
    void NotifyDirtyToParent_();
    void ReleaseParent_();

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