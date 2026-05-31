#pragma once
#include "Archive.h"
#include "BoundingVolume.h"

namespace Jam
{

enum class eProjection
{
    Perspective = 0,
    Orthographic,
};

class Camera
{
    enum class eDirtyFlags : UInt32
    {
        None    = 0,
        Proj    = 1 << 0,
        Frustum = 1 << 1,
        All     = Proj | Frustum
    };

public:
    Camera() = default;

    [[nodiscard]] const Mtx4&    GetProj() const;
    [[nodiscard]] const Frustum& GetFrustum() const;

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

    void  WriteToArchive(ArchiveWriter& _writer) const;
    Error ReadFromArchive(ArchiveReader& _reader);

    constexpr static float kMinZ           = 0.01f;
    constexpr static float kMaxZ           = 30000.f;
    constexpr static float kMinFovYRad     = ToRad(1.f);
    constexpr static float kMaxFovYRad     = ToRad(179.f);
    constexpr static float kMinWidthHeight = 0.1f;
    constexpr static float kMaxWidthHeight = 10000.f;

private:
    eProjection m_projection = eProjection::Perspective;
    float       m_nearZ      = 0.1f;
    float       m_farZ       = 250.f;
    float       m_fovYRad    = ToRad(45.f);
    float       m_width      = 1.f;
    float       m_height     = 1.f;

    mutable Flags<eDirtyFlags> m_dirtyFlags = eDirtyFlags::All;
    mutable Mtx4               m_projMtx    = Mtx4::kIdentity;
    mutable Frustum            m_frustum    = {};
};

JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Camera);

}   // namespace Jam