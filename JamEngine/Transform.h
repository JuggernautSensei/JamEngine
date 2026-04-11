#pragma once

namespace Jam
{

struct AxisAngle
{
    Vec3  axis = Numbers<Vec3>::k_zero;
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
    [[nodiscard]] const Mtx4& GetMatrix() const;
    void                      SetMatrix(const Mtx4& _mtx);

    // transform
    [[nodiscard]] Transform Inverted() const;
    void                    Invert();

    [[nodiscard]] Transform Multed(const Transform& _trans) const;
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
    [[nodiscard]] Quat      GetRotation() const;
    [[nodiscard]] AxisAngle ToAxisAngle() const;
    [[nodiscard]] Vec3      ToEuler() const;   // pitch yaw roll

    void SetRotation(Quat _q);
    void SetRotation(Vec3 _axis, float _rad);
    void SetRotation(Vec3 _eulerRad);
    void SetRotation(float _pitchRad, float _yawRad, float _rollRad);
    void SetLookAt(Vec3 _point);
    void SetLookTo(Vec3 _forward);

    void AddRotation(Quat _q);
    void AddRotation(Vec3 _axis, float _rad);
    void AddRotation(Vec3 _eulerRad);
    void AddRotation(float _pitchRad, float _yawRad, float _rollRad);
    void AddRotationPitch(float _rad);
    void AddRotationYaw(float _rad);
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

    [[nodiscard]] bool IsDirty() const;

private:
    Vec3 m_translation = Numbers<Vec3>::k_zero;
    Quat m_rotation    = Numbers<Quat>::k_identity;
    Vec3 m_scale       = Numbers<Vec3>::k_one;

    mutable bool m_bDirty   = true;
    mutable Mtx4 m_mtxCache = Numbers<Mtx4>::k_identity;
};

}   // namespace Jam