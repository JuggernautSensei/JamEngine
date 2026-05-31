#pragma once
#include "BoundingVolume.h"
#include "GraphicsBase.h"
#include "Mesh.h"

namespace Jam
{

struct RenderView;

enum class eDrawGridFlags : UInt32
{
    None = 0,
    Grid = 1 << 0,
    Axis = 1 << 1,
};

class EditorRenderer
{
public:
    EditorRenderer();
    ~EditorRenderer();

    EditorRenderer(const EditorRenderer&)            = delete;
    EditorRenderer& operator=(const EditorRenderer&) = delete;
    EditorRenderer(EditorRenderer&&)                 = delete;
    EditorRenderer& operator=(EditorRenderer&&)      = delete;

    void Begin(const RenderView& _view);
    void End();

    // state 설정
    void SetColor(RGBA _color);

    // grid rendering
    void DrawGrid(Flags<eDrawGridFlags> _flags);

    // line rendering
    void DrawLine(Vec3 _from, Vec3 _to, RGBA _color);
    void DrawBoundingBox(const Aabb& _aabbInL, const Mtx4& _mtx, RGBA _color);
    void DrawFrustum(const Mtx4& _viewProjMtx, RGBA _rgba);
    void DrawCircle(Vec3 _center, Vec3 _normal, float _radius, RGBA _color, Int32 _segments = 32);
    void FlushLines();

    // filled primitive rendering
    void DrawFilledSphere(Vec3 _center, float _radius, RGBA _color);
    void DrawFilledCube(Vec3 _center, Vec3 _size, RGBA _color);
    void DrawFilledCylinder(Vec3 _center, Vec3 _dir, float _height, float _radius, RGBA _color);
    void DrawFilledCapsule(Vec3 _center, Vec3 _dir, float _height, float _radius, RGBA _color);

private:
    void FlushLines_();
    void UploadTransform_(const Mtx4& _world) const;
    void DrawGrid_() const;

    // 라인 배칭
    struct LineVertex
    {
        Vec3 position = Vec3::kZero;
        RGBA rgba     = RGBA::kWhite;
    };

    static constexpr Int32            kLineLimit       = 512;
    Array<LineVertex, kLineLimit * 2> m_lineVertices   = {};
    Array<UInt16, kLineLimit * 2>     m_lineIndices    = {};
    Int32                             m_numLineVerts   = 0;
    Int32                             m_numLineIndices = 0;

    // 버퍼
    VertexBufferHandle   m_screenTriVbh = kInvalidHandle;
    IndexBufferHandle    m_screenTriIbh = kInvalidHandle;
    VertexBufferHandle   m_lineVbh = kInvalidHandle;
    IndexBufferHandle    m_lineIbh = kInvalidHandle;
    ConstantBufferHandle m_cbh     = kInvalidHandle;

    // 프로그램
    ProgramHandle m_gridPh   = kInvalidHandle;
    ProgramHandle m_linePh   = kInvalidHandle;
    ProgramHandle m_filledPh = kInvalidHandle;

    // 프리미티브 메쉬
    Ref<Mesh> m_sphereMesh   = nullptr;
    Ref<Mesh> m_cylinderMesh = nullptr;
    Ref<Mesh> m_cubeMesh     = nullptr;

    // 상태
    Flags<eRenderFlags> m_lineFlags   = {};
    Flags<eRenderFlags> m_gridFlags   = {};
    Flags<eRenderFlags> m_filledFlags = {};

    Flags<eBlendFlags>    m_blendFlags    = {};
    RGBA                  m_curColor      = RGBA::kWhite;
    Flags<eDrawGridFlags> m_drawGridFlags = eDrawGridFlags::None;
    bool                  m_bBeginRender  = false;

    // 카메라
    Vec3 m_eye  = Vec3::kZero;
    Mtx4 m_view = Mtx4::kIdentity;
    Mtx4 m_proj = Mtx4::kIdentity;
};

}   // namespace Jam