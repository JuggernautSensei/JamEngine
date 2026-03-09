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
    void DrawLine(Vector3 _from, Vector3 _to, RGBA _color);
    void DrawBoundingBox(const Aabb& _aabbInL, const Matrix4x4& _mtx, RGBA _color);
    void DrawFrustum(const Matrix4x4& _viewProjMtx, RGBA _rgba);
    void DrawCircle(Vector3 _center, Vector3 _normal, float _radius, RGBA _color, Int32 _segments = 32);
    void FlushLines();

    // filled primitive rendering
    void DrawFilledSphere(Vector3 _center, float _radius, RGBA _color);
    void DrawFilledCube(Vector3 _center, Vector3 _size, RGBA _color);
    void DrawFilledCylinder(Vector3 _center, Vector3 _dir, float _height, float _radius, RGBA _color);
    void DrawFilledCapsule(Vector3 _center, Vector3 _dir, float _height, float _radius, RGBA _color);

private:
    void FlushLines_();
    void UploadTransform_(const Matrix4x4& _world) const;
    void DrawGrid_() const;

    // 라인 배칭
    struct LineVertex
    {
        Vector3 position = Vec3::k_zero;
        RGBA rgba     = RGBA::k_white;

        inline static VertexLayout s_vl = {};

        static void Init()
        {
            s_vl.Reset();
            s_vl.Push(eVertexAttribute::Position, eAttributeFormat::Float3);
            s_vl.Push(eVertexAttribute::Color0, eAttributeFormat::UInt8x4);
        }
    };

    static constexpr Int32             k_lineLimit      = 512;
    Array<LineVertex, k_lineLimit * 2> m_lineVertices   = {};
    Array<UInt16, k_lineLimit * 2>     m_lineIndices    = {};
    Int32                              m_numLineVerts   = 0;
    Int32                              m_numLineIndices = 0;

    // 버퍼
    VertexBufferHandle   m_gridVbh = k_nullHandle;
    IndexBufferHandle    m_gridIbh = k_nullHandle;
    VertexBufferHandle   m_lineVbh = k_nullHandle;
    IndexBufferHandle    m_lineIbh = k_nullHandle;
    ConstantBufferHandle m_cbh     = k_nullHandle;

    // 프로그램
    ProgramHandle m_gridPh   = k_nullHandle;
    ProgramHandle m_linePh   = k_nullHandle;
    ProgramHandle m_filledPh = k_nullHandle;

    // 프리미티브 메쉬
    Ref<Mesh> m_sphereMesh   = nullptr;
    Ref<Mesh> m_cylinderMesh = nullptr;
    Ref<Mesh> m_cubeMesh     = nullptr;

    // 상태
    RenderState m_lineState   = {};
    RenderState m_gridState   = {};
    RenderState m_filledState = {};

    BlendState            m_blendState    = {};
    RGBA                  m_currentColor  = RGBA::k_white;
    Flags<eDrawGridFlags> m_drawGridFlags = eDrawGridFlags::None;
    bool                  m_bBeginRender  = false;

    // 카메라
    Vector3 m_eye  = Vec3::k_zero;
    Matrix4x4 m_view = Mtx4::k_identity;
    Matrix4x4 m_proj = Mtx4::k_identity;

    // 쉐이더 코드
    static const char* const k_gridShaderSource;
    static const char* const k_lineShaderSource;
};

}   // namespace Jam