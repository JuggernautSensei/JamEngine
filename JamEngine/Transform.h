#pragma once
#include "Math.h"

namespace jam
{

struct AxisAngle
{
    Vec3  axis = Vec3::Zero;
    float rad  = 0.f;
};

class Transform
{
public:
    // axis
    [[nodiscard]] Vec3 GetRight() const;
    [[nodiscard]] Vec3 GetLeft() const;
    [[nodiscard]] Vec3 GetUp() const;
    [[nodiscard]] Vec3 GetDown() const;
    [[nodiscard]] Vec3 GetForward() const;
    [[nodiscard]] Vec3 GetBackward() const;

    // matrix
    [[nodiscard]] const Mat4& GetMatrix() const;
    void                      SetMatrix(const Mat4& _mtx);
    [[nodiscard]] Mat4        ToViewMatrix() const;

    // transform
    [[nodiscard]] Transform Inverted() const;
    void                    Invert();
    void                    Mult(const Transform& _trans);

    // scale
    [[nodiscard]] Vec3  GetScale() const;
    [[nodiscard]] float GetScaleX() const;
    [[nodiscard]] float GetScaleY() const;
    [[nodiscard]] float GetScaleZ() const;

    void SetScale(Vec3 _xyz);
    void SetScale(float _x, float _y, float _z);
    void SetScale(float _uniform);
    void SetScaleX(float _x);
    void SetScaleY(float _y);
    void SetScaleZ(float _z);

    void AddScale(Vec3 _xyz);
    void AddScale(float _x, float _y, float _z);
    void AddScale(float _uniform);
    void AddScaleX(float _x);
    void AddScaleY(float _y);
    void AddScaleZ(float _z);

    // rotation
    [[nodiscard]] Quat      ToQuat() const;
    [[nodiscard]] AxisAngle ToAxisAngle() const;
    [[nodiscard]] Vec3      ToPitchYawRoll() const;

    void SetRotation(Quat _q);
    void SetRotation(Vec3 _axis, float _rad);
    void SetRotation(Vec3 _pitchYawRollRad);
    void SetRotation(float _pitchRad, float _yawRad, float _rollRad);
    void SetLookAt(Vec3 _point);
    void SetLookTo(Vec3 _forward);

    void AddRotation(Quat _q);
    void AddRotation(Vec3 _axis, float _rad);
    void AddRotation(Vec3 _pitchYawRollRad);
    void AddRotation(float _pitchRad, float _yawRad, float _rollRad);
    void AddRotationYaw(float _rad);
    void AddRotationPitch(float _rad);
    void AddRotationRoll(float _rad);

    [[nodiscard]] Vec3  GetTranslation() const;
    [[nodiscard]] float GetTranslationX() const;
    [[nodiscard]] float GetTranslationY() const;
    [[nodiscard]] float GetTranslationZ() const;

    void SetTranslation(Vec3 _xyz);
    void SetTranslation(float _x, float _y, float _z);
    void SetTranslationX(float _x);
    void SetTranslationY(float _y);
    void SetTranslationZ(float _z);

    void AddTranslation(Vec3 _xyz);
    void AddTranslation(float _x, float _y, float _z);
    void AddTranslationX(float _x);
    void AddTranslationY(float _y);
    void AddTranslationZ(float _z);

private:
    void UpdateDirty_() const;

    Vec3 m_right   = Vec3::UnitX;
    Vec3 m_up      = Vec3::UnitY;
    Vec3 m_forward = Vec3::UnitZ;
    Vec3 m_origin  = Vec3::Zero;

    mutable bool m_bDirty   = false;
    mutable Mat4 m_mtxCache = Mat4::Identity;
};

}   // namespace jam