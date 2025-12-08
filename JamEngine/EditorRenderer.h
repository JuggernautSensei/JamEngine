#pragma once
#include "BoundingBox.h"
#include "GraphicsBase.h"
#include "Mesh.h"

namespace jam
{

struct RenderView;

enum class eAxis
{
    X,
    Y,
    Z,
};

enum eDrawGridFlags_
{
    eDrawGridFlags_None = 0,
    eDrawGridFlags_Grid = 1 << 0,
    eDrawGridFlags_Axis = 1 << 1,
};
using eDrawGridFlags = UInt32;

class EditorRenderer
{
public:
    void Init();
    void Shutdown();

    void Begin(const RenderView& _view);
    void End();

    // state 설정
    void SetColor(RGBA _color);

    // grid rendering
    void DrawGrid(eDrawGridFlags _flags);

    // line rendering
    void DrawLine(Vec3 _from, Vec3 _to, RGBA _color);
    void DrawBoundingBox(const Aabb& _aabbInL, const Mat4& _mtx, RGBA _color);
    void DrawFrustum(const Mat4& _viewProjMtx, RGBA _rgba);
    void DrawCircle(Vec3 _center, Vec3 _normal, float _radius, RGBA _color, Int32 _segments = 32);
    void FlushLines();

    // filled primitive rendering
    void DrawFilledSphere(Vec3 _center, float _radius, RGBA _color);
    void DrawFilledCube(Vec3 _center, Vec3 _size, RGBA _color);
    void DrawFilledCylinder(Vec3 _center, Vec3 _dir, float _height, float _radius, RGBA _color);
    void DrawFilledCapsule(Vec3 _center, Vec3 _dir, float _height, float _radius, RGBA _color);

private:
    void CreateGridQuad_();
    void CreatePrimitives_();
    void FlushLines_();
    void UploadTransform_(const Mat4& _world) const;
    void DrawGrid_() const;

    // 라인 배칭
    struct LineVertex
    {
        Vec3 position;
        RGBA rgba;

        static VertexLayout s_layout;

        static void Init()
        {
            VertexLayout layout;
            layout.Add(eVertexAttribute::Position, eAttributeFormat::Float3);
            layout.Add(eVertexAttribute::Color0, eAttributeFormat::UInt8x4);
            s_layout = std::move(layout);
        }
    };

    static constexpr Int32 k_lineLimit                     = 512;
    LineVertex             m_lineVertices[k_lineLimit * 2] = {};
    UInt16                 m_lineIndices[k_lineLimit * 2]  = {};
    Int32                  m_numLineVerts                  = 0;
    Int32                  m_numLineIndices                = 0;

    // 버퍼
    VertexBufferHandle   m_gridVbh = k_invalidHandle;
    IndexBufferHandle    m_gridIbh = k_invalidHandle;
    VertexBufferHandle   m_lineVbh = k_invalidHandle;
    IndexBufferHandle    m_lineIbh = k_invalidHandle;
    ConstantBufferHandle m_cbh     = k_invalidHandle;

    // 프로그램
    ProgramHandle m_gridProgramHandle   = k_invalidHandle;
    ProgramHandle m_lineProgramHandle   = k_invalidHandle;
    ProgramHandle m_filledProgramHandle = k_invalidHandle;

    // 프리미티브 메쉬
    Mesh m_sphere;
    Mesh m_cylinder;
    Mesh m_cube;

    // 상태
    RenderState m_lineState;
    RenderState m_gridState;
    RenderState m_filledState;

    BlendState     m_blendState;
    RGBA           m_currentColor  = RGBA::k_white;
    eDrawGridFlags m_drawGridFlags = eDrawGridFlags_None;
    bool           m_bBeginRender  = false;

    // 카메라
    Vec3 m_eye  = Vec3::Zero;
    Mat4 m_view = Mat4::Identity;
    Mat4 m_proj = Mat4::Identity;
};

}   // namespace jam