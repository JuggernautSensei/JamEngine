#include "pch.h"

#include "RenderPass.h"

#include "Graphics.h"

namespace jam
{

RenderPass::~RenderPass()
{
    CleanUp_();
}

RenderPass::RenderPass(
    RenderPass&& _pass) noexcept
    : m_flags(_pass.m_flags)
    , m_ph(_pass.m_ph)
    , m_fbh(_pass.m_fbh)
    , m_vp(_pass.m_vp)
    , m_state(_pass.m_state)
    , m_fstencil(_pass.m_fstencil)
    , m_bstencil(_pass.m_bstencil)
    , m_stencilRef(_pass.m_stencilRef)
    , m_blendStart(_pass.m_blendStart)
    , m_blendEnd(_pass.m_blendEnd)
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
    std::memcpy(m_blend, _pass.m_blend, sizeof(m_blend));

    _pass.m_flags = eFlags_None;
    _pass.m_ph    = k_invalidHandle;
    _pass.m_fbh   = k_invalidHandle;
}

RenderPass& RenderPass::operator=(
    RenderPass&& _pass) noexcept
{
    if (this != &_pass)
    {
        CleanUp_();

        std::memcpy(m_rgba, _pass.m_rgba, sizeof(m_rgba));
        std::memcpy(m_blend, _pass.m_blend, sizeof(m_blend));

        m_flags           = _pass.m_flags;
        m_ph              = _pass.m_ph;
        m_fbh             = _pass.m_fbh;
        m_clearColorStart = _pass.m_clearColorStart;
        m_clearColorEnd   = _pass.m_clearColorEnd;
        m_clearDepth      = _pass.m_clearDepth;
        m_clearStencil    = _pass.m_clearStencil;
        m_vp              = _pass.m_vp;
        m_cbufferBinds    = std::move(_pass.m_cbufferBinds);
        m_textureBinds    = std::move(_pass.m_textureBinds);
        m_samplerBinds    = std::move(_pass.m_samplerBinds);
        m_state           = _pass.m_state;
        m_blendStart      = _pass.m_blendStart;
        m_blendEnd        = _pass.m_blendEnd;
        m_blendFactor     = _pass.m_blendFactor;
        m_fstencil        = _pass.m_fstencil;
        m_bstencil        = _pass.m_bstencil;
        m_stencilRef      = _pass.m_stencilRef;
        m_executor        = std::move(_pass.m_executor);

        _pass.m_flags = eFlags_None;
        _pass.m_ph    = k_invalidHandle;
        _pass.m_fbh   = k_invalidHandle;
    }
    return *this;
}

RenderPass& RenderPass::SetProgram(
    const ProgramHandle _ph,
    const bool          _bDestoryProgram)
{
    if (JAM_IS_VALID_HANDLE(m_ph) && (m_flags & eFlags_DestroyProgram))
    {
        Graphics::DestroyProgram(m_ph);
    }

    m_ph = _ph;
    SetBit(m_flags, eFlags_DestroyProgram, _bDestoryProgram);
    return *this;
}

RenderPass& RenderPass::SetFrameBuffer(
    const FrameBufferHandle _fbh,
    const bool              _bDestoryFrameBuffer)
{
    if (JAM_IS_VALID_HANDLE(m_fbh) && (m_flags & eFlags_DestroyFrameBuffer))
    {
        Graphics::DestroyFrameBuffer(m_fbh);
    }

    m_fbh = _fbh;
    SetBit(m_flags, eFlags_DestroyFrameBuffer, _bDestoryFrameBuffer);
    return *this;
}

RenderPass& RenderPass::SetClearRenderTargetCommand(
    const RGBA  _rgba,
    const Int32 _renderTargetIdx)
{
    JAM_ASSERT(_renderTargetIdx >= 0 && _renderTargetIdx < k_numRenderTargetLimit, "Render target index out of range.");
    m_rgba[_renderTargetIdx] = _rgba;
    m_clearColorStart        = Min(m_clearColorStart, _renderTargetIdx);
    m_clearColorEnd          = Max(m_clearColorEnd, _renderTargetIdx);
    return *this;
}

