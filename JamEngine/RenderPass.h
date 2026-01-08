#pragma once
#include "GraphicsBase.h"

namespace jam
{

class RenderGraph;

struct ConstantBufferBind
{
    ConstantBufferHandle cbh   = k_invalidHandle;
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
    TextureHandle texh  = k_invalidHandle;
    eShader       stage = eShader::VertexShader;
    Int32         slot  = 0;
};

class RenderPass
{
    friend class RenderGraph;

    enum eFlags_ : UInt32
    {
        eFlags_None               = 0,
        eFlags_DestroyProgram     = 1 << 0,
        eFlags_DestroyFrameBuffer = 1 << 1,

        eFlags_ClearDepth   = 1 << 3,
        eFlags_ClearStencil = 1 << 4,
    };
    using eFlags = UnderlyingType<eFlags_>;

public:
    RenderPass() = default;
    ~RenderPass();

    RenderPass(const RenderPass&)            = delete;
    RenderPass& operator=(const RenderPass&) = delete;
    RenderPass(RenderPass&&) noexcept;
    RenderPass& operator=(RenderPass&&) noexcept;

    RenderPass& SetProgram(ProgramHandle _ph, bool _bDestoryProgram = false);
    RenderPass& SetFrameBuffer(FrameBufferHandle _fbh, bool _bDestoryFrameBuffer = false);
    RenderPass& SetClearRenderTargetCommand(RGBA _rgba, Int32 _renderTargetIdx);
    RenderPass& SetClearDepthStencilCommand(bool _bClearDepth, bool _bClearStencil, float _depth, UInt8 _stencil);

    RenderPass& SetViewport(Viewport _vp);
    RenderPass& SetViewport(UInt16 _x, UInt16 _y, UInt16 _width, UInt16 _height);

    RenderPass& AddConstantBuffer(ConstantBufferHandle _cbh, eShader _stage, Int32 _slot);
    RenderPass& AddTexture(TextureHandle _texh, eShader _stage, Int32 _slot);
    RenderPass& AddSampler(SamplerState _state, eShader _stage, Int32 _slot);

    RenderPass& SetRenderState(RenderState _state);
    RenderPass& SetBlendState(BlendState _state, Int32 _renderTargetIdx);
    RenderPass& SetStencil(StencilState _fstencil, StencilState _bstencil, UInt8 _ref);
    RenderPass& SetExecutor(const std::function<void()>& _executor);

    void Execute() const;
    void Execute(FrameBufferHandle _fbh, Viewport _vp) const;

    [[nodiscard]] bool IsValid() const;

private:
    void                 CleanUp_();
    [[nodiscard]] UInt64 CreateKeyByHandleStage_(AnyHandle _handle, eShader _stage) const;
    [[nodiscard]] UInt64 CreateKeyBySamplerStage_(SamplerState _state, eShader _stage) const;
    [[nodiscard]] UInt64 CreateKeyByStageSlot_(eShader _stage, Int32 _slot) const;

    // misc
    eFlags m_flags = eFlags_None;

    // core
    ProgramHandle     m_ph  = k_invalidHandle;
    FrameBufferHandle m_fbh = k_invalidHandle;
    Viewport          m_vp  = {};

    // rendering state
    RenderState  m_state      = RenderState::k_default;
    StencilState m_fstencil   = StencilState::k_disable;
    StencilState m_bstencil   = StencilState::k_disable;
    UInt8        m_stencilRef = 0;

    // blend
    BlendState m_blend[8]    = { BlendState::k_opaque };
    Int32      m_blendStart  = k_int32Max;
    Int32      m_blendEnd    = k_int32Min;
    RGBA       m_blendFactor = RGBA { 0 };

    // clear renderable
    RGBA  m_rgba[8]         = {};
    Int32 m_clearColorStart = k_int32Max;
    Int32 m_clearColorEnd   = k_int32Min;
    float m_clearDepth      = 1.f;
    UInt8 m_clearStencil    = 0;

    // resource binding
    Vector<ConstantBufferBind> m_cbufferBinds = {};
    Vector<TextureBind>        m_textureBinds = {};
    Vector<SamplerBind>        m_samplerBinds = {};

    std::function<void()> m_executor = nullptr;
};

}   // namespace jam