#pragma once

namespace Jam
{

class Transform
{
    enum class eMatrixDirtyFlags : UInt32
    {
        None   = 0,
        SRT    = 1 << 0,
        InvSRT = 1 << 1,
        All    = SRT | InvSRT
    };

public:
    Transform() = default;
    Transform(Vec3 _scale, Quat _rotation, Vec3 _translation);

    // axis
    [[nodiscard]] Vec3 GetRight() const;
    [[nodiscard]] Vec3 GetLeft() const;
    [[nodiscard]] Vec3 GetUp() const;
    [[nodiscard]] Vec3 GetDown() const;
    [[nodiscard]] Vec3 GetForward() const;
    [[nodiscard]] Vec3 GetBackward() const;

    // srt
    void                      SetSRT(const Mtx4& _srtMtx);
    [[nodiscard]] const Mtx4& GetSRT() const;
    [[nodiscard]] const Mtx4& GetInvSRT() const;

    // transform
    [[nodiscard]] Transform Inverse() const;
    [[nodiscard]] Transform Mult(const Transform& _trans) const;

    // scale
    [[nodiscard]] Vec3  GetScale() const;
    [[nodiscard]] float GetScaleX() const;
    [[nodiscard]] float GetScaleY() const;
    [[nodiscard]] float GetScaleZ() const;

    void SetScale(Vec3 _xyz);
    void SetScale(float _uniform);
    void SetScaleX(float _x);
    void SetScaleY(float _y);
    void SetScaleZ(float _z);

    void AddScale(Vec3 _xyz);
    void AddScale(float _uniform);
    void AddScaleX(float _x);
    void AddScaleY(float _y);
    void AddScaleZ(float _z);

    // rotation
    [[nodiscard]] Quat      GetRotation() const;
    [[nodiscard]] AxisAngle ToAxisAngle() const;
    [[nodiscard]] Vec3      ToEuler() const;

    void SetRotation(Quat _q);
    void SetRotation(Vec3 _axis, float _rad);
    void SetRotation(Vec3 _euler);
    void SetLookAt(Vec3 _focusPoint, Vec3 _u);
    void SetLookTo(Vec3 _f, Vec3 _u);

    void AddRotation(Quat _q);
    void AddRotation(Vec3 _axis, float _rad);
    void AddRotation(Vec3 _euler);
    void AddRotationPitch(float _rad);
    void AddRotationYaw(float _rad);
    void AddRotationRoll(float _rad);

    [[nodiscard]] Vec3  GetTranslation() const;
    [[nodiscard]] float GetTranslationX() const;
    [[nodiscard]] float GetTranslationY() const;
    [[nodiscard]] float GetTranslationZ() const;

    void SetTranslation(Vec3 _xyz);
    void SetTranslationX(float _x);
    void SetTranslationY(float _y);
    void SetTranslationZ(float _z);

    void AddTranslation(Vec3 _xyz);
    void AddTranslationX(float _x);
    void AddTranslationY(float _y);
    void AddTranslationZ(float _z);

private:
    Vec3 m_scale       = Vec3::kOne;
    Quat m_rotation    = Quat::kIdentity;
    Vec3 m_translation = Vec3::kZero;

    mutable Mtx4                     m_cachedSRT    = Mtx4::kIdentity;
    mutable Mtx4                     m_cachedInvSRT = Mtx4::kIdentity;
    mutable Flags<eMatrixDirtyFlags> m_dirtyFlags   = eMatrixDirtyFlags::None;
};

}   // namespace Jam