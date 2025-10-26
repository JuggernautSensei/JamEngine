#pragma once
#include "GraphicsBase.h"

namespace jam
{

struct ConstantBufferBindItem
{
    ConstantBufferHandle handle;
    eShader              stage;
    Int32                slot;
};

struct SamplerBindItem
{
    SamplerState state;
    eShader      stage;
    Int32        slot;
};

struct TextureBindItem
{
    TextureHandle handle;
    eShader       stage;
    Int32         slot;
};

class RenderPass
{
public:
    RenderPass() = default;

    RenderPass& SetProgram(ProgramHandle _handle);
    RenderPass& SetFrameBuffer(FrameBufferHandle _handle);
    RenderPass& SetViewport(Viewport _viewport);
    RenderPass& SetViewport(UInt16 _x, UInt16 _y, UInt16 _width, UInt16 _height);

    RenderPass& AddConstantBuffer(ConstantBufferHandle _handle, eShader _stage, Int32 _slot);
    RenderPass& AddTexture(TextureHandle _handle, eShader _stage, Int32 _slot);
    RenderPass& AddSampler(SamplerState _state, eShader _stage, Int32 _slot);

    RenderPass& SetGfxState(GfxState _state);
    RenderPass& SetBlendState(Int32 _renderTargetIndex, BlendState _state);
    RenderPass& SetStencil(StencilState _front, StencilState _back, UInt8 _ref);

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
    // program
    ProgramHandle m_program = k_invalidHandle;

    // output resources
    FrameBufferHandle m_frameBuffer = k_invalidHandle;
    Viewport          m_viewport    = {};

    // input resources
    std::vector<ConstantBufferBindItem> m_cbufferBindlist = {};
    std::vector<TextureBindItem>        m_textureBindlist = {};
    std::vector<SamplerBindItem>        m_samplerBindlist = {};

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