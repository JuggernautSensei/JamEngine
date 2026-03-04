#include "pch.h"

#include "RenderPass.h"

#include "Graphics.h"
#include "MemoryHasher.h"

namespace Jam
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

    _pass.m_flags = eFlags::None;
    _pass.m_ph    = k_nullHandle;
    _pass.m_fbh   = k_nullHandle;
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

        _pass.m_flags = eFlags::None;
        _pass.m_ph    = k_nullHandle;
        _pass.m_fbh   = k_nullHandle;
    }

    return *this;
}

void RenderPass::SetProgram(
    const ProgramHandle _ph,
    const bool          _bDestoryProgram)
{
    if (m_ph && (m_flags & eFlags::DestroyProgram))
    {
        GetGraphics().DestroyProgram(m_ph);
    }

    m_ph = _ph;
    m_flags.Set(eFlags::DestroyProgram, _bDestoryProgram);
}

void RenderPass::SetFrameBuffer(
    const FrameBufferHandle _fbh,
    const bool              _bDestoryFrameBuffer)
{
    if (m_fbh && (m_flags & eFlags::DestroyFrameBuffer))
    {
        GetGraphics().DestroyFrameBuffer(m_fbh);
    }

    m_fbh = _fbh;
    m_flags.Set(eFlags::DestroyFrameBuffer, _bDestoryFrameBuffer);
}

void RenderPass::SetClearRenderTargetCommand(
    const RGBA  _rgba,
    const Int32 _renderTargetIdx)
{
    JAM_ASSERT(_renderTargetIdx >= 0 && _renderTargetIdx < k_maxNumRenderTargetViewSlots, "Render target index out of range.");
    m_rgba[_renderTargetIdx] = _rgba;
    m_clearColorStart        = Min(m_clearColorStart, _renderTargetIdx);
    m_clearColorEnd          = Max(m_clearColorEnd, _renderTargetIdx);
}

void RenderPass::SetClearDepthStencilCommand(
    const bool  _bClearDepth,
    const bool  _bClearStencil,
    const float _depth,
    const UInt8 _stencil)
{
    m_flags.Set(eFlags::ClearDepth, _bClearDepth);
    m_flags.Set(eFlags::ClearStencil, _bClearStencil);

    m_clearDepth   = _depth;
    m_clearStencil = _stencil;
}

void RenderPass::SetViewport(
    const Viewport _vp)
{
    m_vp = _vp;
}

void RenderPass::SetViewport(
    const UInt16 _x,
    const UInt16 _y,
    const UInt16 _width,
    const UInt16 _height)
{
    m_vp = Viewport { _x, _y, _width, _height };
}

void RenderPass::AddConstantBuffer(
    const ConstantBufferHandle _cbh,
    const eShader              _stage,
    const Int32                _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < k_maxNumConstantBufferSlots, "Constant buffer slot out of range.");
    m_cbufferBinds.emplace_back(_cbh, _stage, _slot);
}

void RenderPass::AddTexture(
    const TextureHandle _texh,
    const eShader       _stage,
    const Int32         _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < k_maxNumShaderResourceViewSlots, "Texture slot out of range.");
    m_textureBinds.emplace_back(_texh, _stage, _slot);
}

void RenderPass::AddSampler(
    const SamplerState _state,
    const eShader      _stage,
    const Int32        _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < k_maxNumSamplerSlots, "Sampler slot out of range.");
    m_samplerBinds.emplace_back(_state, _stage, _slot);
}

void RenderPass::SetRenderState(
    const RenderState _state)
{
    m_state = _state;
}

void RenderPass::SetBlendState(
    const BlendState _state,
    const Int32      _renderTargetIdx)
{
    m_blend[_renderTargetIdx] = _state;
    m_blendStart              = Min(m_blendStart, _renderTargetIdx);
    m_blendEnd                = Max(m_blendEnd, _renderTargetIdx);
}

void RenderPass::SetStencil(
    const StencilState _fstencil,
    const StencilState _bstencil,
    const UInt8        _ref)
{
    m_fstencil   = _fstencil;
    m_bstencil   = _bstencil;
    m_stencilRef = _ref;
}

void RenderPass::SetExecutor(
    const Delegate<void()>& _executor)
{
    m_executor = _executor;
}

void RenderPass::Execute() const
{
    Execute(m_fbh, m_vp);
}

