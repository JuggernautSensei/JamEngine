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

RenderPass::~RenderPass()
{
    CleanUp_();
}

RenderPass::RenderPass(RenderPass&& _pass) noexcept
{
    m_flags       = _pass.m_flags;
    m_program     = _pass.m_program;
    m_frameBuffer = _pass.m_frameBuffer;
    std::memcpy(m_rgba, _pass.m_rgba, sizeof(m_rgba));
    m_clearColorStart = _pass.m_clearColorStart;
    m_clearColorEnd   = _pass.m_clearColorEnd;
    m_depth           = _pass.m_depth;
    m_stencil         = _pass.m_stencil;
    m_viewport        = _pass.m_viewport;
    m_cbufferBindlist = std::move(_pass.m_cbufferBindlist);
    m_textureBindlist = std::move(_pass.m_textureBindlist);
    m_samplerBindlist = std::move(_pass.m_samplerBindlist);
    m_gfxState        = _pass.m_gfxState;
    std::memcpy(m_blendStates, _pass.m_blendStates, sizeof(m_blendStates));
    m_blendStateStart       = _pass.m_blendStateStart;
    m_blendStateEnd         = _pass.m_blendStateEnd;
    m_blendFactor           = _pass.m_blendFactor;
    m_fstencil              = _pass.m_fstencil;
    m_bstencil              = _pass.m_bstencil;
    m_stencilRef            = _pass.m_stencilRef;
    m_executor              = std::move(_pass.m_executor);

    // invalidate moved-from pass
    _pass.m_flags       = eFlags_None;
    _pass.m_program     = k_invalidHandle;
    _pass.m_frameBuffer = k_invalidHandle;
}

RenderPass& RenderPass::operator=(RenderPass&& _pass) noexcept
{
    if (this != &_pass)
    {
        CleanUp_();

        m_flags       = _pass.m_flags;
        m_program     = _pass.m_program;
        m_frameBuffer = _pass.m_frameBuffer;
        std::memcpy(m_rgba, _pass.m_rgba, sizeof(m_rgba));
        m_clearColorStart = _pass.m_clearColorStart;
        m_clearColorEnd   = _pass.m_clearColorEnd;
        m_depth           = _pass.m_depth;
        m_stencil         = _pass.m_stencil;
        m_viewport        = _pass.m_viewport;
        m_cbufferBindlist = std::move(_pass.m_cbufferBindlist);
        m_textureBindlist = std::move(_pass.m_textureBindlist);
        m_samplerBindlist = std::move(_pass.m_samplerBindlist);
        m_gfxState        = _pass.m_gfxState;
        std::memcpy(m_blendStates, _pass.m_blendStates, sizeof(m_blendStates));
        m_blendStateStart       = _pass.m_blendStateStart;
        m_blendStateEnd         = _pass.m_blendStateEnd;
        m_blendFactor           = _pass.m_blendFactor;
        m_fstencil              = _pass.m_fstencil;
        m_bstencil              = _pass.m_bstencil;
        m_stencilRef            = _pass.m_stencilRef;
        m_executor              = std::move(_pass.m_executor);

        // invalidate moved-from pass
        _pass.m_flags       = eFlags_None;
        _pass.m_program     = k_invalidHandle;
        _pass.m_frameBuffer = k_invalidHandle;
    }
    return *this;
}

RenderPass& RenderPass::SetProgram(const ProgramHandle _handle, const bool _bDestoryProgram)
{
    if (JAM_IS_VALID_HANDLE(m_program) && m_flags & eFlags_DestroyProgram)
        Graphics::DestroyProgram(m_program);

    m_program = _handle;
    m_flags   = ApplyFlags<eFlags>(m_flags, eFlags_DestroyProgram, _bDestoryProgram);
    return *this;
}

RenderPass& RenderPass::SetFrameBuffer(const FrameBufferHandle _handle, const bool _bDestoryFrameBuffer)
{
    if (JAM_IS_VALID_HANDLE(m_frameBuffer) && m_flags & eFlags_DestroyFrameBuffer)
        Graphics::DestroyFrameBuffer(m_frameBuffer);

    m_frameBuffer = _handle;
    m_flags       = ApplyFlags<eFlags>(m_flags, eFlags_DestroyFrameBuffer, _bDestoryFrameBuffer);
    return *this;
}

RenderPass& RenderPass::AddClearRenderTargetCommand(const RGBA _color, const Int32 _renderTargetIndex)
{
    JAM_ASSERT(_renderTargetIndex >= 0 && _renderTargetIndex < k_renderTargetCountMax, "Render target index out of range.");
    m_rgba[_renderTargetIndex] = _color;
    m_clearColorStart          = Min(m_clearColorStart, _renderTargetIndex);
    m_clearColorEnd            = Max(m_clearColorEnd, _renderTargetIndex);
    return *this;
}

RenderPass& RenderPass::AddClearDepthStencilCommand(const bool _bClearDepth, const bool _bClearStencil, const float _depth, const UInt8 _stencil)
{
    m_flags   = ApplyFlags<eFlags>(m_flags, eFlags_ClearDepth, _bClearDepth);
    m_flags   = ApplyFlags<eFlags>(m_flags, eFlags_ClearStencil, _bClearStencil);
    m_depth   = _depth;
    m_stencil = _stencil;
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
    Execute(m_frameBuffer, m_viewport);
}

void RenderPass::Execute(const FrameBufferHandle _fbh, const Viewport _viewport) const
{
    JAM_ASSERT(m_executor, "Render pass executor is not set.");

    // set program
    Graphics::SetProgram(m_program);

    // set input resources
    for (const ConstantBufferBindItem& cb: m_cbufferBindlist)
        Graphics::SetConstantBuffer(cb.handle, cb.stage, cb.slot);
    for (const TextureBindItem& tex: m_textureBindlist)
        Graphics::SetTexture(tex.handle, tex.stage, tex.slot);
    for (const SamplerBindItem& samp: m_samplerBindlist)
        Graphics::SetSampler(samp.state, samp.stage, samp.slot);

    // set gfx state
    Graphics::SetGfxState(m_gfxState);
    Graphics::SetStencil(m_fstencil, m_bstencil, m_stencilRef);

    // set frame buffer and viewport
    Graphics::SetFrameBuffer(_fbh);
    Graphics::SetViewport(_viewport);

    // clear render targets
    for (Int32 i = m_clearColorStart; i <= m_clearColorEnd; ++i)
        Graphics::ClearRenderTarget(_fbh, m_rgba[i], i);

    // clear depth stencil
    bool bClearDepth   = (m_flags & eFlags_ClearDepth) != 0;
    bool bClearStencil = (m_flags & eFlags_ClearStencil) != 0;
    if (bClearDepth || bClearStencil)
        Graphics::ClearDepthStencil(_fbh, bClearDepth, bClearStencil, m_depth, m_stencil);

    // set blend states
    for (Int32 i = m_blendStateStart; i <= m_blendStateEnd; ++i)
        Graphics::SetBlendState(m_blendStates[i], i);
    Graphics::SetBlendFactor(m_blendFactor);

    // draw call
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

void RenderPass::CleanUp_()
{
    if (m_flags & eFlags_DestroyProgram)
        JAM_DESTROY_PROGRAM(m_program);

    if (m_flags & eFlags_DestroyFrameBuffer)
        JAM_DESTROY_FRAME_BUFFER(m_frameBuffer);
}

}   // namespace jam