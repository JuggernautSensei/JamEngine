#pragma once
#include "MathUtilities.h"

namespace jam
{

class Transform
{
public:
    Transform() = default;
    explicit Transform(const Mat4& _mtx);

    [[nodiscard]] const Mat4& GetMatrix() const;
    [[nodiscard]] Mat4        GetViewMatrix() const;
    [[nodiscard]] Mat4        GetInvertMatrix() const;   // this is not strict inverse matrix. only for transform invert
    [[nodiscard]] Transform   GetInvert() const;         // this is not strict inverse matrix. only for transform invert

    [[nodiscard]] Vec3 GetRight() const;
    [[nodiscard]] Vec3 GetLeft() const;
    [[nodiscard]] Vec3 GetUp() const;
    [[nodiscard]] Vec3 GetDown() const;
    [[nodiscard]] Vec3 GetForward() const;
    [[nodiscard]] Vec3 GetBackward() const;

    [[nodiscard]] Mat4 GetRotationMatrix() const;
    [[nodiscard]] Mat4 GetScaleMatrix() const;
    [[nodiscard]] Mat4 GetTranslationMatrix() const;

    [[nodiscard]] Vec3  GetTranslation() const;
    [[nodiscard]] float GetTranslationX() const;
    [[nodiscard]] float GetTranslationY() const;
    [[nodiscard]] float GetTranslationZ() const;

    [[nodiscard]] Quat      GetQuaternion() const;
    [[nodiscard]] AxisAngle GetAxisAngle() const;
    [[nodiscard]] Vec3      GetPitchYawRoll() const;

    [[nodiscard]] Vec3  GetScale() const;
    [[nodiscard]] float GetScaleX() const;
    [[nodiscard]] float GetScaleY() const;
    [[nodiscard]] float GetScaleZ() const;

    // Matrix Operations
    Transform& SetMatrix(const Mat4& _mtx);
    Transform& MultMatrix(const Mat4& _mtx);

    // Translation Setters
    Transform& SetTranslation(Vec3 _translation);
    Transform& SetTranslation(float _x, float _y, float _z);
    Transform& SetTranslationX(float _x);
    Transform& SetTranslationY(float _y);
    Transform& SetTranslationZ(float _z);

    // Translation Adders
    Transform& AddTranslation(Vec3 _translation);
    Transform& AddTranslation(float _x, float _y, float _z);
    Transform& AddTranslationX(float _x);
    Transform& AddTranslationY(float _y);
    Transform& AddTranslationZ(float _z);

    // Rotation Setters
    Transform& SetRotation(Quat _rotation);
    Transform& SetRotation(Vec3 _axis, float _angleRad);
    Transform& SetRotation(Vec3 _pitchYawRollRad);
    Transform& SetRotation(float _pitchRad, float _yawRad, float _rollRad);
    Transform& SetLookAt(Vec3 _point);
    Transform& SetLookTo(Vec3 _forward);

    // Rotation Adders
    Transform& AddRotation(Quat _q);
    Transform& AddRotation(Vec3 _axis, float _angleRad);

    // Scale Setters
    Transform& SetScale(Vec3 _scale);
    Transform& SetScale(float _x, float _y, float _z);
    Transform& SetScale(float _uniform);
    Transform& SetScaleX(float _x);
    Transform& SetScaleY(float _y);
    Transform& SetScaleZ(float _z);

    // Scale Adders
    Transform& AddScale(Vec3 _s);
    Transform& AddScale(float _x, float _y, float _z);
    Transform& AddScale(float _s);
    Transform& AddScaleX(float _x);
    Transform& AddScaleY(float _y);
    Transform& AddScaleZ(float _z);

private:
    Mat4 m_mtx = Mat4::Identity;
};

}   // namespace jam