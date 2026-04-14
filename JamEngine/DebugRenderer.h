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
        Vec3 pos = Numbers<Vec3>::k_zero;
        RGBA col = RGBA::k_black;

        inline static VertexLayout s_layout = [] {
            VertexLayout layout {};
            layout.Push(eVertexAttribute::Position, eAttributeFormat::Float3);
            layout.Push(eVertexAttribute::Color0, eAttributeFormat::UInt8x4);
            return layout;
        }();
    };

    struct Constants
    {
        Mtx4  viewProj     = Numbers<Mtx4>::k_identity;
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
    ProgramHandle m_worldPh  = k_invalidHandle;
    ProgramHandle m_screenPh = k_invalidHandle;

    ConstantBufferHandle m_cbh          = k_invalidHandle;
    VertexBufferHandle   m_vbh          = k_invalidHandle;
    Int32                m_vbhByteWidth = 0;
    IndexBufferHandle    m_ibh          = k_invalidHandle;
    Int32                m_ibhByteWidth = 0;

    constexpr static SizeT         k_maxNumVerts   = Limits<UInt16>::k_max + 1;
    constexpr static SizeT         k_maxNumIndices = k_maxNumVerts;
    Array<Vertex, k_maxNumVerts>   m_cpuVerts      = {};
    Int32                          m_numVerts      = 0;
    Array<UInt16, k_maxNumIndices> m_cpuIndices    = {};
    Int32                          m_numIndices    = 0;

    Vec3 m_cursorPos   = Numbers<Vec3>::k_zero;
    RGBA m_cursorCol   = RGBA::k_black;
    bool m_bCursorDown = false;

    bool m_bBeginDraw = false;

    eDebugRendererSpace m_drawSpace = eDebugRendererSpace::World;

    Flags<eRenderFlags> m_userRenderFlags = eRenderFlags::None;
};

}   // namespace Jam