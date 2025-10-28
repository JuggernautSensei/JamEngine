#include "pch.h"

#include "RenderPass.h"

#include "Graphics.h"

namespace jam
{

JAM_NODISCARD UInt64 MakeRenderPassHandleStageKey(const UInt32 _handleValue, const eShader _stage)
{
    return (static_cast<UInt64>(_handleValue) << 32) | static_cast<UInt64>(_stage);
}

JAM_NODISCARD UInt64 MakeRenderPassStageSlotKey(eShader _stage, const Int32 _slot)
{
    return (static_cast<UInt64>(_stage) << 32) | static_cast<UInt64>(_slot);
}

}   // namespace jam

namespace jam
{

RenderPass& RenderPass::SetProgram(const ProgramHandle _handle)
{
    m_program = _handle;
    return *this;
}

RenderPass& RenderPass::SetFrameBuffer(const FrameBufferHandle _handle)
{
    m_frameBuffer = _handle;
    return *this;
}

RenderPass& RenderPass::SetViewport(const Viewport _viewport)
{
    m_viewport = _viewport;
    return *this;
}

RenderPass& RenderPass::SetViewport(const UInt16 _x, const UInt16 _y, const UInt16 _width, const UInt16 _height)
{
    m_viewport = Viewport(_x, _y, _width, _height);
    return *this;
}

RenderPass& RenderPass::AddConstantBuffer(const ConstantBufferHandle _handle, const eShader _stage, const Int32 _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < k_constantBufferCountMax, "Constant buffer slot out of range.");
    m_cbufferBindlist.emplace_back(_handle, _stage, _slot);
    return *this;
}

RenderPass& RenderPass::AddTexture(const TextureHandle _handle, const eShader _stage, const Int32 _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < k_shaderResourceCountMax, "Texture slot out of range.");
    m_textureBindlist.emplace_back(_handle, _stage, _slot);
    return *this;
}

RenderPass& RenderPass::AddSampler(const SamplerState _state, const eShader _stage, const Int32 _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < k_samplerStateCountMax, "Sampler slot out of range.");
    m_samplerBindlist.emplace_back(_state, _stage, _slot);
    return *this;
}

RenderPass& RenderPass::SetGfxState(const GfxState _state)
{
    m_gfxState = _state;
    return *this;
}

RenderPass& RenderPass::SetBlendState(const Int32 _renderTargetIndex, const BlendState _state)
{
    m_blendStates[_renderTargetIndex] = _state;
    m_blendStateStart                 = Min(m_blendStateStart, _renderTargetIndex);
    m_blendStateEnd                   = Max(m_blendStateEnd, _renderTargetIndex);
    return *this;
}

RenderPass& RenderPass::SetStencil(const StencilState _fstencil, const StencilState _bstencil, const UInt8 _ref)
{
    m_fstencil   = _fstencil;
    m_bstencil   = _bstencil;
    m_stencilRef = _ref;
    return *this;
}

RenderPass& RenderPass::SetExecutor(const std::function<void()>& _executor)
{
    m_executor = _executor;
    return *this;
}

void RenderPass::Execute() const
{
    Execute(m_frameBuffer, m_viewport);
}

void RenderPass::Execute(const FrameBufferHandle _fbh, const Viewport _viewport) const
{
    Graphics::SetProgram(m_program);

    for (const ConstantBufferBindItem& cb: m_cbufferBindlist)
        Graphics::SetConstantBuffer(cb.handle, cb.stage, cb.slot);
    for (const TextureBindItem& tex: m_textureBindlist)
        Graphics::SetTexture(tex.handle, tex.stage, tex.slot);
    for (const SamplerBindItem& samp: m_samplerBindlist)
        Graphics::SetSampler(samp.state, samp.stage, samp.slot);

    Graphics::SetGfxState(m_gfxState);
    Graphics::SetStencil(m_fstencil, m_bstencil, m_stencilRef);

    Graphics::SetFrameBuffer(_fbh);
    Graphics::SetViewport(_viewport);

    for (Int32 i = m_blendStateStart; i <= m_blendStateEnd; ++i)
        Graphics::SetBlendState(m_blendStates[i], i);
    Graphics::SetBlendFactor(m_blendFactor);

    JAM_ASSERT(m_executor, "Render pass executor is not set.");
    m_executor();
}

