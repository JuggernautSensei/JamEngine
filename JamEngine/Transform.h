#pragma once

namespace Jam
{

struct AxisAngle
{
    Vector3  axis = Vec3::k_zero;
    float rad  = 0.f;
};

class Transform
{
public:
    // axis
    [[nodiscard]] Vector3 GetRight() const;
    [[nodiscard]] Vector3 GetLeft() const;
    [[nodiscard]] Vector3 GetUp() const;
    [[nodiscard]] Vector3 GetDown() const;
    [[nodiscard]] Vector3 GetForward() const;
    [[nodiscard]] Vector3 GetBackward() const;

    // matrix
    [[nodiscard]] const Matrix4x4& GetMatrix() const;
    void                      SetMatrix(const Matrix4x4& _mtx);
    [[nodiscard]] Matrix4x4        ToViewMatrix() const;

    // transform
    [[nodiscard]] Transform Inverted() const;
    void                    Invert();

    [[nodiscard]] Transform Multed(const Transform& _trans) const;
    void                    Mult(const Transform& _trans);

    // scale
    [[nodiscard]] Vector3  GetScale() const;
    [[nodiscard]] float GetScaleX() const;
    [[nodiscard]] float GetScaleY() const;
    [[nodiscard]] float GetScaleZ() const;

    void SetScale(Vector3 _xyz);
    void SetScale(float _x, float _y, float _z);
    void SetScale(float _uniform);
    void SetScaleX(float _x);
    void SetScaleY(float _y);
    void SetScaleZ(float _z);

    void AddScale(Vector3 _xyz);
    void AddScale(float _x, float _y, float _z);
    void AddScale(float _uniform);
    void AddScaleX(float _x);
    void AddScaleY(float _y);
    void AddScaleZ(float _z);

    // rotation
    [[nodiscard]] Quaternion      ToQuat() const;
    [[nodiscard]] AxisAngle ToAxisAngle() const;
    [[nodiscard]] Vector3      ToEuler() const;   // pitch yaw roll

    void SetRotation(Quaternion _q);
    void SetRotation(Vector3 _axis, float _rad);
    void SetRotation(Vector3 _eulerRad);
    void SetRotation(float _pitchRad, float _yawRad, float _rollRad);
    void SetLookAt(Vector3 _point);
    void SetLookTo(Vector3 _forward);

    void AddRotation(Quaternion _q);
    void AddRotation(Vector3 _axis, float _rad);
    void AddRotation(Vector3 _eulerRad);
    void AddRotation(float _pitchRad, float _yawRad, float _rollRad);
    void AddRotationPitch(float _rad);
    void AddRotationYaw(float _rad);
    void AddRotationRoll(float _rad);

    [[nodiscard]] Vector3  GetTranslation() const;
    [[nodiscard]] float GetTranslationX() const;
    [[nodiscard]] float GetTranslationY() const;
    [[nodiscard]] float GetTranslationZ() const;

    void SetTranslation(Vector3 _xyz);
    void SetTranslation(float _x, float _y, float _z);
    void SetTranslationX(float _x);
    void SetTranslationY(float _y);
    void SetTranslationZ(float _z);

    void AddTranslation(Vector3 _xyz);
    void AddTranslation(float _x, float _y, float _z);
    void AddTranslationX(float _x);
    void AddTranslationY(float _y);
    void AddTranslationZ(float _z);

private:
    Vector3 m_right   = Vec3::k_unitX;
    Vector3 m_up      = Vec3::k_unitY;
    Vector3 m_forward = Vec3::k_unitZ;
    Vector3 m_origin  = Vec3::k_zero;

    mutable bool m_bDirty   = false;
    mutable Matrix4x4 m_mtxCache = Mtx4::k_identity;
};

}   // namespace Jam