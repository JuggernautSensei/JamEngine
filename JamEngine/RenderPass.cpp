#include "pch.h"

#include "RenderPass.h"

#include "Graphics.h"

namespace jam::render_pass
{

[[nodiscard]] UInt64 MakeKeyByHandleStage(const AnyHandle _handle, const eShader _stage)
{
    return (static_cast<UInt64>(_handle.value) << 32) | static_cast<UInt64>(_stage);
}

[[nodiscard]] UInt64 MakeKeyBySamplerStage(const SamplerState _state, const eShader _stage)
{
    return (static_cast<UInt64>(_state.value) << 32) | static_cast<UInt64>(_stage);
}

[[nodiscard]] UInt64 MakeKeyByStageSlot(eShader _stage, const Int32 _slot)
{
    return (static_cast<UInt64>(_stage) << 32) | static_cast<UInt64>(_slot);
}

}   // namespace jam::render_pass

namespace jam
{

RenderPass::~RenderPass()
{
    CleanUp_();
}

RenderPass::RenderPass(RenderPass&& _pass) noexcept
    : m_flags(_pass.m_flags)
    , m_program(_pass.m_program)
    , m_fbh(_pass.m_fbh)
    , m_vp(_pass.m_vp)
    , m_gfxState(_pass.m_gfxState)
    , m_fstencil(_pass.m_fstencil)
    , m_bstencil(_pass.m_bstencil)
    , m_stencilRef(_pass.m_stencilRef)
    , m_blendStateStart(_pass.m_blendStateStart)
    , m_blendStateEnd(_pass.m_blendStateEnd)
    , m_blendFactor(_pass.m_blendFactor)
    , m_clearColorStart(_pass.m_clearColorStart)
    , m_clearColorEnd(_pass.m_clearColorEnd)
    , m_clearDepth(_pass.m_clearDepth)
    , m_clearStencil(_pass.m_clearStencil)
    , m_cbufferBinds(std::move(_pass.m_cbufferBinds))
    , m_textureBinds(std::move(_pass.m_textureBinds))
    , m_samplerBinds(std::move(_pass.m_samplerBinds))
    , m_executor(std::move(_pass.m_executor))
{
    std::memcpy(m_rgba, _pass.m_rgba, sizeof(m_rgba));
    std::memcpy(m_blendStates, _pass.m_blendStates, sizeof(m_blendStates));

    _pass.m_flags   = eFlags_None;
    _pass.m_program = k_invalidHandle;
    _pass.m_fbh     = k_invalidHandle;
}

RenderPass& RenderPass::operator=(RenderPass&& _pass) noexcept
{
    if (this != &_pass)
    {
        CleanUp_();

        std::memcpy(m_rgba, _pass.m_rgba, sizeof(m_rgba));
        std::memcpy(m_blendStates, _pass.m_blendStates, sizeof(m_blendStates));

        m_flags           = _pass.m_flags;
        m_program         = _pass.m_program;
        m_fbh             = _pass.m_fbh;
        m_clearColorStart = _pass.m_clearColorStart;
        m_clearColorEnd   = _pass.m_clearColorEnd;
        m_clearDepth      = _pass.m_clearDepth;
        m_clearStencil    = _pass.m_clearStencil;
        m_vp              = _pass.m_vp;
        m_cbufferBinds    = std::move(_pass.m_cbufferBinds);
        m_textureBinds    = std::move(_pass.m_textureBinds);
        m_samplerBinds    = std::move(_pass.m_samplerBinds);
        m_gfxState        = _pass.m_gfxState;
        m_blendStateStart = _pass.m_blendStateStart;
        m_blendStateEnd   = _pass.m_blendStateEnd;
        m_blendFactor     = _pass.m_blendFactor;
        m_fstencil        = _pass.m_fstencil;
        m_bstencil        = _pass.m_bstencil;
        m_stencilRef      = _pass.m_stencilRef;
        m_executor        = std::move(_pass.m_executor);

        _pass.m_flags   = eFlags_None;
        _pass.m_program = k_invalidHandle;
        _pass.m_fbh     = k_invalidHandle;
    }
    return *this;
}

RenderPass& RenderPass::SetProgram(const ProgramHandle _handle, const bool _bDestoryProgram)
{
    if (JAM_IS_VALID_HANDLE(m_program) && m_flags & eFlags_DestroyProgram)
    {
        Graphics::DestroyProgram(m_program);
    }

    m_program = _handle;
    m_flags   = SetBit<eFlags>(m_flags, eFlags_DestroyProgram, _bDestoryProgram);
    return *this;
}

RenderPass& RenderPass::SetFrameBuffer(const FrameBufferHandle _handle, const bool _bDestoryFrameBuffer)
{
    if (JAM_IS_VALID_HANDLE(m_fbh) && m_flags & eFlags_DestroyFrameBuffer)
    {
        Graphics::DestroyFrameBuffer(m_fbh);
    }

    m_fbh   = _handle;
    m_flags = SetBit<eFlags>(m_flags, eFlags_DestroyFrameBuffer, _bDestoryFrameBuffer);
    return *this;
}

RenderPass& RenderPass::SetClearRenderTargetCommand(const RGBA _color, const Int32 _renderTargetIndex)
{
    JAM_ASSERT(_renderTargetIndex >= 0 && _renderTargetIndex < k_numRenderTargetLimit, "Render target index out of range.");
    m_rgba[_renderTargetIndex] = _color;
    m_clearColorStart          = Min(m_clearColorStart, _renderTargetIndex);
    m_clearColorEnd            = Max(m_clearColorEnd, _renderTargetIndex);
    return *this;
}

RenderPass& RenderPass::SetClearDepthStencilCommand(const bool _bClearDepth, const bool _bClearStencil, const float _depth, const UInt8 _stencil)
{
    m_flags        = SetBit<eFlags>(m_flags, eFlags_ClearDepth, _bClearDepth);
    m_flags        = SetBit<eFlags>(m_flags, eFlags_ClearStencil, _bClearStencil);
    m_clearDepth   = _depth;
    m_clearStencil = _stencil;
    return *this;
}

RenderPass& RenderPass::SetViewport(const Viewport _viewport)
{
    m_vp = _viewport;
    return *this;
}

RenderPass& RenderPass::SetViewport(const UInt16 _x, const UInt16 _y, const UInt16 _width, const UInt16 _height)
{
    m_vp = Viewport(_x, _y, _width, _height);
    return *this;
}

RenderPass& RenderPass::AddConstantBuffer(const ConstantBufferHandle _cbh, const eShader _stage, const Int32 _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < k_numConstantBufferLimit, "Constant buffer slot out of range.");
    m_cbufferBinds.emplace_back(_cbh, _stage, _slot);
    return *this;
}

