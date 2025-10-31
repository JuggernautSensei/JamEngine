#pragma once

namespace jam
{

enum class eProjection : Int8
{
    Perspective = 0,
    Orthographic
};

enum class eAspectRatio : Int8
{
    Ratio_4_3 = 0,
    Ratio_16_9,
    Ratio_21_9
};

class Camera
{
public:
    Camera();

    JAM_NODISCARD Mat4 CreateProj() const;

    void SetProjection(eProjection _projection);
    void SetAspectRatio(eAspectRatio _aspectRatio);
    void SetNearZ(float _nearZ);
    void SetFarZ(float _farZ);
    void SetFovYRad(float _fovYRad);
    void SetWidth(float _width);
    void SetHeight(float _height);

    JAM_NODISCARD eProjection GetProjection() const;
    JAM_NODISCARD float       GetAspectRatio() const;
    JAM_NODISCARD float       GetNearZ() const;
    JAM_NODISCARD float       GetFarZ() const;
    JAM_NODISCARD float       GetFovYRad() const;
    JAM_NODISCARD float       GetWidth() const;
    JAM_NODISCARD float       GetHeight() const;

    static constexpr float k_nearestZ  = 0.01f;
    static constexpr float k_farthestZ = 30000.f;

private:
    eProjection m_projection = eProjection::Perspective;
    float       m_nearZ      = 0.1f;
    float       m_farZ       = 1000.f;
    float       m_fovYRad    = ToRad(45.f);
    float       m_width      = 1.f;
    float       m_height     = 1.f;
};

}   // namespace jam