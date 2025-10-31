#pragma once
#include "GraphicsBase.h"

namespace jam
{

struct ConstantBufferBindItem
{
    ConstantBufferHandle handle = k_invalidHandle;
    eShader              stage  = eShader::Unknown;
    Int32                slot   = 0;
};

struct SamplerBindItem
{
    SamplerState state = SamplerState::k_default;
    eShader      stage = eShader::Unknown;
    Int32        slot  = 0;
};

struct TextureBindItem
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
    RenderPass& AddClearRenderTargetCommand(RGBA _color, Int32 _renderTargetIndex);
    RenderPass& AddClearDepthStencilCommand(bool _bClearDepth, bool _bClearStencil, float _depth, UInt8 _stencil);

    RenderPass& SetViewport(Viewport _viewport);
    RenderPass& SetViewport(UInt16 _x, UInt16 _y, UInt16 _width, UInt16 _height);

    RenderPass& AddConstantBuffer(ConstantBufferHandle _handle, eShader _stage, Int32 _slot);
    RenderPass& AddTexture(TextureHandle _handle, eShader _stage, Int32 _slot);
    RenderPass& AddSampler(SamplerState _state, eShader _stage, Int32 _slot);

    RenderPass& SetGfxState(GfxState _state);
    RenderPass& SetBlendState(BlendState _state, Int32 _renderTargetIndex);
    RenderPass& SetStencil(StencilState _fstencil, StencilState _bstencil, UInt8 _ref);

    RenderPass& SetExecutor(const std::function<void()>& _executor);

    void Execute() const;
    void Execute(FrameBufferHandle _fbh, Viewport _viewport) const;   // 원하는 프레임버퍼와 뷰포트로 오버라이드하여 실행

    JAM_NODISCARD bool Validate() const;

    JAM_NODISCARD ProgramHandle     GetProgram() const;
    JAM_NODISCARD FrameBufferHandle GetFrameBuffer() const;
    JAM_NODISCARD Viewport          GetViewport() const;

    JAM_NODISCARD const std::vector<ConstantBufferBindItem>& GetConstantBufferBindList() const;
    JAM_NODISCARD const std::vector<TextureBindItem>& GetTextureBindList() const;
    JAM_NODISCARD const std::vector<SamplerBindItem>& GetSamplerBindList() const;

    JAM_NODISCARD GfxState     GetGfxState() const;
    JAM_NODISCARD BlendState   GetBlendState(Int32 _renderTargetIndex) const;
    JAM_NODISCARD RGBA         GetBlendFactor() const;
    JAM_NODISCARD StencilState GetFrontStencilState() const;
    JAM_NODISCARD StencilState GetBackStencilState() const;
    JAM_NODISCARD UInt8        GetStencilRef() const;

private:
    void CleanUp_();

    eFlags m_flags = eFlags_None;

    // program
    ProgramHandle m_program = k_invalidHandle;

    // frame buffer
    FrameBufferHandle m_frameBuffer     = k_invalidHandle;
    RGBA              m_rgba[8]         = {};
    Int32             m_clearColorStart = Int32Limits::max();
    Int32             m_clearColorEnd   = Int32Limits::min();
    float             m_depth           = 1.f;
    UInt8             m_stencil         = 0;

    // viewport
    Viewport m_viewport = {};

    // input resources
    std::vector<ConstantBufferBindItem>   m_cbufferBindlist       = {};
    std::vector<TextureBindItem>          m_textureBindlist       = {};
    std::vector<SamplerBindItem>          m_samplerBindlist       = {};

    // state
    GfxState     m_gfxState   = GfxState::k_default;
    StencilState m_fstencil   = StencilState::k_disable;
    StencilState m_bstencil   = StencilState::k_disable;
    UInt8        m_stencilRef = 0;

    BlendState m_blendStates[8]  = { BlendState::k_opaque };
    Int32      m_blendStateStart = Int32Limits::max();
    Int32      m_blendStateEnd   = Int32Limits::min();

    RGBA m_blendFactor = RGBA { 0 };

    std::function<void()> m_executor = nullptr;
};

}   // namespace jam