RenderPass& RenderPass::AddTexture(const TextureHandle _texh, const eShader _stage, const Int32 _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < k_numShaderResourceLimit, "Texture slot out of range.");
    m_textureBinds.emplace_back(_texh, _stage, _slot);
    return *this;
}

RenderPass& RenderPass::AddSampler(const SamplerState _state, const eShader _stage, const Int32 _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < k_numSamplerLimit, "Sampler slot out of range.");
    m_samplerBinds.emplace_back(_state, _stage, _slot);
    return *this;
}

RenderPass& RenderPass::SetGfxState(const RenderState _state)
{
    m_gfxState = _state;
    return *this;
}

RenderPass& RenderPass::SetBlendState(const BlendState _state, const Int32 _renderTargetIndex)
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
    Execute(m_fbh, m_vp);
}

void RenderPass::Execute(const FrameBufferHandle _fbh, const Viewport _vp) const
{
    JAM_ASSERT(m_executor, "Render pass executor is not set.");

    Graphics::SetProgram(m_program);

    for (const ConstantBufferBind& cb: m_cbufferBinds)
    {
        Graphics::SetConstantBuffer(cb.handle, cb.stage, cb.slot);
    }

    for (const TextureBind& tex: m_textureBinds)
    {
        Graphics::SetTexture(tex.handle, tex.stage, tex.slot);
    }

    for (const SamplerBind& samp: m_samplerBinds)
    {
        Graphics::SetSampler(samp.state, samp.stage, samp.slot);
    }

    Graphics::SetGfxState(m_gfxState);
    Graphics::SetStencil(m_fstencil, m_bstencil, m_stencilRef);

    Graphics::SetFrameBuffer(_fbh);
    Graphics::SetViewport(_vp);

    // clear render targets
    for (Int32 i = m_clearColorStart; i <= m_clearColorEnd; ++i)
    {
        Graphics::ClearRenderTarget(_fbh, m_rgba[i], i);
    }

    // clear depth/stencil
    const bool bClearDepth   = (m_flags & eFlags_ClearDepth) != 0;
    const bool bClearStencil = (m_flags & eFlags_ClearStencil) != 0;
    if (bClearDepth || bClearStencil)
    {
        Graphics::ClearDepthStencil(_fbh, bClearDepth, bClearStencil, m_clearDepth, m_clearStencil);
    }

    // blend
    for (Int32 i = m_blendStateStart; i <= m_blendStateEnd; ++i)
    {
        Graphics::SetBlendState(m_blendStates[i], i);
    }
    Graphics::SetBlendFactor(m_blendFactor);

    m_executor();
}