RenderPass& RenderPass::SetClearDepthStencilCommand(
    const bool  _bClearDepth,
    const bool  _bClearStencil,
    const float _depth,
    const UInt8 _stencil)
{
    SetBit(m_flags, eFlags_ClearDepth, _bClearDepth);
    SetBit(m_flags, eFlags_ClearStencil, _bClearStencil);
    m_clearDepth   = _depth;
    m_clearStencil = _stencil;
    return *this;
}

RenderPass& RenderPass::SetViewport(
    const Viewport _vp)
{
    m_vp = _vp;
    return *this;
}

RenderPass& RenderPass::SetViewport(
    const UInt16 _x,
    const UInt16 _y,
    const UInt16 _width,
    const UInt16 _height)
{
    m_vp = Viewport { _x, _y, _width, _height };
    return *this;
}

RenderPass& RenderPass::AddConstantBuffer(
    const ConstantBufferHandle _cbh,
    const eShader              _stage,
    const Int32                _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < k_numConstantBufferLimit, "Constant buffer slot out of range.");
    m_cbufferBinds.emplace_back(_cbh, _stage, _slot);
    return *this;
}

RenderPass& RenderPass::AddTexture(
    const TextureHandle _texh,
    const eShader       _stage,
    const Int32         _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < k_numShaderResourceLimit, "Texture slot out of range.");
    m_textureBinds.emplace_back(_texh, _stage, _slot);
    return *this;
}

RenderPass& RenderPass::AddSampler(
    const SamplerState _state,
    const eShader      _stage,
    const Int32        _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < k_numSamplerLimit, "Sampler slot out of range.");
    m_samplerBinds.emplace_back(_state, _stage, _slot);
    return *this;
}

RenderPass& RenderPass::SetRenderState(
    const RenderState _state)
{
    m_state = _state;
    return *this;
}

RenderPass& RenderPass::SetBlendState(
    const BlendState _state,
    const Int32      _renderTargetIdx)
{
    m_blend[_renderTargetIdx] = _state;
    m_blendStart              = Min(m_blendStart, _renderTargetIdx);
    m_blendEnd                = Max(m_blendEnd, _renderTargetIdx);
    return *this;
}

RenderPass& RenderPass::SetStencil(
    const StencilState _fstencil,
    const StencilState _bstencil,
    const UInt8        _ref)
{
    m_fstencil   = _fstencil;
    m_bstencil   = _bstencil;
    m_stencilRef = _ref;
    return *this;
}

RenderPass& RenderPass::SetExecutor(
    const std::function<void()>& _executor)
{
    m_executor = _executor;
    return *this;
}

void RenderPass::Execute() const
{
    Execute(m_fbh, m_vp);
}

void RenderPass::Execute(
    const FrameBufferHandle _fbh,
    const Viewport          _vp) const
{
    Graphics::SetProgram(m_ph);
    Graphics::SetRenderState(m_state);
    Graphics::SetStencil(m_fstencil, m_bstencil, m_stencilRef);
    Graphics::SetFrameBuffer(_fbh);
    Graphics::SetViewport(_vp);

    for (const ConstantBufferBind& bind: m_cbufferBinds)
    {
        Graphics::SetConstantBuffer(bind.cbh, bind.stage, bind.slot);
    }

    for (const TextureBind& bind: m_textureBinds)
    {
        Graphics::SetTexture(bind.texh, bind.stage, bind.slot);
    }

    for (const SamplerBind& bind: m_samplerBinds)
    {
        Graphics::SetSampler(bind.state, bind.stage, bind.slot);
    }

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
    for (Int32 i = m_blendStart; i <= m_blendEnd; ++i)
    {
        Graphics::SetBlendState(m_blend[i], i);
    }
    Graphics::SetBlendFactor(m_blendFactor);

    JAM_ASSERT(m_executor, "Render pass executor is not set.");
    m_executor();
}

