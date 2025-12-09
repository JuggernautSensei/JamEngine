#pragma once
#include "GraphicsBase.h"

namespace jam
{

class RenderGraph;

struct ConstantBufferBind
{
    ConstantBufferHandle handle = k_invalidHandle;
    eShader              stage  = eShader::Unknown;
    Int32                slot   = 0;
};

struct SamplerBind
{
    SamplerState state = {};
    eShader      stage = eShader::Unknown;
    Int32        slot  = 0;
};

struct TextureBind
{
    TextureHandle handle = k_invalidHandle;
    eShader       stage  = eShader::Unknown;
    Int32         slot   = 0;
};

class RenderPass
{
    enum eFlags_ : UInt32
    {
        eFlags_None               = 0,
        eFlags_DestroyProgram     = 1 << 0,
        eFlags_DestroyFrameBuffer = 1 << 1,

        eFlags_ClearDepth   = 1 << 3,
        eFlags_ClearStencil = 1 << 4,
    };

    using eFlags = EnumIntType<eFlags_>;

public:
    RenderPass() = default;
    ~RenderPass();

    RenderPass(const RenderPass&)            = delete;
    RenderPass& operator=(const RenderPass&) = delete;
    RenderPass(RenderPass&&) noexcept;
    RenderPass& operator=(RenderPass&&) noexcept;

    RenderPass& SetProgram(ProgramHandle _handle, bool _bDestoryProgram = false);
    RenderPass& SetFrameBuffer(FrameBufferHandle _handle, bool _bDestoryFrameBuffer = false);
    RenderPass& SetClearRenderTargetCommand(RGBA _color, Int32 _renderTargetIndex);
    RenderPass& SetClearDepthStencilCommand(bool _bClearDepth, bool _bClearStencil, float _depth, UInt8 _stencil);

    RenderPass& SetViewport(Viewport _viewport);
    RenderPass& SetViewport(UInt16 _x, UInt16 _y, UInt16 _width, UInt16 _height);

    RenderPass& AddConstantBuffer(ConstantBufferHandle _cbh, eShader _stage, Int32 _slot);
    RenderPass& AddTexture(TextureHandle _texh, eShader _stage, Int32 _slot);
    RenderPass& AddSampler(SamplerState _state, eShader _stage, Int32 _slot);

    RenderPass& SetRenderState(RenderState _state);
    RenderPass& SetBlendState(BlendState _state, Int32 _renderTargetIndex);
    RenderPass& SetStencil(StencilState _fstencil, StencilState _bstencil, UInt8 _ref);
    RenderPass& SetExecutor(const std::function<void()>& _executor);

    void Execute() const;
    void Execute(FrameBufferHandle _fbh, Viewport _vp) const;

    [[nodiscard]] bool Validate() const;

private:
    void CleanUp_();

    // misc
    eFlags m_flags = eFlags_None;

    // core
    ProgramHandle     m_program = k_invalidHandle;
    FrameBufferHandle m_fbh     = k_invalidHandle;
    Viewport          m_vp      = {};

    // rendering state
    RenderState  m_renderState = RenderState::k_default;
    StencilState m_fstencil    = StencilState::k_disable;
    StencilState m_bstencil    = StencilState::k_disable;
    UInt8        m_stencilRef  = 0;

    // blend
    BlendState m_blendStates[8]  = { BlendState::k_opaque };
    Int32      m_blendStateStart = k_int32Max;
    Int32      m_blendStateEnd   = k_int32Min;
    RGBA       m_blendFactor     = RGBA { 0 };

    // clear renderable
    RGBA  m_rgba[8]         = {};
    Int32 m_clearColorStart = k_int32Max;
    Int32 m_clearColorEnd   = k_int32Min;
    float m_clearDepth      = 1.f;
    UInt8 m_clearStencil    = 0;

    // resource binding
    std::vector<ConstantBufferBind> m_cbufferBinds = {};
    std::vector<TextureBind>        m_textureBinds = {};
    std::vector<SamplerBind>        m_samplerBinds = {};

    std::function<void()> m_executor = nullptr;

    friend class RenderGraph;
};

}   // namespace jam