bool RenderPass::Validate() const
{
    if (!JAM_IS_VALID_HANDLE(m_program))
    {
        return false;
    }

    {
        FrameBufferInfo fbInfo = Graphics::GetFrameBufferInfo(m_fbh);
        HashSet<UInt64> outputResourceSet;
        outputResourceSet.reserve(fbInfo.numRenderTargets + 1);

        for (Int32 i = 0; i < fbInfo.numRenderTargets; ++i)
        {
            TextureHandle renderTargetTexh = fbInfo.renderTargetTexhs[i];
            JAM_ASSERT(JAM_IS_VALID_HANDLE(renderTargetTexh), "Frame buffer has invalid render target texture handle.");
            outputResourceSet.emplace(renderTargetTexh.value);
        }

        if (JAM_IS_VALID_HANDLE(fbInfo.depthStencilTexh))
        {
            outputResourceSet.emplace(fbInfo.depthStencilTexh.value);
        }

        for (const TextureBind& texBind: m_textureBinds)
        {
            if (outputResourceSet.contains(texBind.handle.value))
            {
                return false;
            }
        }
    }

    // Validate texture bindings for duplicates
    {
        HashSet<UInt64> handleStageSet;
        HashSet<UInt64> stageSlotSet;
        handleStageSet.reserve(m_textureBinds.size());
        stageSlotSet.reserve(m_textureBinds.size());

        for (const TextureBind& texBind: m_textureBinds)
        {
            const UInt64 handleStageKey = render_pass::MakeKeyByHandleStage(texBind.handle, texBind.stage);
            const UInt64 stageSlotKey   = render_pass::MakeKeyByStageSlot(texBind.stage, texBind.slot);

            if (!handleStageSet.emplace(handleStageKey).second)
            {
                return false;
            }

            if (!stageSlotSet.emplace(stageSlotKey).second)
            {
                return false;
            }
        }
    }

    {
        HashSet<UInt64> handleStageSet;
        HashSet<UInt64> stageSlotSet;
        handleStageSet.reserve(m_cbufferBinds.size());
        stageSlotSet.reserve(m_cbufferBinds.size());

        for (const ConstantBufferBind& cbBind: m_cbufferBinds)
        {
            const UInt64 handleStageKey = render_pass::MakeKeyByHandleStage(cbBind.handle, cbBind.stage);
            const UInt64 stageSlotKey   = render_pass::MakeKeyByStageSlot(cbBind.stage, cbBind.slot);

            if (!handleStageSet.emplace(handleStageKey).second || !stageSlotSet.emplace(stageSlotKey).second)
            {
                return false;
            }
        }
    }

    return true;
}

void RenderPass::CleanUp_()
{
    if (m_flags & eFlags_DestroyProgram)
    {
        JAM_DESTROY_PROGRAM(m_program);
    }

    if (m_flags & eFlags_DestroyFrameBuffer)
    {
        JAM_DESTROY_FRAME_BUFFER(m_fbh);
    }
}

}   // namespace jam