bool RenderPass::Validate() const
{
    // 프로그램이 없다면 유효하지 않음
    if (!JAM_IS_VALID_HANDLE(m_ph))
    {
        return false;
    }

    // 입력과 출력 리소스가 중복되면 conflict
    {
        const FrameBufferInfo fbInfo = Graphics::GetFrameBufferInfo(m_fbh);

        HashSet<UInt32> outputResources;   // key: handle value
        outputResources.reserve(fbInfo.numRenderTargets + 1);

        // 렌더타겟 텍스처 수집
        for (Int32 i = 0; i < fbInfo.numRenderTargets; ++i)
        {
            const TextureHandle texh = fbInfo.renderTargetTexhs[i];
            JAM_ASSERT(JAM_IS_VALID_HANDLE(texh), "Frame buffer has invalid render target texture handle.");
            outputResources.emplace(texh.value);
        }

        // 뎁스스텐실 텍스처 수집
        if (JAM_IS_VALID_HANDLE(fbInfo.depthStencilTexh))
        {
            outputResources.emplace(fbInfo.depthStencilTexh.value);
        }

        // 입력 리소스와 충돌 검사
        for (const TextureBind& texBind: m_textureBinds)
        {
            if (outputResources.contains(texBind.texh.value))
            {
                return false;
            }
        }
    }

    // A: 동일 스테이지 + 동일 리소스  -> 충돌
    // B: 동일 쉐이더 + 동일 슬롯      -> 중복 명령
    {
        HashSet<UInt64> textureStage;   // key: handle | stage
        textureStage.reserve(m_textureBinds.size());

        HashSet<UInt64> stageSlot;   // key: stage | slot
        stageSlot.reserve(m_textureBinds.size());

        for (const TextureBind& bind: m_textureBinds)
        {
            // A
            {
                const UInt64 key          = CreateKeyByHandleStage_(bind.texh, bind.stage);
                const auto [_, bSucceded] = textureStage.emplace(key);
                if (!bSucceded)   // dup
                {
                    return false;
                }
            }

            // check slot bindings for duplicates at same stage
            {
                const UInt64 key          = CreateKeyByStageSlot_(bind.stage, bind.slot);
                const auto [_, bSucceded] = stageSlot.emplace(key);
                if (!bSucceded)   // dup
                {
                    return false;
                }
            }
        }
    }

    // 동일 쉐이더 + 동일 슬롯에 대해 중복 바인딩이 존재하는 경우
    {
        // cbuffer | stage set
        // check cbuffer bindings for duplicates at same stage
        HashSet<UInt64> cbufferStageSet;
        cbufferStageSet.reserve(m_cbufferBinds.size());

        // stage | slot set
        // check slot bindings for duplicates at same stage
        HashSet<UInt64> stageSlotSet;
        stageSlotSet.reserve(m_cbufferBinds.size());

        // check cbuffer bindings for duplicates at same stage
        for (const ConstantBufferBind& cbBind: m_cbufferBinds)
        {
            // check cbuffer bindings for duplicates at same stage
            {
                const UInt64 key          = CreateKeyByHandleStage_(cbBind.cbh, cbBind.stage);
                const auto [_, bSucceded] = cbufferStageSet.emplace(key);
                if (!bSucceded)   // dup
                {
                    return false;
                }
            }

            // check slot bindings for duplicates at same stage
            {
                const UInt64 key          = CreateKeyByStageSlot_(cbBind.stage, cbBind.slot);
                const auto [_, bSucceded] = stageSlotSet.emplace(key);
                if (!bSucceded)   // dup
                {
                    return false;
                }
            }
        }
    }

    return true;
}

void RenderPass::CleanUp_()
{
    if (m_flags & eFlags_DestroyProgram)
    {
        JAM_DESTROY_PROGRAM(m_ph);
    }

    if (m_flags & eFlags_DestroyFrameBuffer)
    {
        JAM_DESTROY_FRAME_BUFFER(m_fbh);
    }
}

UInt64 RenderPass::CreateKeyByHandleStage_(
    const AnyHandle _handle,
    const eShader   _stage) const
{
    return (static_cast<UInt64>(_handle.value) << 32) | static_cast<UInt64>(_stage);
}

UInt64 RenderPass::CreateKeyBySamplerStage_(
    const SamplerState _state,
    const eShader      _stage) const
{
    return (static_cast<UInt64>(_state.value) << 32) | static_cast<UInt64>(_stage);
}

UInt64 RenderPass::CreateKeyByStageSlot_(
    eShader     _stage,
    const Int32 _slot) const
{
    return (static_cast<UInt64>(_stage) << 32) | static_cast<UInt64>(_slot);
}

}   // namespace jam