#pragma once
#include "MathUtilities.h"

namespace jam
{

class Transform
{
public:
    JAM_NODISCARD const Mat4& GetMatrix() const;
    JAM_NODISCARD Mat4        GetViewMatrix() const;

    JAM_NODISCARD Vec3 GetRight() const;
    JAM_NODISCARD Vec3 GetLeft() const;
    JAM_NODISCARD Vec3 GetUp() const;
    JAM_NODISCARD Vec3 GetDown() const;
    JAM_NODISCARD Vec3 GetForward() const;
    JAM_NODISCARD Vec3 GetBackward() const;

    JAM_NODISCARD Vec3  GetTranslation() const;
    JAM_NODISCARD float GetTranslationX() const;
    JAM_NODISCARD float GetTranslationY() const;
    JAM_NODISCARD float GetTranslationZ() const;

    JAM_NODISCARD Quat      GetQuaternion() const;
    JAM_NODISCARD AxisAngle GetAxisAngle() const;
    JAM_NODISCARD Vec3      GetPitchYawRoll() const;

    JAM_NODISCARD Vec3  GetScale() const;
    JAM_NODISCARD float GetScaleX() const;
    JAM_NODISCARD float GetScaleY() const;
    JAM_NODISCARD float GetScaleZ() const;

    // Matrix Operations
    Transform&              Multiply(const Mat4& _mat);
    JAM_NODISCARD Transform Multiplied(const Mat4& _mat) const;

    Transform& SetMatrix(const Mat4& _mat);

    // Translation Setters
    Transform& SetTranslation(const Vec3& _translation);
    Transform& SetTranslation(float _x, float _y, float _z);
    Transform& SetTranslationX(float _x);
    Transform& SetTranslationY(float _y);
    Transform& SetTranslationZ(float _z);

    // Translation Adders
    Transform& AddTranslation(const Vec3& _translation);
    Transform& AddTranslation(float _x, float _y, float _z);
    Transform& AddTranslationX(float _x);
    Transform& AddTranslationY(float _y);
    Transform& AddTranslationZ(float _z);

    // Rotation Setters
    Transform& SetRotation(const Quat& _rotation);
    Transform& SetRotation(const Vec3& _axis, float _angleRad);
    Transform& SetRotation(const Vec3& _pitchYawRollRad);
    Transform& SetRotation(float _pitchRad, float _yawRad, float _rollRad);
    Transform& SetLookAt(const Vec3& _point);
    Transform& SetLookTo(const Vec3& _lookDir);

    // Rotation Adders
    Transform& AddRotation(const Quat& _rotation);
    Transform& AddRotation(const Vec3& _axis, float _angleRad);

    // Scale Setters
    Transform& SetScale(const Vec3& _scale);
    Transform& SetScale(float _x, float _y, float _z);
    Transform& SetScale(float _uniform);
    Transform& SetScaleX(float _x);
    Transform& SetScaleY(float _y);
    Transform& SetScaleZ(float _z);

    // Scale Adders
    Transform& AddScale(const Vec3& _scale);
    Transform& AddScale(float _x, float _y, float _z);
    Transform& AddScale(float _uniform);
    Transform& AddScaleX(float _x);
    Transform& AddScaleY(float _y);
    Transform& AddScaleZ(float _z);

private:
    Mat4 m_mtx = Mat4::Identity;
};

}   // namespace jam