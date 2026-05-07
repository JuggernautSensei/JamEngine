#include "pch.h"
#include "RenderGraph.h"
#include "Graphics.h"
#include "ImGuiEngine.h"
#include "Profile.h"

namespace Jam
{

namespace
{
    enum class eResourceAccessEx
    {
        Read,
        Write,
        ReadWrite,
    };
}

RenderGraph::~RenderGraph()
{
    Clear(eRenderGraphClearFlags::All);
}

RenderGraph::UtilityPassBuilder RenderGraph::AddUtilityPass(const StringView _name)
{
    JAM_ASSERT(!HasPass_(_name), "Pass '{}' already exists.", _name);
    PassDesc& desc = m_descs.emplace_back();
    desc.name      = _name;
    m_bNeedCompile = true;
    return UtilityPassBuilder { this, &desc };
}

RenderGraph::UtilityPassBuilder RenderGraph::ModifyUtilityPass(const StringView _name)
{
    PassDesc& desc = GetPassDesc_(_name);
    JAM_ASSERT(desc.type == ePass::Utility, "Pass '{}' is not a utility pass.", _name);
    m_bNeedCompile = true;
    return UtilityPassBuilder { this, &desc };
}

RenderGraph::RenderPassBuilder RenderGraph::AddRenderPass(const StringView _name)
{
    JAM_ASSERT(!HasPass_(_name), "Pass '{}' already exists.", _name);
    PassDesc& desc = m_descs.emplace_back();
    desc.name      = _name;
    m_bNeedCompile = true;
    return RenderPassBuilder { this, &desc };
}

RenderGraph::RenderPassBuilder RenderGraph::ModifyGrphicsPass(const StringView _name)
{
    PassDesc& desc = GetPassDesc_(_name);
    JAM_ASSERT(desc.type == ePass::Render, "Pass '{}' is not a render pass.", _name);
    m_bNeedCompile = true;
    return RenderPassBuilder { this, &desc };
}

RenderGraph::ComputePassBuilder RenderGraph::AddComputePass(const StringView _name)
{
    JAM_ASSERT(!HasPass_(_name), "Pass '{}' already exists.", _name);
    PassDesc& desc = m_descs.emplace_back();
    JAM_ASSERT(desc.type == ePass::Compute, "Newly added pass '{}' must be a compute pass.", _name);
    desc.name      = _name;
    m_bNeedCompile = true;
    return ComputePassBuilder { this, &desc };
}

RenderGraph::ComputePassBuilder RenderGraph::ModifyComputePass(const StringView _name)
{
    PassDesc& desc = GetPassDesc_(_name);
    m_bNeedCompile = true;
    return ComputePassBuilder { this, &desc };
}

void RenderGraph::RemovePass(const StringView _name)
{
    if (eastl::erase_if(m_descs, [&](const PassDesc& d) { return d.name == _name; }) == 0)
    {
        JAM_ASSERT(0, "Pass '{}' not found.", _name);
    }
    m_bNeedCompile = true;
}

String RenderGraph::GetPassNameAt(Int32 _index) const
{
    JAM_ASSERT(_index >= 0 && _index < static_cast<Int32>(m_descs.size()), "Pass index {} is out of range.", _index);
    return m_descs[_index].name;
}

StringView RenderGraph::GetPrevPassName(Int32 _step) const
{
    JAM_ASSERT(_step > 0 && _step <= static_cast<Int32>(m_descs.size()), "Invalid step {} for previous pass.", _step);
    return m_descs[m_descs.size() - _step].name;
}

////////// 컴파일 //////////

void RenderGraph::CompileIfNeed_(const Int32 _width, const Int32 _height)
{
    JAM_ASSERT(!m_descs.empty(), "RenderGraph must have at least one pass.");

    Graphics&  gfx         = GetGraphics();
    const bool bNeedResize = (m_lastWidth != _width) || (m_lastHeight != _height);

    if (!m_bNeedCompile && !bNeedResize)
    {
        return;
    }

    ////////// 텍스처/프레임버퍼 리사이즈 //////////

    if (bNeedResize)
    {
        JAM_FUNCTION_MSG_PROFILE("Resize Textures");

        m_lastWidth  = _width;
        m_lastHeight = _height;

        for (TextureResource& tex: m_textures | std::views::values)
        {
            if (!tex.bHasOwnership || !tex.bAutoResize)
            {
                continue;
            }

            const Int32 width  = static_cast<Int32>(Ceil(_width * tex.resizeScale));
            const Int32 height = static_cast<Int32>(Ceil(_height * tex.resizeScale));
            JAM_ASSERT(width > 0 && height > 0, "Auto-resize produced invalid size ({}, {}) for '{}'.", width, height, tex.name);
            JAM_DESTROY_TEXTURE(tex.handle);
            tex.handle = gfx.CreateTexture2D(width, height, tex.format, false, tex.arraySize, tex.flags);
        }

        for (FrameBufferResource& fbRes: m_frameBuffers | std::views::values)
        {
            if (!fbRes.bHasOwnership || !fbRes.bAutoResize)
            {
                continue;
            }

            Array<Attachment, kMaxNumAttachmentSlots> attachments;
            Int32                                     numAttachment = 0;

            for (Int32 i = 0; i < fbRes.numRenderTargets; ++i)
            {
                const StringView name           = fbRes.attachments[i];
                attachments[numAttachment].texh = m_textures.at(name).handle;
                ++numAttachment;
            }

            if (fbRes.bHasDepthStencil)
            {
                const String name               = fbRes.attachments[kMaxNumRenderTargetViewSlots];
                attachments[numAttachment].texh = m_textures.at(name).handle;
                ++numAttachment;
            }

            JAM_DESTROY_FRAME_BUFFER(fbRes.handle);
            fbRes.handle = gfx.CreateFrameBuffer({ attachments.data(), numAttachment }, false);
        }
    }

    // 리사이즈만 발생한 경우, 패스 구조는 그대로이므로 핸들 값만 갱신
    if (bNeedResize && !m_bNeedCompile)
    {
        JAM_FUNCTION_MSG_PROFILE("Not Compile Resize Only");

        for (Pass& pass: m_runtimePasses)
        {
            for (ResourceBindCommand& cmd: pass.readBindCmds)
            {
                if (cmd.type == eShaderResource::Texture)
                {
                    cmd.handleValue = m_textures.at(cmd.name).handle.GetValue();
                }
            }

            for (ResourceBindCommand& cmd: pass.readWriteBindCmds)
            {
                if (cmd.type == eShaderResource::Texture)
                {
                    cmd.handleValue = m_textures.at(cmd.name).handle.GetValue();
                }
            }

            PassDesc* pDesc = pass.pDesc;
            if (pDesc->type == ePass::Render)
            {
                if (pDesc->frameBufferName.IsEmpty())
                {
                    pDesc->fbh = kInvalidHandle;
                    if (pDesc->bUseFrameBufferResolutionForViewport)
                    {
                        pDesc->viewport.w = static_cast<UInt16>(_width);
                        pDesc->viewport.h = static_cast<UInt16>(_height);
                    }
                }
                else
                {
                    const FrameBufferResource& fbRes = m_frameBuffers.at(pDesc->frameBufferName);
                    pDesc->fbh                       = fbRes.handle;

                    if (pDesc->bUseFrameBufferResolutionForViewport)
                    {
                        const Vec3i resolution = gfx.GetFrameBufferResolution(pDesc->fbh);
                        pDesc->viewport.w      = static_cast<UInt16>(resolution.width);
                        pDesc->viewport.h      = static_cast<UInt16>(resolution.height);
                    }
                }
            }
        }

        return;
    }

    JAM_FUNCTION_MSG_PROFILE("Compile RenderGraph");

    ////////// 실제 리소스 핸들 매핑 //////////

    for (PassDesc& desc: m_descs)
    {
        if (!desc.bEnable)
        {
            continue;
        }

        if (!desc.programName.IsEmpty())
        {
            desc.ph = m_programs.at(desc.programName).handle;
        }

        if (!desc.computeProgramName.IsEmpty())
        {
            desc.computePh = m_programs.at(desc.computeProgramName).handle;
        }

        desc.writeResources.clear();
        if (desc.type == ePass::Render)
        {
            if (desc.frameBufferName.IsEmpty())
            {
                desc.fbh = kInvalidHandle;
                if (desc.bUseFrameBufferResolutionForViewport)
                {
                    desc.viewport.w = static_cast<UInt16>(_width);
                    desc.viewport.h = static_cast<UInt16>(_height);
                }
            }
            else
            {
                const FrameBufferResource& fbRes = m_frameBuffers.at(desc.frameBufferName);
                desc.fbh                         = fbRes.handle;

                if (desc.bUseFrameBufferResolutionForViewport)
                {
                    const Vec3i resolution = gfx.GetFrameBufferResolution(desc.fbh);
                    desc.viewport.w        = static_cast<UInt16>(resolution.width);
                    desc.viewport.h        = static_cast<UInt16>(resolution.height);
                }

                desc.writeResources.reserve(fbRes.numRenderTargets);
                for (Int32 i = 0; i < fbRes.numRenderTargets; ++i)
                {
                    desc.writeResources.insert(fbRes.attachments[i]);
                }

                if (fbRes.bHasDepthStencil)
                {
                    desc.writeResources.insert(fbRes.attachments[kMaxNumRenderTargetViewSlots]);
                }
            }
        }

        desc.readResources.clear();
        desc.readResources.reserve(desc.readBindCmds.size());
        for (ResourceBindCommand& cmd: desc.readBindCmds | std::views::values)
        {
            if (cmd.type == eShaderResource::Texture)
            {
                cmd.handleValue = m_textures.at(cmd.name).handle.GetValue();
            }
            else
            {
                cmd.handleValue = m_storageBuffers.at(cmd.name).handle.GetValue();
            }
            desc.readResources.insert(cmd.name);
        }

        desc.readWriteResources.clear();
        desc.readWriteResources.reserve(desc.readWriteBindCmds.size());
        for (ResourceBindCommand& cmd: desc.readWriteBindCmds | std::views::values)
        {
            if (cmd.type == eShaderResource::Texture)
            {
                cmd.handleValue = m_textures.at(cmd.name).handle.GetValue();
            }
            else
            {
                cmd.handleValue = m_storageBuffers.at(cmd.name).handle.GetValue();
            }
            desc.readWriteResources.insert(cmd.name);
        }

        for (ConstantBufferBindCommand& cmd: desc.cbufferBindCmds | std::views::values)
        {
            cmd.cbh = m_cbuffers.at(cmd.name).handle;
        }
    }

    ////////// 디버그 검증 //////////

    if (JAM_DEBUG_ENABLED)
    {
        StringHashMap<eResourceAccessEx> used = {};
        for (const PassDesc& desc: m_descs)
        {
            if (!desc.bEnable)
            {
                continue;
            }

            used.clear();
            used.reserve(desc.readBindCmds.size() + desc.readWriteBindCmds.size() + desc.writeResources.size());

            for (const ResourceBindCommand& cmd: desc.readBindCmds | std::views::values)
            {
                const auto it = used.find(cmd.name);
                JAM_ASSERT(it == used.end() || it->second == eResourceAccessEx::Read, "Pass '{}' binds resource '{}' multiple times with different access types.", desc.name, cmd.name);
                used[cmd.name] = eResourceAccessEx::Read;
            }

            for (const ResourceBindCommand& bind: desc.readWriteBindCmds | std::views::values)
            {
                const auto it = used.find(bind.name);
                JAM_ASSERT(it == used.end() || it->second == eResourceAccessEx::ReadWrite, "Pass '{}' binds resource '{}' multiple times with different access types.", desc.name, bind.name);
                used[bind.name] = eResourceAccessEx::ReadWrite;
            }

            for (const String& resource: desc.writeResources)
            {
                const auto it = used.find(resource);
                JAM_ASSERT(it == used.end() || it->second == eResourceAccessEx::Write, "Pass '{}' binds resource '{}' multiple times with different access types.", desc.name, resource);
                used[resource] = eResourceAccessEx::Write;
            }
        }
    }

    ////////// 의존성 그래프 구성 + 데드 패스 컬링 //////////

    {
        // 리소스를 생성(쓰기)하는 패스 집합
        StringHashMap<HashSet<Int32>> productors = {};
        for (Int32 i = 0; i < m_descs.size(); ++i)
        {
            const PassDesc& desc = m_descs[i];
            if (!desc.bEnable)
            {
                continue;
            }

            for (const String& write: desc.writeResources)
            {
                productors[write].insert(i);
            }

            for (const String& readWrite: desc.readWriteResources)
            {
                productors[readWrite].insert(i);
            }
        }

        // 역방향 의존성 간선 (i가 j의 출력을 읽으면, revGraph[i]에 j 추가)
        Vector<HashSet<Int32>> revGraph(m_descs.size());
        for (Int32 i = 0; i < m_descs.size(); ++i)
        {
            const PassDesc& desc = m_descs[i];
            if (!desc.bEnable)
            {
                continue;
            }

            for (const String& read: desc.readResources)
            {
                const auto it = productors.find(read);
                if (it != productors.end())
                {
                    for (const Int32 j: it->second)
                    {
                        if (j != i)
                        {
                            revGraph[i].insert(j);
                        }
                    }
                }
            }

            for (const String& readWrite: desc.readWriteResources)
            {
                const auto it = productors.find(readWrite);
                if (it != productors.end())
                {
                    for (const Int32 j: it->second)
                    {
                        if (j != i)
                        {
                            revGraph[i].insert(j);
                        }
                    }
                }
            }
        }

        // 최종 패스와 사이드이펙트 패스에서 역방향 BFS -> 살아남을 패스 마킹
        Vector<Int32> stack = {};
        for (Int32 i = 0; i < m_descs.size(); ++i)
        {
            const PassDesc& desc = m_descs[i];
            if (desc.bEnable && (desc.bFinalPass || desc.bSideEffect))
            {
                stack.push_back(i);
            }
        }
        JAM_ASSERT(!stack.empty(), "RenderGraph must have at least one final pass.");

        Vector<bool> survived(m_descs.size(), false);
        while (!stack.empty())
        {
            const Int32 cur = stack.back();
            stack.pop_back();

            if (survived[cur])
            {
                continue;
            }

            survived[cur] = true;

            for (const Int32 prev: revGraph[cur])
            {
                if (!survived[prev])
                {
                    stack.push_back(prev);
                }
            }
        }

        m_runtimePasses.clear();
        for (Int32 i = 0; i < m_descs.size(); ++i)
        {
            if (survived[i])
            {
                m_runtimePasses.push_back({ &m_descs[i] });
            }
        }
    }

    ////////// 패스 간 리소스 헤저드 해결 + 바인딩 최적화 //////////

    // 두 번 순회하는 이유:
    // 마지막 패스 종료 상태가 다음 프레임 첫 패스 시작 상태가 되어 충돌할 수 있음.
    // 두 번째 순회의 unbind 리스트만 최종 결과로 사용. bind 커맨드는 첫 번째 순회에서만 수집.

    struct ResourceBindingState
    {
        eResourceAccessEx         access = eResourceAccessEx::Read;
        HashSet<Slot, SlotHasher> slots  = {};
    };

    HashMap<Slot, String, SlotHasher>  readSlots      = {};
    HashMap<Slot, String, SlotHasher>  readWriteSlots = {};
    HashMap<Slot, String, SlotHasher>  cbufferSlots   = {};
    HashMap<Slot, Sampler, SlotHasher> samplerSlots   = {};

    StringHashMap<ResourceBindingState> resourceBindingStates = {};

    HashSet<Slot, SlotHasher> tmpReadUnbindSlots      = {};
    HashSet<Slot, SlotHasher> tmpReadWriteUnbindSlots = {};

    const SizeT numPasses = m_runtimePasses.size();
    for (SizeT i = 0; i < numPasses * 2; ++i)
    {
        const bool      bIsFirstLoop  = i < numPasses;
        const bool      bIsSecondLoop = !bIsFirstLoop;
        Pass&           pass          = m_runtimePasses[i % numPasses];
        const PassDesc& desc          = *pass.pDesc;

        ////////// CB/Sampler 바인드 커맨드 수집 (첫 번째 순회에서만) //////////

        if (bIsFirstLoop)
        {
            for (const ConstantBufferBindCommand& cmd: desc.cbufferBindCmds | std::views::values)
            {
                const auto it = cbufferSlots.find(cmd.slot);
                if (it == cbufferSlots.end() || it->second != cmd.name)
                {
                    pass.cbufferBindCmds.push_back(cmd);
                }
            }

            for (const SamplerFlagsBindCommand& cmd: desc.samplerBindCmds | std::views::values)
            {
                const auto it = samplerSlots.find(cmd.slot);
                if (it == samplerSlots.end() || it->second != cmd.state)
                {
                    pass.samplerBindCmds.push_back(cmd);
                }
            }
        }

        ////////// 충돌 검출 + unbind 슬롯 수집 //////////

        tmpReadUnbindSlots.clear();
        tmpReadWriteUnbindSlots.clear();

        for (const ResourceBindCommand& cmd: desc.readBindCmds | std::views::values)
        {
            bool       bAddCmd = bIsFirstLoop;
            const auto it      = resourceBindingStates.find(cmd.name);
            if (it != resourceBindingStates.end())
            {
                const ResourceBindingState& state = it->second;
                switch (state.access)
                {
                    case eResourceAccessEx::ReadWrite:
                        tmpReadWriteUnbindSlots.insert(state.slots.begin(), state.slots.end());
                        break;
                    case eResourceAccessEx::Write:
                        pass.bUnbindFrameBuffer = true;
                        break;
                    case eResourceAccessEx::Read:
                        if (state.slots.contains(cmd.slot))
                        {
                            bAddCmd = false;
                        }
                        break;
                    default:
                        JAM_UNREACHABLE;
                }
            }

            if (bAddCmd)
            {
                pass.readBindCmds.push_back(cmd);
            }
        }

        for (const ResourceBindCommand& cmd: desc.readWriteBindCmds | std::views::values)
        {
            bool       bAddCmd = bIsFirstLoop;
            const auto it      = resourceBindingStates.find(cmd.name);
            if (it != resourceBindingStates.end())
            {
                const ResourceBindingState& state = it->second;
                switch (state.access)
                {
                    case eResourceAccessEx::ReadWrite:
                        tmpReadWriteUnbindSlots.insert(state.slots.begin(), state.slots.end());
                        break;
                    case eResourceAccessEx::Write:
                        pass.bUnbindFrameBuffer = true;
                        break;
                    case eResourceAccessEx::Read:
                        if (state.slots.contains(cmd.slot))
                        {
                            bAddCmd = false;
                        }
                        break;
                    default:
                        JAM_UNREACHABLE;
                }
            }

            if (bAddCmd)
            {
                pass.readWriteBindCmds.push_back(cmd);
            }
        }

        for (const String& write: desc.writeResources)
        {
            const auto it = resourceBindingStates.find(write);
            if (it == resourceBindingStates.end())
            {
                continue;
            }

            const ResourceBindingState& state = it->second;
            switch (state.access)
            {
                case eResourceAccessEx::ReadWrite:
                    tmpReadWriteUnbindSlots.insert(state.slots.begin(), state.slots.end());
                    break;
                case eResourceAccessEx::Read:
                    tmpReadUnbindSlots.insert(state.slots.begin(), state.slots.end());
                    break;
                case eResourceAccessEx::Write:
                    JAM_ASSERT(state.slots.empty(), "write state must hold no tracked slots.");
                    break;
                default:
                    JAM_UNREACHABLE;
            }
        }

        ////////// 두 번째 순회의 unbind 리스트만 패스에 기록 //////////

        if (bIsSecondLoop)
        {
            pass.readUnbindSlots.reserve(tmpReadUnbindSlots.size());
            for (const Slot slot: tmpReadUnbindSlots)
            {
                pass.readUnbindSlots.push_back(slot);
            }

            pass.readWriteUnbindSlots.reserve(tmpReadWriteUnbindSlots.size());
            for (const Slot slot: tmpReadWriteUnbindSlots)
            {
                pass.readWriteUnbindSlots.push_back(slot);
            }
        }

        ////////// 가상 unbind — 슬롯 추적 상태 갱신 //////////

        for (const Slot& slot: tmpReadUnbindSlots)
        {
            JAM_ASSERT(readSlots.contains(slot), "read slot must be tracked.");
            const String&         name  = readSlots.at(slot);
            ResourceBindingState& state = resourceBindingStates.at(name);
            JAM_ASSERT(state.access == eResourceAccessEx::Read, "access kind must be Read for read-slot unbind.");
            JAM_ASSERT(state.slots.contains(slot), "state slots must contain the unbinding slot.");
            state.slots.erase(slot);
            if (state.slots.empty())
            {
                resourceBindingStates.erase(name);
            }
            readSlots.erase(slot);
        }

        for (const Slot& slot: tmpReadWriteUnbindSlots)
        {
            JAM_ASSERT(readWriteSlots.contains(slot), "readWrite slot must be tracked.");
            const String&         name  = readWriteSlots.at(slot);
            ResourceBindingState& state = resourceBindingStates.at(name);
            JAM_ASSERT(state.access == eResourceAccessEx::ReadWrite, "access kind must be ReadWrite for rw-slot unbind.");
            JAM_ASSERT(state.slots.contains(slot), "state slots must contain the unbinding slot.");
            state.slots.erase(slot);
            if (state.slots.empty())
            {
                resourceBindingStates.erase(name);
            }
            readWriteSlots.erase(slot);
        }

        for (const String& write: desc.writeResources)
        {
            const auto it = resourceBindingStates.find(write);
            if (it != resourceBindingStates.end())
            {
                JAM_ASSERT(it->second.slots.empty() || it->second.access == eResourceAccessEx::Write, "write resource '{}' still owns tracked slots before re-bind as write.", write);
                resourceBindingStates.erase(it);
            }
        }

        ////////// 가상 bind — 슬롯 추적 상태 갱신 //////////

        for (const ResourceBindCommand& cmd: desc.readBindCmds | std::views::values)
        {
            const auto prevIt = readSlots.find(cmd.slot);
            if (prevIt != readSlots.end() && prevIt->second != cmd.name)
            {
                ResourceBindingState& prevState = resourceBindingStates.at(prevIt->second);
                JAM_ASSERT(prevState.access == eResourceAccessEx::Read, "previous occupant of read slot must be Read.");
                JAM_ASSERT(prevState.slots.contains(cmd.slot), "previous occupant must own this slot.");
                prevState.slots.erase(cmd.slot);
                if (prevState.slots.empty())
                {
                    resourceBindingStates.erase(prevIt->second);
                }
            }

            readSlots[cmd.slot]         = cmd.name;
            ResourceBindingState& state = resourceBindingStates[cmd.name];
            state.access                = eResourceAccessEx::Read;
            state.slots.insert(cmd.slot);
        }

        for (const ResourceBindCommand& cmd: desc.readWriteBindCmds | std::views::values)
        {
            const auto prevIt = readWriteSlots.find(cmd.slot);
            if (prevIt != readWriteSlots.end() && prevIt->second != cmd.name)
            {
                ResourceBindingState& prevState = resourceBindingStates.at(prevIt->second);
                JAM_ASSERT(prevState.access == eResourceAccessEx::ReadWrite, "previous occupant of rw slot must be ReadWrite.");
                JAM_ASSERT(prevState.slots.contains(cmd.slot), "previous occupant must own this slot.");
                prevState.slots.erase(cmd.slot);
                if (prevState.slots.empty())
                {
                    resourceBindingStates.erase(prevIt->second);
                }
            }

            readWriteSlots[cmd.slot]    = cmd.name;
            ResourceBindingState& state = resourceBindingStates[cmd.name];
            state.access                = eResourceAccessEx::ReadWrite;
            state.slots.insert(cmd.slot);
        }

        for (const String& write: desc.writeResources)
        {
            ResourceBindingState& state = resourceBindingStates[write];
            JAM_ASSERT(state.slots.empty(), "write resource '{}' must not own any read/rw slots when bound as write.", write);
            state.access = eResourceAccessEx::Write;
        }

        for (const ConstantBufferBindCommand& cmd: desc.cbufferBindCmds | std::views::values)
        {
            cbufferSlots[cmd.slot] = cmd.name;
        }

        for (const SamplerFlagsBindCommand& cmd: desc.samplerBindCmds | std::views::values)
        {
            samplerSlots[cmd.slot] = cmd.state;
        }
    }

    m_bNeedCompile = false;
    gfx.Reset();
}

////////// 실행 //////////

void RenderGraph::Execute(
    const FrameBufferHandle _fbh,
    const Viewport          _vp)
{
    CompileIfNeed_(_vp.w, _vp.h);

    Graphics&   gfx       = GetGraphics();
    const SizeT numPasses = m_runtimePasses.size();

    for (SizeT i = 0; i < numPasses; ++i)
    {
        const Pass&     pass = m_runtimePasses[i];
        const PassDesc& desc = *pass.pDesc;

        if (desc.type == ePass::Utility)
        {
            if (desc.executor)
            {
                desc.executor();
            }
        }
        else if (desc.type == ePass::Compute)
        {
            ////////// Compute Pass //////////

            bool bAnyUnbound = false;
            for (const Slot slot: pass.readUnbindSlots)
            {
                gfx.SetTexture(kInvalidHandle, slot.stage, slot.slot);
                bAnyUnbound = true;
            }

            for (const Slot slot: pass.readWriteUnbindSlots)
            {
                gfx.SetTexture(kInvalidHandle, slot.stage, slot.slot, eResourceAccess::ReadWrite);
                bAnyUnbound = true;
            }

            if (bAnyUnbound)
            {
                gfx.Touch();
            }

            gfx.SetComputeProgram(desc.computePh);

            for (const ConstantBufferBindCommand& bind: pass.cbufferBindCmds)
            {
                gfx.SetConstantBuffer(bind.cbh, eShader::ComputeShader, bind.slot.slot);
            }

            for (const SamplerFlagsBindCommand& bind: pass.samplerBindCmds)
            {
                gfx.SetSampler(bind.state.flags, bind.state.rgba, eShader::ComputeShader, bind.slot.slot);
            }

            for (const ResourceBindCommand& bind: pass.readBindCmds)
            {
                if (bind.type == eShaderResource::Texture)
                {
                    gfx.SetTexture(TextureHandle { bind.handleValue }, bind.slot.stage, bind.slot.slot);
                }
                else
                {
                    gfx.SetBuffer(StorageBufferHandle { bind.handleValue }, bind.slot.stage, bind.slot.slot);
                }
            }

            for (const ResourceBindCommand& bind: pass.readWriteBindCmds)
            {
                if (bind.type == eShaderResource::Texture)
                {
                    gfx.SetTexture(TextureHandle { bind.handleValue }, bind.slot.stage, bind.slot.slot, eResourceAccess::ReadWrite);
                }
                else
                {
                    gfx.SetBuffer(StorageBufferHandle { bind.handleValue }, bind.slot.stage, bind.slot.slot, eResourceAccess::ReadWrite);
                }
            }

            gfx.Dispatch(desc.numThreadGroupsX, desc.numThreadGroupsY, desc.numThreadGroupsZ);
        }
        else
        {
            ////////// Render Pass //////////

            bool bAnyUnbind = false;
            for (const Slot& slot: pass.readUnbindSlots)
            {
                gfx.SetTexture(kInvalidHandle, slot.stage, slot.slot);
                bAnyUnbind = true;
            }

            for (const Slot& slot: pass.readWriteUnbindSlots)
            {
                gfx.SetTexture(kInvalidHandle, slot.stage, slot.slot, eResourceAccess::ReadWrite);
                bAnyUnbind = true;
            }

            if (pass.bUnbindFrameBuffer)
            {
                gfx.SetFrameBuffer(kInvalidHandle);
                bAnyUnbind = true;
            }

            if (bAnyUnbind)
            {
                gfx.Touch();
            }

            FrameBufferHandle fbh = desc.fbh;
            Viewport          vp  = desc.viewport;
            if (desc.bFinalPass)   // 최종 패스의 경우 Execute()에 전달된 프레임버퍼와 뷰포트를 사용
            {
                fbh = _fbh;
                vp  = _vp;
            }

            gfx.SetProgram(desc.ph);
            gfx.SetFrameBuffer(fbh);
            gfx.SetViewport(vp);
            gfx.SetRenderFlags(desc.renderFlags);
            gfx.SetStencilFlags(desc.fstencil, desc.bstencil, desc.stencilRef);
            gfx.SetScissorRect(desc.scissorRect);

            for (Int32 j = 0; j < desc.numBlendFlagsBindCmds; ++j)
            {
                const BlendFlagsBindCommand& bind = desc.blendFlagsBindCmds[j];
                gfx.SetBlendFlags(bind.state, bind.slot);
            }

            gfx.SetBlendFactor(desc.blendFactor);

            for (Int32 j = 0; j < desc.numRenderTargetClearCmds; ++j)
            {
                const RenderTargetClearCommand& bind = desc.renderTargetClearCmds[j];
                gfx.ClearRenderTarget(fbh, bind.color, bind.slot);
            }

            if (desc.bClearDepth || desc.bClearStencil)
            {
                gfx.ClearDepthStencil(fbh, desc.bClearDepth, desc.bClearStencil, desc.depthClearValue, desc.stencilClearValue);
            }

            for (const ConstantBufferBindCommand& bind: pass.cbufferBindCmds)
            {
                gfx.SetConstantBuffer(bind.cbh, bind.slot.stage, bind.slot.slot);
            }

            for (const SamplerFlagsBindCommand& bind: pass.samplerBindCmds)
            {
                gfx.SetSampler(bind.state.flags, bind.state.rgba, bind.slot.stage, bind.slot.slot);
            }

            for (const ResourceBindCommand& bind: pass.readBindCmds)
            {
                if (bind.type == eShaderResource::Texture)
                {
                    gfx.SetTexture(TextureHandle { bind.handleValue }, bind.slot.stage, bind.slot.slot);
                }
                else
                {
                    gfx.SetBuffer(StorageBufferHandle { bind.handleValue }, bind.slot.stage, bind.slot.slot);
                }
            }

            for (const ResourceBindCommand& bind: pass.readWriteBindCmds)
            {
                if (bind.type == eShaderResource::Texture)
                {
                    gfx.SetTexture(TextureHandle { bind.handleValue }, bind.slot.stage, bind.slot.slot, eResourceAccess::ReadWrite);
                }
                else
                {
                    gfx.SetBuffer(StorageBufferHandle { bind.handleValue }, bind.slot.stage, bind.slot.slot, eResourceAccess::ReadWrite);
                }
            }

            if (desc.executor)
            {
                desc.executor();
            }
        }
    }
}

////////// 리소스 관리 //////////

void RenderGraph::InsertTexture(
    const StringView    _name,
    const TextureHandle _texh,
    const bool          _bOwnership)
{
    TextureResource& res = m_textures[_name];
    if (res.bHasOwnership)
    {
        JAM_DESTROY_TEXTURE(res.handle);
    }
    res.name          = _name;
    res.handle        = _texh;
    res.bHasOwnership = _bOwnership;
    res.bAutoResize   = false;
    res.resizeScale   = 1.f;
    res.format        = eTextureFormat::Unknown;
    res.arraySize     = 1;
    res.flags         = {};
    m_bNeedCompile    = true;
}

void RenderGraph::EmplaceTexture2D(
    const StringView                 _name,
    const Int32                      _width,
    const Int32                      _height,
    const eTextureFormat             _format,
    const Int32                      _numLayers,
    const Flags<eTextureCreateFlags> _flags,
    const Span<const MemoryView>     _initData)
{
    Graphics&           gfx  = GetGraphics();
    const TextureHandle texh = gfx.CreateTexture2D(_width, _height, _format, false, _numLayers, _flags, _initData);
    InsertTexture(_name, texh, true);
}

void RenderGraph::EmplaceTexture2D(
    const StringView                 _name,
    const float                      _resizeScale,
    const eTextureFormat             _format,
    const Int32                      _numLayers,
    const Flags<eTextureCreateFlags> _flags)
{
    JAM_ASSERT(_resizeScale > 0.f, "resizeScale must be > 0 for '{}'.", _name);

    TextureResource& res = m_textures[_name];
    if (res.bHasOwnership)
    {
        JAM_DESTROY_TEXTURE(res.handle);
    }

    res.name          = _name;
    res.handle        = kInvalidHandle;
    res.bHasOwnership = true;
    res.bAutoResize   = true;
    res.resizeScale   = _resizeScale;
    res.format        = _format;
    res.arraySize     = _numLayers;
    res.flags         = _flags;
    m_bNeedCompile    = true;
}

void RenderGraph::EmplaceTexture3D(
    const StringView                 _name,
    const Int32                      _width,
    const Int32                      _height,
    const Int32                      _depth,
    const eTextureFormat             _format,
    const Flags<eTextureCreateFlags> _flags,
    const Span<const MemoryView>     _initData)
{
    Graphics&           gfx  = GetGraphics();
    const TextureHandle texh = gfx.CreateTexture3D(_width, _height, _depth, _format, false, _flags, _initData);
    InsertTexture(_name, texh, true);
}

void RenderGraph::EmplaceTextureCube(
    const StringView                 _name,
    const Int32                      _width,
    const Int32                      _height,
    const eTextureFormat             _format,
    const Int32                      _numCubes,
    const Flags<eTextureCreateFlags> _flags)
{
    Graphics&           gfx  = GetGraphics();
    const TextureHandle texh = gfx.CreateTextureCube(_width, _height, _format, false, _numCubes, _flags);
    InsertTexture(_name, texh, true);
}

void RenderGraph::RemoveTexture(StringView _name)
{
    const auto it = m_textures.find(_name);
    JAM_ASSERT(it != m_textures.end(), "Texture '{}' not found.", _name);
    if (it->second.bHasOwnership)
    {
        JAM_DESTROY_TEXTURE(it->second.handle);
    }
    m_textures.erase(it);
    m_bNeedCompile = true;
}

TextureHandle RenderGraph::GetTexture(const StringView _name) const
{
    const TextureResource& texRes = m_textures.at(_name);
    return texRes.handle;
}

bool RenderGraph::HasTexture(
    const StringView _name) const
{
    return m_textures.contains(_name);
}

////////// 리소스 관리 — 상수 버퍼 //////////

void RenderGraph::InsertConstantBuffer(
    const StringView           _name,
    const ConstantBufferHandle _cbh,
    const bool                 _bOwnership)
{
    ConstantBufferResource& res = m_cbuffers[_name];
    if (res.bHasOwnership)
    {
        JAM_DESTROY_CONSTANT_BUFFER(res.handle);
    }
    res.name          = _name;
    res.handle        = _cbh;
    res.bHasOwnership = _bOwnership;
    m_bNeedCompile    = true;
}

void RenderGraph::EmplaceConstantBuffer(
    const StringView _name,
    const UInt32     _byteWidth)
{
    Graphics&                  gfx = GetGraphics();
    const ConstantBufferHandle cbh = gfx.CreateConstantBuffer(_byteWidth);
    JAM_ASSERT(cbh != kInvalidHandle, "Failed to create constant buffer '{}'.", _name);
    InsertConstantBuffer(_name, cbh, true);
}

void RenderGraph::RemoveConstantBuffer(const StringView _name)
{
    const auto it = m_cbuffers.find(_name);
    JAM_ASSERT(it != m_cbuffers.end(), "ConstantBuffer '{}' not found.", _name);
    if (it->second.bHasOwnership)
    {
        JAM_DESTROY_CONSTANT_BUFFER(it->second.handle);
    }
    m_cbuffers.erase(it);
    m_bNeedCompile = true;
}

ConstantBufferHandle RenderGraph::GetConstantBuffer(const StringView _name) const
{
    const ConstantBufferResource& cbRes = m_cbuffers.at(_name);
    return cbRes.handle;
}

bool RenderGraph::HasConstantBuffer(const StringView _name) const
{
    return m_cbuffers.contains(_name);
}

////////// 리소스 관리 — 스토리지 버퍼 //////////

void RenderGraph::InsertStorageBuffer(
    const StringView          _name,
    const StorageBufferHandle _sbh,
    const bool                _bOwnership)
{
    JAM_ASSERT(!m_storageBuffers.contains(_name), "StorageBuffer '{}' already registered.", _name);

    StorageBufferResource& res = m_storageBuffers[_name];
    if (res.bHasOwnership)
    {
        JAM_DESTROY_STORAGE_BUFFER(res.handle);
    }
    res.name          = _name;
    res.handle        = _sbh;
    res.bHasOwnership = _bOwnership;
    m_bNeedCompile    = true;
}

void RenderGraph::EmplaceRawBuffer(
    const StringView                       _name,
    const Int32                            _byteWidth,
    const Flags<eStorageBufferCreateFlags> _flags,
    const MemoryView                       _initDataOrEmpty)
{
    Graphics&                 gfx = GetGraphics();
    const StorageBufferHandle sbh = gfx.CreateRawBuffer(_byteWidth, _flags, _initDataOrEmpty);
    InsertStorageBuffer(_name, sbh, true);
}

void RenderGraph::EmplaceStructuredBuffer(
    const StringView                       _name,
    const Int32                            _numElements,
    const Int32                            _stride,
    const bool                             _bAppendConsume,
    const bool                             _bCounter,
    const Flags<eStorageBufferCreateFlags> _flags,
    const MemoryView                       _initDataOrEmpty)
{
    Graphics&                 gfx = GetGraphics();
    const StorageBufferHandle sbh = gfx.CreateStructuredBuffer(_numElements, _stride, _bAppendConsume, _bCounter, _flags, _initDataOrEmpty);
    InsertStorageBuffer(_name, sbh, true);
}

void RenderGraph::EmplaceTypedBuffer(
    const StringView                       _name,
    const Int32                            _numElements,
    const eStorageBufferData               _dataType,
    const Flags<eStorageBufferCreateFlags> _flags,
    const MemoryView                       _initDataOrEmpty)
{
    Graphics&                 gfx = GetGraphics();
    const StorageBufferHandle sbh = gfx.CreateTypedBuffer(_numElements, _dataType, _flags, _initDataOrEmpty);
    InsertStorageBuffer(_name, sbh, true);
}

void RenderGraph::RemoveStorageBuffer(StringView _name)
{
    const auto it = m_storageBuffers.find(_name);
    JAM_ASSERT(it != m_storageBuffers.end(), "StorageBuffer '{}' not found.", _name);
    if (it->second.bHasOwnership)
    {
        JAM_DESTROY_STORAGE_BUFFER(it->second.handle);
    }
    m_storageBuffers.erase(it);
    m_bNeedCompile = true;
}

StorageBufferHandle RenderGraph::GetStorageBuffer(const StringView _name) const
{
    const StorageBufferResource& sbRes = m_storageBuffers.at(_name);
    JAM_ASSERT(sbRes.handle != kInvalidHandle, "StorageBuffer '{}' not created.", _name);
    return sbRes.handle;
}

bool RenderGraph::HasStorageBuffer(const StringView _name) const
{
    return m_storageBuffers.contains(_name);
}

////////// 리소스 관리 — 프레임버퍼 //////////

void RenderGraph::InsertFrameBuffer(
    const StringView        _name,
    const FrameBufferHandle _fbh,
    const bool              _bOwnership)
{
    FrameBufferResource& fbRes = m_frameBuffers[_name];
    if (fbRes.bHasOwnership)
    {
        JAM_DESTROY_FRAME_BUFFER(fbRes.handle);
    }
    fbRes.name          = _name;
    fbRes.handle        = _fbh;
    fbRes.bHasOwnership = _bOwnership;
    fbRes.attachments.fill({});
    fbRes.numRenderTargets = 0;
    fbRes.bHasDepthStencil = false;
    fbRes.bAutoResize      = false;
    m_bNeedCompile         = true;
}

void RenderGraph::EmplaceFrameBuffer(
    const StringView               _name,
    const Span<const AttachmentRG> _attachments)
{
    JAM_ASSERT(!_attachments.empty(), "FrameBuffer '{}' must have at least one attachment.", _name);

    FrameBufferResource& fbRes = m_frameBuffers[_name];
    if (fbRes.bHasOwnership)
    {
        JAM_DESTROY_FRAME_BUFFER(fbRes.handle);
    }

    fbRes.name          = _name;
    fbRes.bHasOwnership = true;
    fbRes.bAutoResize   = false;

    for (const AttachmentRG& attachment: _attachments)
    {
        const TextureResource& texRes = m_textures.at(attachment.name);
        if (IsDepthFormat(texRes.format))
        {
            JAM_ASSERT(!fbRes.bHasDepthStencil, "FrameBuffer '{}' cannot have multiple depth/stencil attachments. Attachment '{}' is a duplicate.", _name, attachment.name);
            fbRes.attachments[kDepthStencilAttachmentSlot] = attachment.name;
            fbRes.bHasDepthStencil                         = true;
        }
        else
        {
            fbRes.attachments[fbRes.numRenderTargets++] = attachment.name;
        }

        fbRes.bAutoResize |= texRes.bAutoResize;
        JAM_ASSERT(fbRes.bAutoResize == texRes.bAutoResize, "All attachments of FrameBuffer '{}' must have the same auto-resize setting. Attachment '{}' differs.", _name, attachment.name);
    }

    // auto-resize인 경우 CompileIfNeed_ 에서 지연 생성. 아닐경우 즉시 생성
    if (!fbRes.bAutoResize)
    {
        Array<Attachment, kMaxNumAttachmentSlots> attachments    = {};
        Int32                                     numAttachments = 0;
        for (const AttachmentRG& attachment: _attachments)
        {
            const TextureResource& texRes = m_textures.at(attachment.name);
            JAM_ASSERT(texRes.handle != kInvalidHandle, "Texture '{}' for FrameBuffer '{}' is not created.", attachment.name, _name);
            attachments[numAttachments].texh        = texRes.handle;
            attachments[numAttachments].mipIndex    = attachment.mip;
            attachments[numAttachments].layerOffset = attachment.layerOffset;
            attachments[numAttachments].numLayers   = attachment.numLayers;
            ++numAttachments;
        }

        Graphics& gfx = GetGraphics();
        fbRes.handle  = gfx.CreateFrameBuffer({ attachments.data(), numAttachments }, false);
    }
    m_bNeedCompile = true;
}

void RenderGraph::EmplaceFrameBuffer(
    const StringView _name,
    const StringView _attachment)
{
    AttachmentRG attahment[1] = {};
    attahment[0].name         = _attachment;
    EmplaceFrameBuffer(_name, attahment);
}

void RenderGraph::RemoveFrameBuffer(const StringView _name)
{
    const auto it = m_frameBuffers.find(_name);
    JAM_ASSERT(it != m_frameBuffers.end(), "FrameBuffer '{}' not found.", _name);
    if (it->second.bHasOwnership)
    {
        JAM_DESTROY_FRAME_BUFFER(it->second.handle);
    }
    m_frameBuffers.erase(it);
    m_bNeedCompile = true;
}

FrameBufferHandle RenderGraph::GetFrameBuffer(const StringView _name) const
{
    const FrameBufferResource& fbRes = m_frameBuffers.at(_name);
    return fbRes.handle;
}

bool RenderGraph::HasFrameBuffer(const StringView _name) const
{
    return m_frameBuffers.contains(_name);
}

////////// 리소스 관리 — 셰이더 //////////

void RenderGraph::InsertShader(
    const StringView   _name,
    const ShaderHandle _sh,
    const bool         _bOwnership)
{
    ShaderResource& res = m_shaders[_name];
    res.name            = _name;
    res.handle          = _sh;
    res.bHasOwnership   = _bOwnership;
    m_bNeedCompile      = true;
}

void RenderGraph::EmplaceShader(
    const StringView _name,
    const MemoryView _bytecode)
{
    Graphics& gfx = GetGraphics();
    InsertShader(_name, gfx.CreateShader(_bytecode), true);
}

void RenderGraph::RemoveShader(const StringView _name)
{
    const auto it = m_shaders.find(_name);
    JAM_ASSERT(it != m_shaders.end(), "Shader '{}' not found.", _name);
    if (it->second.bHasOwnership)
    {
        JAM_DESTROY_SHADER(it->second.handle);
    }
    m_shaders.erase(it);
    m_bNeedCompile = true;
}

ShaderHandle RenderGraph::GetShader(const StringView _name) const
{
    const ShaderResource& shRes = m_shaders.at(_name);
    return shRes.handle;
}

bool RenderGraph::HasShader(const StringView _name) const
{
    return m_shaders.contains(_name);
}

bool RenderGraph::HasProgram(const StringView _name) const
{
    return m_programs.contains(_name);
}

////////// 리소스 관리 — 프로그램 //////////

void RenderGraph::InsertProgram(
    const StringView    _name,
    const ProgramHandle _ph,
    const bool          _bOwnership)
{
    ProgramResource& res = m_programs[_name];
    if (res.bHasOwnership)
    {
        JAM_DESTROY_PROGRAM(res.handle);
    }
    res.name          = _name;
    res.handle        = _ph;
    res.bHasOwnership = _bOwnership;
    m_bNeedCompile    = true;
}

void RenderGraph::EmplaceProgram(
    const StringView _name,
    const StringView _vsOrEmpty,
    const StringView _psOrEmpty)
{
    EmplaceProgram(_name, _vsOrEmpty, _psOrEmpty, {}, {}, {});
}

void RenderGraph::EmplaceProgram(
    const StringView _name,
    const StringView _vsOrEmpty,
    const StringView _psOrEmpty,
    const StringView _gsOrEmpty,
    const StringView _hsOrEmpty,
    const StringView _dsOrEmpty)
{
    const ShaderHandle  vsh = _vsOrEmpty.IsEmpty() ? kInvalidHandle : GetShader(_vsOrEmpty);
    const ShaderHandle  psh = _psOrEmpty.IsEmpty() ? kInvalidHandle : GetShader(_psOrEmpty);
    const ShaderHandle  gsh = _gsOrEmpty.IsEmpty() ? kInvalidHandle : GetShader(_gsOrEmpty);
    const ShaderHandle  hsh = _hsOrEmpty.IsEmpty() ? kInvalidHandle : GetShader(_hsOrEmpty);
    const ShaderHandle  dsh = _dsOrEmpty.IsEmpty() ? kInvalidHandle : GetShader(_dsOrEmpty);
    const ProgramHandle ph  = GetGraphics().CreateProgram(vsh, psh, gsh, hsh, dsh);
    InsertProgram(_name, ph, true);
}

void RenderGraph::EmplaceComputeProgram(
    const StringView _name,
    const StringView _csName)
{
    const ShaderHandle  csh = GetShader(_csName);
    const ProgramHandle ph  = GetGraphics().CreateProgram(csh);
    InsertProgram(_name, ph, true);
}

void RenderGraph::RemoveProgram(StringView _name)
{
    const auto it = m_programs.find(_name);
    JAM_ASSERT(it != m_programs.end(), "Program '{}' not found.", _name);
    if (it->second.bHasOwnership)
    {
        JAM_DESTROY_PROGRAM(it->second.handle);
    }
    m_programs.erase(it);
    m_bNeedCompile = true;
}

ProgramHandle RenderGraph::GetProgram(const StringView _name) const
{
    const ProgramResource& progRes = m_programs.at(_name);
    return progRes.handle;
}

////////// 전체 초기화 //////////

void RenderGraph::Clear(const Flags<eRenderGraphClearFlags> _flags)
{
    m_bNeedCompile = true;

    if (_flags.IsEnabled(eRenderGraphClearFlags::Pass))
    {
        m_descs.clear();
        m_runtimePasses.clear();
    }

    if (_flags.IsEnabled(eRenderGraphClearFlags::Resource))
    {
        for (TextureResource& res: m_textures | std::views::values)
        {
            if (res.bHasOwnership)
            {
                JAM_DESTROY_TEXTURE(res.handle);
            }
        }
        m_textures.clear();

        for (FrameBufferResource& res: m_frameBuffers | std::views::values)
        {
            if (res.bHasOwnership)
            {
                JAM_DESTROY_FRAME_BUFFER(res.handle);
            }
        }
        m_frameBuffers.clear();

        for (ConstantBufferResource& res: m_cbuffers | std::views::values)
        {
            if (res.bHasOwnership)
            {
                JAM_DESTROY_CONSTANT_BUFFER(res.handle);
            }
        }
        m_cbuffers.clear();

        for (StorageBufferResource& res: m_storageBuffers | std::views::values)
        {
            if (res.bHasOwnership)
            {
                JAM_DESTROY_STORAGE_BUFFER(res.handle);
            }
        }
        m_storageBuffers.clear();

        for (ShaderResource& res: m_shaders | std::views::values)
        {
            if (res.bHasOwnership)
            {
                JAM_DESTROY_SHADER(res.handle);
            }
        }
        m_shaders.clear();

        for (ProgramResource& res: m_programs | std::views::values)
        {
            if (res.bHasOwnership)
            {
                JAM_DESTROY_PROGRAM(res.handle);
            }
        }
        m_programs.clear();
    }
}

RenderGraph::PassDesc& RenderGraph::GetPassDesc_(const StringView name)
{
    const auto it = std::ranges::find_if(m_descs, [name](const PassDesc& _desc) { return _desc.name == name; });
    JAM_ASSERT(it != m_descs.end(), "Pass '{}' not found.", name);
    return *it;
}

bool RenderGraph::HasPass_(StringView name) const
{
    return std::ranges::any_of(m_descs, [name](const PassDesc& pd) { return pd.name == name; });
}

////////// UTILITY PASS BUILDER //////////

RenderGraph::UtilityPassBuilder::UtilityPassBuilder(
    RenderGraph* _pGraph,
    PassDesc*    _pDesc)
    : m_pGraph(_pGraph)
    , m_pDesc(_pDesc)
{
    m_pDesc->type        = ePass::Utility;
    m_pDesc->bSideEffect = true;
}

RenderGraph::UtilityPassBuilder& RenderGraph::UtilityPassBuilder::Enable(const bool _bEnable)
{
    m_pDesc->bEnable = _bEnable;
    return *this;
}

RenderGraph::UtilityPassBuilder& RenderGraph::UtilityPassBuilder::SetExecutor(const Delegate<void()>& _executor)
{
    JAM_ASSERT(_executor, "Executor cannot be null.");
    m_pDesc->executor = _executor;
    return *this;
}

////////// GRAPHICS PASS BUILDER //////////

RenderGraph::RenderPassBuilder::RenderPassBuilder(
    RenderGraph* _pGraph,
    PassDesc*    _pDesc)
    : m_pGraph(_pGraph)
    , m_pDesc(_pDesc)
{
    m_pDesc->type = ePass::Render;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::AsFinalPass(const bool _bEnable)
{
    m_pDesc->bFinalPass = _bEnable;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::Enable(const bool _bEnable)
{
    m_pDesc->bEnable = _bEnable;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetProgram(const StringView _name)
{
    JAM_ASSERT(!_name.IsEmpty(), "Program name cannot be empty.");
    m_pDesc->programName = _name;
    m_pDesc->ph          = kInvalidHandle;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::ReleaseProgram()
{
    m_pDesc->programName.Clear();
    m_pDesc->ph = kInvalidHandle;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetFrameBuffer(const StringView _name)
{
    JAM_ASSERT(!_name.IsEmpty(), "FrameBuffer name cannot be empty.");
    m_pDesc->frameBufferName = _name;
    m_pDesc->fbh             = kInvalidHandle;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::ReleaseFrameBuffer()
{
    m_pDesc->frameBufferName.Clear();
    m_pDesc->fbh = kInvalidHandle;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetViewport(const Viewport _vp)
{
    m_pDesc->viewport                             = _vp;
    m_pDesc->bUseFrameBufferResolutionForViewport = false;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetFrameBufferResolutionForViewport()
{
    m_pDesc->bUseFrameBufferResolutionForViewport = true;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetScissorRect(const Rect _rect)
{
    m_pDesc->scissorRect = _rect;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetRenderFlags(const Flags<eRenderFlags> _flags)
{
    m_pDesc->renderFlags = _flags;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetStencilFlags(
    const Flags<eStencilFlags> _fstencil,
    const Flags<eStencilFlags> _bstencil,
    const UInt8                _ref)
{
    m_pDesc->fstencil   = _fstencil;
    m_pDesc->bstencil   = _bstencil;
    m_pDesc->stencilRef = _ref;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetBlendFlags(
    const Flags<eBlendFlags> _flags,
    const Int32              _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < kMaxNumRenderTargetViewSlots, "Blend slot {} out of range.", _slot);

    for (Int32 i = 0; i < m_pDesc->numBlendFlagsBindCmds; ++i)
    {
        if (m_pDesc->blendFlagsBindCmds[i].slot == _slot)
        {
            m_pDesc->blendFlagsBindCmds[i].state = _flags;
            return *this;
        }
    }

    JAM_ASSERT(m_pDesc->numBlendFlagsBindCmds < kMaxNumRenderTargetViewSlots, "Too many blend states.");
    m_pDesc->blendFlagsBindCmds[m_pDesc->numBlendFlagsBindCmds++] = { _flags, _slot };
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetBlendFactor(const RGBA _factor)
{
    m_pDesc->blendFactor = _factor;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetRenderTargetClearCommand(
    const RGBA  _color,
    const Int32 _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < kMaxNumRenderTargetViewSlots, "ClearColor slot {} out of range.", _slot);

    for (Int32 i = 0; i < m_pDesc->numRenderTargetClearCmds; ++i)
    {
        if (m_pDesc->renderTargetClearCmds[i].slot == _slot)
        {
            m_pDesc->renderTargetClearCmds[i].color = _color;
            return *this;
        }
    }

    m_pDesc->renderTargetClearCmds[m_pDesc->numRenderTargetClearCmds++] = { _color, _slot };
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::ReleaseRenderTargetCommand(const Int32 _slot)
{
    for (Int32 i = 0; i < m_pDesc->numRenderTargetClearCmds; ++i)
    {
        if (m_pDesc->renderTargetClearCmds[i].slot == _slot)
        {
            const Int32 last                     = m_pDesc->numRenderTargetClearCmds - 1;
            m_pDesc->renderTargetClearCmds[i]    = m_pDesc->renderTargetClearCmds[last];
            m_pDesc->renderTargetClearCmds[last] = {};
            --m_pDesc->numRenderTargetClearCmds;
            break;
        }
    }
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetDepthClearCommand(const float _value)
{
    m_pDesc->depthClearValue = _value;
    m_pDesc->bClearDepth     = true;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::ReleaseDepthClearCommand()
{
    m_pDesc->bClearDepth = false;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetStencilClearCommand(const UInt8 _value)
{
    m_pDesc->stencilClearValue = _value;
    m_pDesc->bClearStencil     = true;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::ReleaseStencilClearCommand()
{
    m_pDesc->bClearStencil = false;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetTexture(
    const StringView      _name,
    const eShader         _stage,
    const Int32           _slot,
    const eResourceAccess _access)
{
    JAM_ASSERT(m_pGraph->m_textures.contains(_name), "Texture '{}' not registered.", _name);

    const Slot slot = { _stage, _slot };
    if (_access == eResourceAccess::Read)
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumShaderResourceViewSlots, "Texture slot {} out of range.", _slot);
        m_pDesc->readBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Texture, _stage, _slot };
    }
    else
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumUnorderedAccessViewSlots, "Texture slot {} out of range.", _slot);
        m_pDesc->readWriteBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Texture, _stage, _slot };
    }
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetBuffer(
    const StringView      _name,
    const eShader         _stage,
    const Int32           _slot,
    const eResourceAccess _access)
{
    JAM_ASSERT(m_pGraph->m_storageBuffers.contains(_name), "StorageBuffer '{}' not registered.", _name);

    const Slot slot = { _stage, _slot };
    if (_access == eResourceAccess::Read)
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumShaderResourceViewSlots, "Buffer slot {} out of range.", _slot);
        m_pDesc->readBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Buffer, _stage, _slot };
    }
    else
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumUnorderedAccessViewSlots, "Buffer slot {} out of range.", _slot);
        m_pDesc->readWriteBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Buffer, _stage, _slot };
    }
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::ReleaseResource(
    const eShader         _stage,
    const Int32           _slot,
    const eResourceAccess _access)
{
    const Slot slot = { _stage, _slot };
    if (_access == eResourceAccess::Read)
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumShaderResourceViewSlots, "Resource slot {} out of range.", _slot);
        m_pDesc->readBindCmds.erase(slot);
    }
    else
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumUnorderedAccessViewSlots, "Resource slot {} out of range.", _slot);
        m_pDesc->readWriteBindCmds.erase(slot);
    }
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetConstantBuffer(
    const StringView _name,
    const eShader    _stage,
    const Int32      _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < kMaxNumConstantBufferSlots, "CBuffer slot {} out of range.", _slot);
    JAM_ASSERT(m_pGraph->m_cbuffers.contains(_name), "ConstantBuffer '{}' not registered.", _name);
    const Slot slot                = { _stage, _slot };
    m_pDesc->cbufferBindCmds[slot] = { _name, kInvalidHandle, slot };
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::ReleaseConstantBuffer(
    const eShader _stage,
    const Int32   _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < kMaxNumConstantBufferSlots, "CBuffer slot {} out of range.", _slot);
    m_pDesc->cbufferBindCmds.erase({ _stage, _slot });
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetSampler(
    const Flags<eSamplerFlags> _state,
    const eShader              _stage,
    const Int32                _slot)
{
    return SetSampler(_state, RGBA::kBlack, _stage, _slot);
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetSampler(
    const Flags<eSamplerFlags> _state,
    const RGBA                 _borderColor,
    const eShader              _stage,
    const Int32                _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < kMaxNumSamplerSlots, "Sampler slot {} out of range.", _slot);
    const Slot slot                = { _stage, _slot };
    m_pDesc->samplerBindCmds[slot] = { _state, _borderColor, _stage, _slot };
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::ReleaseSamplerFlags(
    const eShader _stage,
    const Int32   _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < kMaxNumSamplerSlots, "Sampler slot {} out of range.", _slot);
    m_pDesc->samplerBindCmds.erase({ _stage, _slot });
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetExecutor(const Delegate<void()>& _executor)
{
    JAM_ASSERT(_executor, "Executor cannot be null.");
    m_pDesc->executor = _executor;
    return *this;
}

////////// COMPUTE PASS BUILDER //////////

RenderGraph::ComputePassBuilder::ComputePassBuilder(
    RenderGraph* _pGraph,
    PassDesc*    _pDesc)
    : m_pGraph(_pGraph)
    , m_pDesc(_pDesc)
{
    _pDesc->type = ePass::Compute;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::AsFinalPass(const bool _bEnable)
{
    m_pDesc->bFinalPass = _bEnable;
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::Enable(const bool _bEnable)
{
    m_pDesc->bEnable = _bEnable;
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::SetComputeProgram(const StringView _name)
{
    JAM_ASSERT(!_name.IsEmpty(), "Compute program name cannot be empty.");
    m_pDesc->computeProgramName = _name;
    m_pDesc->computePh          = kInvalidHandle;
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::ReleaseComputeProgram()
{
    m_pDesc->computeProgramName.Clear();
    m_pDesc->computePh = kInvalidHandle;
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::SetTexture(
    const StringView      _name,
    const Int32           _slot,
    const eResourceAccess _access)
{
    const Slot slot = { eShader::ComputeShader, _slot };
    if (_access == eResourceAccess::Read)
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumShaderResourceViewSlots, "Texture slot {} out of range.", _slot);
        m_pDesc->readBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Texture, slot };
    }
    else
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumUnorderedAccessViewSlots, "Texture slot {} out of range.", _slot);
        m_pDesc->readWriteBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Texture, slot };
    }
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::SetBuffer(
    const StringView      _name,
    const Int32           _slot,
    const eResourceAccess _access)
{
    const Slot slot = { eShader::ComputeShader, _slot };
    if (_access == eResourceAccess::Read)
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumShaderResourceViewSlots, "Buffer slot {} out of range.", _slot);
        m_pDesc->readBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Buffer, slot };
    }
    else
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumUnorderedAccessViewSlots, "Buffer slot {} out of range.", _slot);
        m_pDesc->readWriteBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Buffer, slot };
    }
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::ReleaseResource(
    const Int32           _slot,
    const eResourceAccess _access)
{
    const Slot slot = { eShader::ComputeShader, _slot };
    if (_access == eResourceAccess::Read)
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumShaderResourceViewSlots, "Buffer slot {} out of range.", _slot);
        m_pDesc->readBindCmds.erase(slot);
    }
    else
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumUnorderedAccessViewSlots, "Buffer slot {} out of range.", _slot);
        m_pDesc->readWriteBindCmds.erase(slot);
    }
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::SetConstantBuffer(
    const StringView _name,
    const Int32      _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < kMaxNumConstantBufferSlots, "CBuffer slot {} out of range.", _slot);
    JAM_ASSERT(m_pGraph->m_cbuffers.contains(_name), "ConstantBuffer '{}' not registered.", _name);
    const Slot slot                = { eShader::ComputeShader, _slot };
    m_pDesc->cbufferBindCmds[slot] = { _name, kInvalidHandle, eShader::ComputeShader, _slot };
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::ReleaseConstantBuffer(const Int32 _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < kMaxNumConstantBufferSlots, "CBuffer slot {} out of range.", _slot);
    m_pDesc->cbufferBindCmds.erase({ eShader::ComputeShader, _slot });
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::ReleaseSamplerState(const Int32 _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < kMaxNumSamplerSlots, "Sampler slot {} out of range.", _slot);
    m_pDesc->samplerBindCmds.erase({ eShader::ComputeShader, _slot });
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::SetSampler(
    const Flags<eSamplerFlags> _state,
    const Int32                _slot)
{
    return SetSampler(_state, RGBA::kBlack, _slot);
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::SetSampler(
    const Flags<eSamplerFlags> _state,
    const RGBA                 _borderColor,
    const Int32                _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < kMaxNumSamplerSlots, "Sampler slot {} out of range.", _slot);
    const Slot slot                = { eShader::ComputeShader, _slot };
    m_pDesc->samplerBindCmds[slot] = { _state, _borderColor, eShader::ComputeShader, _slot };
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::SetNumThreadGroups(
    const Int32 _numThreadGroupsX,
    const Int32 _numThreadGroupsY,
    const Int32 _numThreadGroupsZ)
{
    JAM_ASSERT(_numThreadGroupsX > 0 && _numThreadGroupsY > 0 && _numThreadGroupsZ > 0, "Number of thread groups must be greater than zero.");
    m_pDesc->numThreadGroupsX = _numThreadGroupsX;
    m_pDesc->numThreadGroupsY = _numThreadGroupsY;
    m_pDesc->numThreadGroupsZ = _numThreadGroupsZ;
    return *this;
}

}   // namespace Jam