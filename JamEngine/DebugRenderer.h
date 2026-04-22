#pragma once
#include "GraphicsBase.h"
#include "RenderView.h"

namespace Jam
{

enum class eDebugRendererSpace
{
    World,
    Screen,
};

class DebugRenderer
{

    struct Vertex
    {
        Vec3 pos = Vec3::kZero;
        RGBA col = RGBA::kBlack;

        inline static VertexLayout s_layout = [] {
            VertexLayout layout {};
            layout.Push(eVertexAttribute::Position, eAttributeFormat::Float3);
            layout.Push(eVertexAttribute::Color0, eAttributeFormat::UInt8x4);
            return layout;
        }();
    };

    struct Constants
    {
        Mtx4  viewProj     = Mtx4::kIdentity;
        float screenWidth  = 0.f;
        float screenHeight = 0.f;
        float padding[2]   = {};
    };

public:
    DebugRenderer();
    ~DebugRenderer();

    DebugRenderer(const DebugRenderer&)            = delete;
    DebugRenderer& operator=(const DebugRenderer&) = delete;
    DebugRenderer(DebugRenderer&&)                 = delete;
    DebugRenderer& operator=(DebugRenderer&&)      = delete;

    void Begin(const RenderView& _view);
    void End();
    void SetColor(RGBA _col);
    void SetSpace(eDebugRendererSpace _space);
    void SetRenderFlags(Flags<eRenderFlags> _flags);

    void MoveTo(Vec3 _pos);
    void MoveTo(float _x, float _y, float _z = 0.f);
    void DrawTo(Vec3 _pos);
    void DrawTo(float _x, float _y, float _z = 0.f);
    void Flush();

private:
    ProgramHandle m_worldPh  = kInvalidHandle;
    ProgramHandle m_screenPh = kInvalidHandle;

    ConstantBufferHandle m_cbh          = kInvalidHandle;
    VertexBufferHandle   m_vbh          = kInvalidHandle;
    Int32                m_vbhByteWidth = 0;
    IndexBufferHandle    m_ibh          = kInvalidHandle;
    Int32                m_ibhByteWidth = 0;

    constexpr static SizeT         kMaxNumVerts   = Limits<UInt16>::kMax + 1;
    constexpr static SizeT         kMaxNumIndices = kMaxNumVerts;
    Array<Vertex, kMaxNumVerts>   m_cpuVerts      = {};
    Int32                          m_numVerts      = 0;
    Array<UInt16, kMaxNumIndices> m_cpuIndices    = {};
    Int32                          m_numIndices    = 0;

    Vec3 m_cursorPos   = Vec3::kZero;
    RGBA m_cursorCol   = RGBA::kBlack;
    bool m_bCursorDown = false;

    bool m_bBeginDraw = false;

    eDebugRendererSpace m_drawSpace = eDebugRendererSpace::World;

    Flags<eRenderFlags> m_userRenderFlags = eRenderFlags::None;
};

}   // namespace Jam