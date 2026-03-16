#pragma once
#include "GraphicsBase.h"

namespace Jam::Regacy
{

class RenderGraph_Regacy;

struct ConstantBufferBind
{
    ConstantBufferHandle cbh   = k_nullHandle;
    eShader              stage = eShader::VertexShader;
    Int32                slot  = 0;
};

struct SamplerBind
{
    SamplerState state = {};
    eShader      stage = eShader::VertexShader;
    Int32        slot  = 0;
};

struct TextureBind
{
    TextureHandle texh  = k_nullHandle;
    eShader       stage = eShader::VertexShader;
    Int32         slot  = 0;
};

class RenderPass_Regacy
{
    friend class RenderGraph_Regacy;

    enum class eFlags : UInt32
    {
        None               = 0,
        DestroyProgram     = 1 << 0,
        DestroyFrameBuffer = 1 << 1,
        ClearDepth         = 1 << 3,
        ClearStencil       = 1 << 4,
    };

public:
    RenderPass_Regacy() = default;
    ~RenderPass_Regacy();

    RenderPass_Regacy(const RenderPass_Regacy&)            = delete;
    RenderPass_Regacy& operator=(const RenderPass_Regacy&) = delete;
    RenderPass_Regacy(RenderPass_Regacy&&) noexcept;
    RenderPass_Regacy& operator=(RenderPass_Regacy&&) noexcept;

    void SetProgram(ProgramHandle _ph, bool _bDestoryProgram = false);
    void SetFrameBuffer(FrameBufferHandle _fbh, bool _bDestoryFrameBuffer = false);
    void SetClearRenderTargetCommand(RGBA _rgba, Int32 _renderTargetIdx);
    void SetClearDepthStencilCommand(bool _bClearDepth, bool _bClearStencil, float _depth, UInt8 _stencil);
    void SetViewport(Viewport _vp);
    void SetViewport(UInt16 _x, UInt16 _y, UInt16 _width, UInt16 _height);
    void AddConstantBuffer(ConstantBufferHandle _cbh, eShader _stage, Int32 _slot);
    void AddTexture(TextureHandle _texh, eShader _stage, Int32 _slot);
    void AddSampler(SamplerState _state, eShader _stage, Int32 _slot);
    void SetRenderState(RenderState _state);
    void SetBlendState(BlendState _state, Int32 _renderTargetIdx);
    void SetStencil(StencilState _fstencil, StencilState _bstencil, UInt8 _ref);
    void SetExecutor(const Delegate<void()>& _executor);

    void Execute() const;
    void Execute(FrameBufferHandle _fbh, Viewport _vp) const;

    [[nodiscard]] bool CheckValidation() const;

private:
    void                 CleanUp_();
    [[nodiscard]] UInt64 MakeHandleStageKey_(AnyHandle _handle, eShader _stage) const;
    [[nodiscard]] UInt64 MakeSamplerStageKey_(SamplerState _state, eShader _stage) const;
    [[nodiscard]] UInt64 MakeStageSlotKey_(eShader _stage, Int32 _slot) const;

    // misc
    Flags<eFlags> m_flags = eFlags::None;

    // core
    ProgramHandle     m_ph  = k_nullHandle;
    FrameBufferHandle m_fbh = k_nullHandle;
    Viewport          m_vp  = {};

    // rendering state
    RenderState  m_state      = RenderState::k_default;
    StencilState m_fstencil   = StencilState::k_disable;
    StencilState m_bstencil   = StencilState::k_disable;
    UInt8        m_stencilRef = 0;

    // blend
    BlendState m_blend[8]    = { BlendState::k_opaque };
    Int32      m_blendStart  = Limits<Int32>::k_max;
    Int32      m_blendEnd    = Limits<Int32>::k_min;
    RGBA       m_blendFactor = RGBA { 0 };

    // clear renderable
    RGBA  m_rgba[8]         = {};
    Int32 m_clearColorStart = Limits<Int32>::k_max;
    Int32 m_clearColorEnd   = Limits<Int32>::k_min;
    float m_clearDepth      = 1.f;
    UInt8 m_clearStencil    = 0;

    // resource binding
    Vector<ConstantBufferBind> m_cbufferBinds = {};
    Vector<TextureBind>        m_textureBinds = {};
    Vector<SamplerBind>        m_samplerBinds = {};

    Delegate<void()> m_executor = nullptr;
};

}   // namespace Jam