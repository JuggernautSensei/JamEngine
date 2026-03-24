#pragma once
#include "Archive.h"

namespace Jam
{

enum class eProjection
{
    Perspective = 0,
    Orthographic,
};

class Camera
{
public:
    Camera() = default;

    [[nodiscard]] const Mtx4& GetProjMatrix() const;

    void SetProjection(eProjection _projection);
    void SetNearZ(float _nearZ);
    void SetFarZ(float _farZ);
    void SetFovYRad(float _fovYRad);
    void SetWidth(float _width);
    void SetHeight(float _height);
    void SetAspectRatio(float _aspect);

    [[nodiscard]] eProjection GetProjection() const;
    [[nodiscard]] float       GetAspectRatio() const;
    [[nodiscard]] float       GetNearZ() const;
    [[nodiscard]] float       GetFarZ() const;
    [[nodiscard]] float       GetFovYRad() const;
    [[nodiscard]] float       GetWidth() const;
    [[nodiscard]] float       GetHeight() const;
    [[nodiscard]] bool        IsDirty() const;

    void  WriteToArchive(ArchiveWriter& _writer) const;
    Error ReadFromArchive(ArchiveReader& _reader);

    constexpr static float k_minZ           = 0.01f;
    constexpr static float k_maxZ           = 30000.f;
    constexpr static float k_minFovYRad     = ToRad(1.f);
    constexpr static float k_maxFovYRad     = ToRad(179.f);
    constexpr static float k_minWidthHeight = 0.1f;
    constexpr static float k_maxWidthHeight = 10000.f;

private:
    eProjection m_projection = eProjection::Perspective;
    float       m_nearZ      = 0.1f;
    float       m_farZ       = 250.f;
    float       m_fovYRad    = ToRad(45.f);
    float       m_width      = 1.f;
    float       m_height     = 1.f;

    mutable bool m_bDirty   = true;
    mutable Mtx4 m_mtxCache = Numbers<Mtx4>::k_identity;
};

JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Camera);

}   // namespace Jam