void RenderPass::Execute(
    const FrameBufferHandle _fbh,
    const Viewport          _vp) const
{
    GetGraphics().SetProgram(m_ph);
    GetGraphics().SetRenderState(m_state);
    GetGraphics().SetStencil(m_fstencil, m_bstencil, m_stencilRef);
    GetGraphics().SetFrameBuffer(_fbh);
    GetGraphics().SetViewport(_vp);

    for (const ConstantBufferBind& bind: m_cbufferBinds)
    {
        GetGraphics().SetConstantBuffer(bind.cbh, bind.stage, bind.slot);
    }

    for (const TextureBind& bind: m_textureBinds)
    {
        GetGraphics().SetTexture(bind.texh, bind.stage, bind.slot);
    }

    for (const SamplerBind& bind: m_samplerBinds)
    {
        GetGraphics().SetSampler(bind.state, bind.stage, bind.slot);
    }

    // clear render targets
    for (Int32 i = m_clearColorStart; i <= m_clearColorEnd; ++i)
    {
        GetGraphics().ClearColor(_fbh, m_rgba[i], i);
    }

    // clear depth/stencil
    const bool bClearDepth   = (m_flags & eFlags::ClearDepth);
    const bool bClearStencil = (m_flags & eFlags::ClearStencil);
    if (bClearDepth || bClearStencil)
    {
        GetGraphics().ClearDepth(_fbh, bClearDepth, bClearStencil, m_clearDepth, m_clearStencil);
    }

    // blend
    for (Int32 i = m_blendStart; i <= m_blendEnd; ++i)
    {
        GetGraphics().SetBlendState(m_blend[i], i);
    }
    GetGraphics().SetBlendFactor(m_blendFactor);

    JAM_ASSERT(m_executor, "Render pass executor is not set.");
    m_executor.Call();
}

bool RenderPass::CheckValidation() const
{
    // 프로그램이 없다면 유효하지 않음
    if (!m_ph)
    {
        return false;
    }

    // 입력과 출력 리소스가 중복되면 conflict
    {
        const FrameBufferInfo fbInfo = GetGraphics().GetFrameBufferInfo(m_fbh);

        HashSet<UInt32> texhSet = {};   // key: handle value
        texhSet.reserve(fbInfo.numRenderTargets + 1);

        // 출력 리소스 수집
        {
            // 렌더타겟 텍스처 수집
            for (Int32 i = 0; i < fbInfo.numRenderTargets; ++i)
            {
                const TextureHandle texh = fbInfo.renderTargetTexhs[i];
                JAM_ASSERT(texh, "Frame buffer has invalid render target texture handle.");
                texhSet.emplace(texh.value);
            }

            // 뎁스스텐실 텍스처 수집
            if (fbInfo.depthStencilTexh)
            {
                texhSet.emplace(fbInfo.depthStencilTexh.value);
            }
        }

        // 입력 리소스와 충돌 검사
        for (const TextureBind& texBind: m_textureBinds)
        {
            if (texhSet.contains(texBind.texh.value))
            {
                return false;
            }
        }
    }

    // A: 동일 스테이지 + 동일 리소스   -> 충돌
    // B: 동일 쉐이더 + 동일 슬롯      -> 중복 명령
    {
        HashSet<UInt64> texhStageMap = {};   // key: texture handle | stage
        texhStageMap.reserve(m_textureBinds.size());

        HashSet<UInt64> stageSlotMap = {};   // key: stage | slot
        stageSlotMap.reserve(m_textureBinds.size());

        for (const TextureBind& bind: m_textureBinds)
        {
            // 동일 스테이지에 동일 리소스가 바인딩 되는지 검사
            {
                const UInt64 key = MakeHandleStageKey_(bind.texh, bind.stage);
                if (!texhStageMap.emplace(key).second)   // dup
                {
                    return false;
                }
            }

            // 동일 쉐이더 + 동일 슬롯에 바인딩 되는지 검사
            {
                const UInt64 key = MakeStageSlotKey_(bind.stage, bind.slot);
                if (!stageSlotMap.emplace(key).second)   // dup
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
        HashSet<UInt64> cbufferStageSet = {};
        cbufferStageSet.reserve(m_cbufferBinds.size());

        // stage | slot set
        // check slot bindings for duplicates at same stage
        HashSet<UInt64> stageSlotSet = {};
        stageSlotSet.reserve(m_cbufferBinds.size());

        // check cbuffer bindings for duplicates at same stage
        for (const ConstantBufferBind& cbBind: m_cbufferBinds)
        {
            // check cbuffer bindings for duplicates at same stage
            {
                const UInt64 key = MakeHandleStageKey_(cbBind.cbh, cbBind.stage);
                if (!cbufferStageSet.emplace(key).second)   // dup
                {
                    return false;
                }
            }

            // check slot bindings for duplicates at same stage
            {
                const UInt64 key = MakeStageSlotKey_(cbBind.stage, cbBind.slot);
                if (!stageSlotSet.emplace(key).second)   // dup
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
    if (m_flags & eFlags::DestroyProgram)
    {
        JAM_DESTROY_PROGRAM(m_ph);
    }

    if (m_flags & eFlags::DestroyFrameBuffer)
    {
        JAM_DESTROY_FRAME_BUFFER(m_fbh);
    }
}

UInt64 RenderPass::MakeHandleStageKey_(
    const AnyHandle _handle,
    const eShader   _stage) const
{
    return (NumericCast<UInt64>(_handle.value) << 32) | NumericCast<UInt64>(_stage);
}

UInt64 RenderPass::MakeSamplerStageKey_(
    const SamplerState _state,
    const eShader      _stage) const
{
    return (NumericCast<UInt64>(_state.value) << 32) | NumericCast<UInt64>(_stage);
}

UInt64 RenderPass::MakeStageSlotKey_(
    const eShader _stage,
    const Int32   _slot) const
{
    return (NumericCast<UInt64>(_stage) << 32) | NumericCast<UInt64>(_slot);
}

}   // namespace Jam