bool RenderPass::Validate() const
{
    if (!JAM_IS_VALID_HANDLE(m_program))
        return false;

    // 리소스 중복 혹은 충돌 검사
    HashSet<UInt64> handleStageSet;
    HashSet<UInt64> stageSlotSet;

    // 입력 리소스와 출력 리소스 충돌 검사
    {
        // 출력 리소스 수집
        FrameBufferInfo fbInfo = Graphics::GetFrameBufferInfo(m_frameBuffer);
        handleStageSet.reserve(fbInfo.renderTargetCount + 1 + m_textureBindlist.size());

        for (Int32 i = 0; i < fbInfo.renderTargetCount; ++i)
        {
            TextureHandle rtth = fbInfo.renderTargetTextureHandles[i];
            JAM_ASSERT(JAM_IS_VALID_HANDLE(rtth), "Frame buffer has invalid render target texture handle.");
            handleStageSet.emplace(static_cast<UInt64>(rtth.value));
        }
        handleStageSet.emplace(static_cast<UInt64>(fbInfo.depthStencilTextureHandle.value));

        // 입력 리소스와 충돌하는지 검사
        for (const TextureBindItem& texBind: m_textureBindlist)
        {
            if (handleStageSet.contains(texBind.handle.value))
                return false;
        }
    }

    // 입력 리소스 중복 검사
    {
        // 텍스처 검사
        handleStageSet.clear();
        stageSlotSet.clear();
        handleStageSet.reserve(m_textureBindlist.size());
        stageSlotSet.reserve(m_textureBindlist.size());

        for (const TextureBindItem& texBind: m_textureBindlist)
        {
            UInt64 handleStageKey = MakeRenderPassHandleStageKey(texBind.handle.value, texBind.stage);
            UInt64 stageSlotKey   = MakeRenderPassStageSlotKey(texBind.stage, texBind.slot);

            if (!handleStageSet.emplace(handleStageKey).second)
                return false;
            if (!stageSlotSet.emplace(stageSlotKey).second)
                return false;
        }

        // 상수 버퍼 검사
        handleStageSet.clear();
        stageSlotSet.clear();
        handleStageSet.reserve(m_cbufferBindlist.size());
        stageSlotSet.reserve(m_cbufferBindlist.size());

        for (const ConstantBufferBindItem& cbBind: m_cbufferBindlist)
        {
            UInt64 handleStageKey = MakeRenderPassHandleStageKey(cbBind.handle.value, cbBind.stage);
            UInt64 stageSlotKey   = MakeRenderPassStageSlotKey(cbBind.stage, cbBind.slot);

            if (!handleStageSet.emplace(handleStageKey).second)
                return false;
            if (!stageSlotSet.emplace(stageSlotKey).second)
                return false;
        }

        // 샘플러 검사
        handleStageSet.clear();
        stageSlotSet.clear();
        handleStageSet.reserve(m_samplerBindlist.size());
        stageSlotSet.reserve(m_samplerBindlist.size());

        for (const SamplerBindItem& sampBind: m_samplerBindlist)
        {
            UInt64 handleStageKey = MakeRenderPassHandleStageKey(sampBind.state.value, sampBind.stage);
            UInt64 stageSlotKey   = MakeRenderPassStageSlotKey(sampBind.stage, sampBind.slot);

            if (!stageSlotSet.emplace(stageSlotKey).second)
                return false;
            if (!handleStageSet.emplace(handleStageKey).second)
                return false;
        }
    }

    return true;
}

ProgramHandle RenderPass::GetProgram() const
{
    return m_program;
}

FrameBufferHandle RenderPass::GetFrameBuffer() const
{
    return m_frameBuffer;
}

Viewport RenderPass::GetViewport() const
{
    return m_viewport;
}

GfxState RenderPass::GetGfxState() const
{
    return m_gfxState;
}

BlendState RenderPass::GetBlendState(const Int32 _renderTargetIndex) const
{
    JAM_ASSERT(0 <= _renderTargetIndex && _renderTargetIndex < 8, "Render target index out of range.");
    return m_blendStates[_renderTargetIndex];
}

RGBA RenderPass::GetBlendFactor() const
{
    return m_blendFactor;
}

StencilState RenderPass::GetFrontStencilState() const
{
    return m_fstencil;
}

StencilState RenderPass::GetBackStencilState() const
{
    return m_bstencil;
}

UInt8 RenderPass::GetStencilRef() const
{
    return m_stencilRef;
}

const std::vector<ConstantBufferBindItem>& RenderPass::GetConstantBufferBindList() const
{
    return m_cbufferBindlist;
}

const std::vector<TextureBindItem>& RenderPass::GetTextureBindList() const
{
    return m_textureBindlist;
}

const std::vector<SamplerBindItem>& RenderPass::GetSamplerBindList() const
{
    return m_samplerBindlist;
}

}   // namespace jam