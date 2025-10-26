#include "pch.h"

#include "RenderPass.h"

#include "Graphics.h"

namespace jam
{

JAM_NODISCARD UInt64 MakeValidateSetKey(const UInt32 _handleValue, const eShader _stage)
{
    return (static_cast<UInt64>(_handleValue) << 32) | static_cast<UInt64>(_stage);
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
    FrameBufferInfo info = Graphics::GetFrameBufferInfo(_handle);

    m_frameBuffer       = _handle;
    m_renderTargetCount = info.renderTargetCount;

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
    return *this;
}

RenderPass& RenderPass::SetStencil(const StencilState _front, const StencilState _back, const UInt8 _ref)
{
    m_fstencil   = _front;
    m_bstencil   = _back;
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
    Graphics::SetProgram(m_program);

    Graphics::SetFrameBuffer(m_frameBuffer);
    Graphics::SetViewport(m_viewport);

    for (const ConstantBufferBindItem& cb: m_cbufferBindlist)
        Graphics::SetConstantBuffer(cb.handle, cb.stage, cb.slot);
    for (const TextureBindItem& tex: m_textureBindlist)
        Graphics::SetTexture(tex.handle, tex.stage, tex.slot);
    for (const SamplerBindItem& samp: m_samplerBindlist)
        Graphics::SetSampler(samp.state, samp.stage, samp.slot);

    Graphics::SetGfxState(m_gfxState);
    Graphics::SetStencil(m_fstencil, m_bstencil, m_stencilRef);
    for (Int32 i = 0; i < m_renderTargetCount; ++i)
        Graphics::SetBlendState(i, m_blendStates[i]);
    Graphics::SetBlendFactor(m_blendFactor);

    if (m_executor)
        m_executor();
}

bool RenderPass::Validate() const
{
    if (!JAM_IS_VALID_HANDLE(m_program))
    {
        return false;
    }

    if (!JAM_IS_VALID_HANDLE(m_frameBuffer))
    {
        return false;
    }

    if (m_viewport.w == 0 || m_viewport.h == 0)
    {
        return false;
    }

    // 리소스 중복 혹은 충돌 검사
    HashSet<UInt64> validateSet;

    // 입력 리소스와 출력 리소스 충돌 검사
    {
        // 출력 리소스 수집
        FrameBufferInfo fbInfo = Graphics::GetFrameBufferInfo(m_frameBuffer);
        for (Int32 i = 0; i < fbInfo.renderTargetCount; ++i)
        {
            TextureHandle rtth = fbInfo.renderTargetTextureHandles[i];
            JAM_ASSERT(JAM_IS_VALID_HANDLE(rtth), "Frame buffer has invalid render target texture handle.");
            validateSet.emplace(rtth.value);
        }
        validateSet.emplace(fbInfo.depthStencilTextureHandle.value);

        // 입력 리소스와 충돌하는지 검사
        for (const TextureBindItem& texBind: m_textureBindlist)
        {
            if (validateSet.contains(texBind.handle.value))
                return false;
        }
    }

    // 입력 리소스 중복 검사
    {
        // 텍스처
        {
            validateSet.clear();
            for (const TextureBindItem& texBind: m_textureBindlist)
            {
                auto [_, bResult] = validateSet.emplace(MakeValidateSetKey(texBind.handle.value, texBind.stage));
                if (!bResult)
                    return false;
            }
        }

        // 상수 버퍼
        {
            validateSet.clear();
            for (const ConstantBufferBindItem& cbBind: m_cbufferBindlist)
            {
                auto [_, bResult] = validateSet.emplace(MakeValidateSetKey(cbBind.handle.value, cbBind.stage));
                if (!bResult)
                    return false;
            }
        }

        // 샘플러
        {
            validateSet.clear();
            for (const SamplerBindItem& sampBind: m_samplerBindlist)
            {
                auto [_, bResult] = validateSet.emplace(MakeValidateSetKey(sampBind.state.value, sampBind.stage));
                if (!bResult)
                    return false;
            }
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
    JAM_ASSERT(0 <= _renderTargetIndex && _renderTargetIndex < m_renderTargetCount, "Render target index out of range.");
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