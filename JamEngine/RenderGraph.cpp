#include "pch.h"
#include "RenderGraph.h"
#include "Graphics.h"
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
    PassInfo& info = m_infos.emplace_back();
    info.name      = _name;
    m_bNeedCompile = true;
    return UtilityPassBuilder { this, &info };
}

RenderGraph::UtilityPassBuilder RenderGraph::ModifyUtilityPass(const StringView _name)
{
    PassInfo& info = GetPassDesc_(_name);
    JAM_ASSERT(info.type == ePass::Utility, "Pass '{}' is not a utility pass.", _name);
    m_bNeedCompile = true;
    return UtilityPassBuilder { this, &info };
}

RenderGraph::RenderPassBuilder RenderGraph::AddRenderPass(const StringView _name)
{
    JAM_ASSERT(!HasPass_(_name), "Pass '{}' already exists.", _name);
    PassInfo& info = m_infos.emplace_back();
    info.name      = _name;
    m_bNeedCompile = true;
    return RenderPassBuilder { this, &info };
}

RenderGraph::RenderPassBuilder RenderGraph::ModifyRenderPass(const StringView _name)
{
    PassInfo& info = GetPassDesc_(_name);
    m_bNeedCompile = true;
    return RenderPassBuilder { this, &info };
}

RenderGraph::ComputePassBuilder RenderGraph::AddComputePass(const StringView _name)
{
    JAM_ASSERT(!HasPass_(_name), "Pass '{}' already exists.", _name);
    PassInfo& info = m_infos.emplace_back();
    info.name      = _name;
    m_bNeedCompile = true;
    return ComputePassBuilder { this, &info };
}

RenderGraph::ComputePassBuilder RenderGraph::ModifyComputePass(const StringView _name)
{
    PassInfo& info = GetPassDesc_(_name);
    m_bNeedCompile = true;
    return ComputePassBuilder { this, &info };
}

void RenderGraph::RemovePass(const StringView _name)
{
    if (eastl::erase_if(m_infos, [&](const PassInfo& d) { return d.name == _name; }) == 0)
    {
        JAM_ASSERT(0, "Pass '{}' not found.", _name);
    }
    m_bNeedCompile = true;
}

StringView RenderGraph::GetLastPass(Int32 _step) const
{
    JAM_ASSERT(_step > 0 && _step <= static_cast<Int32>(m_infos.size()), "Invalid step {} for previous pass.", _step);
    return m_infos[m_infos.size() - _step].name;
}

StringView RenderGraph::GetPassAttachment(
    const StringView _passName,
    const Int32      _slot) const
{
    const PassInfo&            info  = GetPassDesc_(_passName);
    const FrameBufferResource& fbres = m_frameBuffers.at(info.frameBufferName);
    JAM_ASSERT((0 <= _slot && _slot < fbres.numRenderTargets) || (_slot == kDepthStencilSlot && fbres.bHasDepthStencil), "Invalid slot {} for attachment in pass '{}'.", _slot, _passName);
    return fbres.attachments[_slot];
}

////////// 컴파일 //////////

void RenderGraph::CompileIfNeed_(const Int32 _width, const Int32 _height)
{
    JAM_ASSERT(!m_infos.empty(), "RenderGraph must have at least one pass.");

    Graphics&  gfx         = GetGraphics();
    const bool bNeedResize = (m_lastWidth != _width) || (m_lastHeight != _height);

    if (!m_bNeedCompile && !bNeedResize)
    {
        return;
    }

    // 텍스처, 프레임버퍼 리사이즈

    if (bNeedResize)
    {
        JAM_FUNCTION_MSG_PROFILE("Resize Textures");

        m_lastWidth  = _width;
        m_lastHeight = _height;

        for (TextureResource& tres: m_textures | std::views::values)
        {
            if (!tres.bHasOwnership || !tres.bAutoResize)
            {
                continue;
            }

            JAM_DESTROY_TEXTURE(tres.handle);

            const Int32 width  = static_cast<Int32>(Ceil(_width * tres.resizeScale));
            const Int32 height = static_cast<Int32>(Ceil(_height * tres.resizeScale));
            JAM_ASSERT(width > 0 && height > 0, "Auto-resize produced invalid size ({}, {}) for '{}'.", width, height, tres.name);
            JAM_ASSERT(tres.type != eTexture::Texture3D, "Auto-resize is not supported for 3D textures. Texture '{}'", tres.name);

            if (tres.type == eTexture::Texture2D)
            {
                tres.handle = gfx.CreateTexture2D(width, height, tres.format, false, tres.numLayers, tres.flags);
            }
            else if (tres.type == eTexture::TextureCube)
            {
                tres.handle = gfx.CreateTextureCube(width, height, tres.format, false, tres.numLayers, tres.flags);
            }
        }

        for (FrameBufferResource& fbres: m_frameBuffers | std::views::values)
        {
            if (!fbres.bHasOwnership || !fbres.bAutoResize)
            {
                continue;
            }

            JAM_DESTROY_FRAME_BUFFER(fbres.handle);

            Array<Attachment, kMaxNumAttachmentSlots> attachments   = {};
            Int32                                     numAttachment = 0;
            for (Int32 i = 0; i < fbres.numRenderTargets; ++i)
            {
                const StringView name           = fbres.attachments[i];
                attachments[numAttachment].texh = m_textures.at(name).handle;
                ++numAttachment;
            }

            if (fbres.bHasDepthStencil)
            {
                const String name               = fbres.attachments[kMaxNumRenderTargetViewSlots];
                attachments[numAttachment].texh = m_textures.at(name).handle;
                ++numAttachment;
            }

            fbres.handle = gfx.CreateFrameBuffer({ attachments.data(), numAttachment }, false);
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

            PassInfo* pDesc = pass.pDesc;
            if (pDesc->type == ePass::Render)
            {
                if (pDesc->frameBufferName.IsEmpty())
                {
                    pDesc->fbh = kInvalidHandle;
                    if (pDesc->bUseFrameBufferResolutionForViewport)
                    {
                        pDesc->viewport.width  = static_cast<UInt16>(_width);
                        pDesc->viewport.height = static_cast<UInt16>(_height);
                    }
                }
                else
                {
                    const FrameBufferResource& fbres = m_frameBuffers.at(pDesc->frameBufferName);
                    pDesc->fbh                       = fbres.handle;

                    if (pDesc->bUseFrameBufferResolutionForViewport)
                    {
                        const Vec3i resolution = gfx.GetFrameBufferResolution(pDesc->fbh);
                        pDesc->viewport.width  = static_cast<UInt16>(resolution.width);
                        pDesc->viewport.height = static_cast<UInt16>(resolution.height);
                    }
                }
            }
        }

        return;
    }

    JAM_FUNCTION_MSG_PROFILE("Compile RenderGraph");

    // 실제 리소스 매핑
    for (PassInfo& info: m_infos)
    {
        if (!info.bEnable)
        {
            continue;
        }

        if (!info.programName.IsEmpty())
        {
            info.ph = m_programs.at(info.programName).handle;
        }

        if (!info.computeProgramName.IsEmpty())
        {
            info.computePh = m_programs.at(info.computeProgramName).handle;
        }

        info.writeResources.clear();
        if (info.type == ePass::Render)
        {
            if (info.frameBufferName.IsEmpty())
            {
                info.fbh = kInvalidHandle;
                if (info.bUseFrameBufferResolutionForViewport)
                {
                    info.viewport.width  = static_cast<UInt16>(_width);
                    info.viewport.height = static_cast<UInt16>(_height);
                }
            }
            else
            {
                const FrameBufferResource& fbres = m_frameBuffers.at(info.frameBufferName);
                info.fbh                         = fbres.handle;

                if (info.bUseFrameBufferResolutionForViewport)
                {
                    const Vec3i resolution = gfx.GetFrameBufferResolution(info.fbh);
                    info.viewport.width    = static_cast<UInt16>(resolution.width);
                    info.viewport.height   = static_cast<UInt16>(resolution.height);
                }

                info.writeResources.reserve(fbres.numRenderTargets);
                for (Int32 i = 0; i < fbres.numRenderTargets; ++i)
                {
                    info.writeResources.insert(fbres.attachments[i]);
                }

                if (fbres.bHasDepthStencil)
                {
                    info.writeResources.insert(fbres.attachments[kMaxNumRenderTargetViewSlots]);
                }
            }
        }

        info.readResources.clear();
        info.readResources.reserve(info.readBindCmds.size());
        for (ResourceBindCommand& cmd: info.readBindCmds | std::views::values)
        {
            if (cmd.type == eShaderResource::Texture)
            {
                cmd.handleValue = m_textures.at(cmd.name).handle.GetValue();
            }
            else
            {
                cmd.handleValue = m_storageBuffers.at(cmd.name).handle.GetValue();
            }
            info.readResources.insert(cmd.name);
        }

        info.readWriteResources.clear();
        info.readWriteResources.reserve(info.readWriteBindCmds.size());
        for (ResourceBindCommand& cmd: info.readWriteBindCmds | std::views::values)
        {
            if (cmd.type == eShaderResource::Texture)
            {
                cmd.handleValue = m_textures.at(cmd.name).handle.GetValue();
            }
            else
            {
                cmd.handleValue = m_storageBuffers.at(cmd.name).handle.GetValue();
            }
            info.readWriteResources.insert(cmd.name);
        }

        for (ConstantBufferBindCommand& cmd: info.cbufferBindCmds | std::views::values)
        {
            cmd.cbh = m_cbuffers.at(cmd.name).handle;
        }
    }

    // 디버그 검증

    if (JAM_DEBUG_ENABLED)
    {
        StringHashMap<eResourceAccessEx> used = {};
        for (const PassInfo& info: m_infos)
        {
            if (!info.bEnable)
            {
                continue;
            }

            used.clear();
            used.reserve(info.readBindCmds.size() + info.readWriteBindCmds.size() + info.writeResources.size());

            for (const ResourceBindCommand& cmd: info.readBindCmds | std::views::values)
            {
                const auto it = used.find(cmd.name);
                JAM_ASSERT(it == used.end() || it->second == eResourceAccessEx::Read, "Pass '{}' binds resource '{}' multiple times with different access types.", info.name, cmd.name);
                used[cmd.name] = eResourceAccessEx::Read;
            }

            for (const ResourceBindCommand& bind: info.readWriteBindCmds | std::views::values)
            {
                const auto it = used.find(bind.name);
                JAM_ASSERT(it == used.end() || it->second == eResourceAccessEx::ReadWrite, "Pass '{}' binds resource '{}' multiple times with different access types.", info.name, bind.name);
                used[bind.name] = eResourceAccessEx::ReadWrite;
            }

            for (const String& resource: info.writeResources)
            {
                const auto it = used.find(resource);
                JAM_ASSERT(it == used.end() || it->second == eResourceAccessEx::Write, "Pass '{}' binds resource '{}' multiple times with different access types.", info.name, resource);
                used[resource] = eResourceAccessEx::Write;
            }
        }
    }

    // 데드 패스 컬링.

    {
        // 리소스를 write하는 패스 집합
        StringHashMap<HashSet<Int32>> productors = {};
        for (Int32 i = 0; i < m_infos.size(); ++i)
        {
            const PassInfo& info = m_infos[i];
            if (!info.bEnable)
            {
                continue;
            }

            for (const String& write: info.writeResources)
            {
                productors[write].insert(i);
            }

            for (const String& readWrite: info.readWriteResources)
            {
                productors[readWrite].insert(i);
            }
        }

        // 역방향 의존성 간선 (i가 j의 출력을 읽으면, revGraph[i]에 j 추가)
        Vector<HashSet<Int32>> revGraph(m_infos.size());
        for (Int32 i = 0; i < m_infos.size(); ++i)
        {
            const PassInfo& info = m_infos[i];
            if (!info.bEnable)
            {
                continue;
            }

            for (const String& read: info.readResources)
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

            for (const String& readWrite: info.readWriteResources)
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
        for (Int32 i = 0; i < m_infos.size(); ++i)
        {
            const PassInfo& info = m_infos[i];
            if (info.bEnable && (info.bFinalPass || info.bSideEffect))
            {
                stack.push_back(i);
            }
        }
        JAM_ASSERT(!stack.empty(), "RenderGraph must have at least one final pass.");

        Vector<bool> survived(m_infos.size(), false);
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
        for (Int32 i = 0; i < m_infos.size(); ++i)
        {
            if (survived[i])
            {
                m_runtimePasses.push_back({ &m_infos[i] });
            }
        }
    }

    // 패스 간 리소스 헤저드 해결 + 바인딩 최적화
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

    StringHashMap<ResourceBindingState> resBindingStates = {};

    HashSet<Slot, SlotHasher> tmpReadUnbindSlots      = {};
    HashSet<Slot, SlotHasher> tmpReadWriteUnbindSlots = {};

    const SizeT numPasses = m_runtimePasses.size();
    for (SizeT i = 0; i < numPasses * 2; ++i)
    {
        const bool      bIsFirstLoop  = i < numPasses;
        const bool      bIsSecondLoop = !bIsFirstLoop;
        Pass&           pass          = m_runtimePasses[i % numPasses];
        const PassInfo& info          = *pass.pDesc;

        // CBuffer, Sampler 바인드 커맨드 수집

        if (bIsFirstLoop)
        {
            for (const ConstantBufferBindCommand& cmd: info.cbufferBindCmds | std::views::values)
            {
                const auto it = cbufferSlots.find(cmd.slot);
                if (it == cbufferSlots.end() || it->second != cmd.name)
                {
                    pass.cbufferBindCmds.push_back(cmd);
                }
            }

            for (const SamplerFlagsBindCommand& cmd: info.samplerBindCmds | std::views::values)
            {
                const auto it = samplerSlots.find(cmd.slot);
                if (it == samplerSlots.end() || it->second != cmd.state)
                {
                    pass.samplerBindCmds.push_back(cmd);
                }
            }
        }

        // 충돌 검출, unbind 슬롯 수집

        tmpReadUnbindSlots.clear();
        tmpReadWriteUnbindSlots.clear();

        for (const ResourceBindCommand& cmd: info.readBindCmds | std::views::values)
        {
            bool       bAddCmd = bIsFirstLoop;
            const auto it      = resBindingStates.find(cmd.name);
            if (it != resBindingStates.end())
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

        for (const ResourceBindCommand& cmd: info.readWriteBindCmds | std::views::values)
        {
            bool       bAddCmd = bIsFirstLoop;
            const auto it      = resBindingStates.find(cmd.name);
            if (it != resBindingStates.end())
            {
                const ResourceBindingState& state = it->second;
                switch (state.access)
                {
                    case eResourceAccessEx::ReadWrite:
                        if (state.slots.contains(cmd.slot))
                        {
                            bAddCmd = false;
                        }
                        break;
                    case eResourceAccessEx::Write:
                        pass.bUnbindFrameBuffer = true;
                        break;
                    case eResourceAccessEx::Read:
                        tmpReadUnbindSlots.insert(state.slots.begin(), state.slots.end());
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

        for (const String& write: info.writeResources)
        {
            const auto it = resBindingStates.find(write);
            if (it == resBindingStates.end())
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

        // 두 번째 순회의 unbind 리스트만 패스에 기록

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

        // 가상 unbind

        for (const Slot& slot: tmpReadUnbindSlots)
        {
            JAM_ASSERT(readSlots.contains(slot), "read slot must be tracked.");
            const String&         name  = readSlots.at(slot);
            ResourceBindingState& state = resBindingStates.at(name);
            JAM_ASSERT(state.access == eResourceAccessEx::Read, "access kind must be Read for read-slot unbind.");
            JAM_ASSERT(state.slots.contains(slot), "state slots must contain the unbinding slot.");
            state.slots.erase(slot);
            if (state.slots.empty())
            {
                resBindingStates.erase(name);
            }
            readSlots.erase(slot);
        }

        for (const Slot& slot: tmpReadWriteUnbindSlots)
        {
            JAM_ASSERT(readWriteSlots.contains(slot), "readWrite slot must be tracked.");
            const String&         name  = readWriteSlots.at(slot);
            ResourceBindingState& state = resBindingStates.at(name);
            JAM_ASSERT(state.access == eResourceAccessEx::ReadWrite, "access kind must be ReadWrite for rw-slot unbind.");
            JAM_ASSERT(state.slots.contains(slot), "state slots must contain the unbinding slot.");
            state.slots.erase(slot);
            if (state.slots.empty())
            {
                resBindingStates.erase(name);
            }
            readWriteSlots.erase(slot);
        }

        for (const String& write: info.writeResources)
        {
            const auto it = resBindingStates.find(write);
            if (it != resBindingStates.end())
            {
                JAM_ASSERT(it->second.slots.empty() || it->second.access == eResourceAccessEx::Write, "write resource '{}' still owns tracked slots before re-bind as write.", write);
                resBindingStates.erase(it);
            }
        }

        // 가상 bind

        for (const ResourceBindCommand& cmd: info.readBindCmds | std::views::values)
        {
            const auto prevIt = readSlots.find(cmd.slot);
            if (prevIt != readSlots.end() && prevIt->second != cmd.name)
            {
                ResourceBindingState& prevState = resBindingStates.at(prevIt->second);
                JAM_ASSERT(prevState.access == eResourceAccessEx::Read, "previous occupant of read slot must be Read.");
                JAM_ASSERT(prevState.slots.contains(cmd.slot), "previous occupant must own this slot.");
                prevState.slots.erase(cmd.slot);
                if (prevState.slots.empty())
                {
                    resBindingStates.erase(prevIt->second);
                }
            }

            readSlots[cmd.slot]         = cmd.name;
            ResourceBindingState& state = resBindingStates[cmd.name];
            state.access                = eResourceAccessEx::Read;
            state.slots.insert(cmd.slot);
        }

        for (const ResourceBindCommand& cmd: info.readWriteBindCmds | std::views::values)
        {
            const auto prevIt = readWriteSlots.find(cmd.slot);
            if (prevIt != readWriteSlots.end() && prevIt->second != cmd.name)
            {
                ResourceBindingState& prevState = resBindingStates.at(prevIt->second);
                JAM_ASSERT(prevState.access == eResourceAccessEx::ReadWrite, "previous occupant of rw slot must be ReadWrite.");
                JAM_ASSERT(prevState.slots.contains(cmd.slot), "previous occupant must own this slot.");
                prevState.slots.erase(cmd.slot);
                if (prevState.slots.empty())
                {
                    resBindingStates.erase(prevIt->second);
                }
            }

            readWriteSlots[cmd.slot]    = cmd.name;
            ResourceBindingState& state = resBindingStates[cmd.name];
            state.access                = eResourceAccessEx::ReadWrite;
            state.slots.insert(cmd.slot);
        }

        for (const String& write: info.writeResources)
        {
            ResourceBindingState& state = resBindingStates[write];
            JAM_ASSERT(state.slots.empty(), "write resource '{}' must not own any read/rw slots when bound as write.", write);
            state.access = eResourceAccessEx::Write;
        }

        for (const ConstantBufferBindCommand& cmd: info.cbufferBindCmds | std::views::values)
        {
            cbufferSlots[cmd.slot] = cmd.name;
        }

        for (const SamplerFlagsBindCommand& cmd: info.samplerBindCmds | std::views::values)
        {
            samplerSlots[cmd.slot] = cmd.state;
        }
    }

    m_bNeedCompile = false;
    gfx.Reset();
}

void RenderGraph::Execute(
    const FrameBufferHandle _fbh,
    const Viewport          _vp)
{
    CompileIfNeed_(_vp.width, _vp.height);

    Graphics&   gfx       = GetGraphics();
    const SizeT numPasses = m_runtimePasses.size();

    for (SizeT i = 0; i < numPasses; ++i)
    {
        const Pass&     pass = m_runtimePasses[i];
        const PassInfo& info = *pass.pDesc;

        if (info.type == ePass::Utility)
        {
            if (info.executor)
            {
                info.executor();
            }
        }
        else if (info.type == ePass::Compute)
        {
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

            if (pass.bUnbindFrameBuffer)
            {
                gfx.SetFrameBuffer(kInvalidHandle);
                bAnyUnbound = true;
            }

            if (bAnyUnbound)
            {
                gfx.Touch();
            }

            gfx.SetComputeProgram(info.computePh);

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

            if (info.executor)
            {
                info.executor();
            }
        }
        else
        {
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

            FrameBufferHandle fbh = info.fbh;
            Viewport          vp  = info.viewport;
            if (info.bFinalPass)   // 최종 패스의 경우 Execute()에 전달된 프레임버퍼와 뷰포트를 사용
            {
                fbh = _fbh;
                vp  = _vp;
            }

            gfx.SetProgram(info.ph);
            gfx.SetFrameBuffer(fbh);
            gfx.SetViewport(vp);
            gfx.SetRenderFlags(info.renderFlags);
            gfx.SetStencilFlags(info.fstencil, info.bstencil, info.stencilRef);
            gfx.SetScissor(info.scissorRect);

            for (Int32 j = 0; j < info.numBlendFlagsBindCmds; ++j)
            {
                const BlendFlagsBindCommand& bind = info.blendFlagsBindCmds[j];
                gfx.SetBlendFlags(bind.state, bind.slot);
            }

            gfx.SetBlendFactor(info.blendFactor);

            for (Int32 j = 0; j < info.numRenderTargetClearCmds; ++j)
            {
                const RenderTargetClearCommand& bind = info.renderTargetClearCmds[j];
                gfx.ClearRenderTarget(fbh, bind.color, bind.slot);
            }

            if (info.bClearDepth || info.bClearStencil)
            {
                gfx.ClearDepthStencil(fbh, info.bClearDepth, info.bClearStencil, info.depthClearValue, info.stencilClearValue);
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

            if (info.executor)
            {
                info.executor();
            }
        }
    }
}

void RenderGraph::InsertTexture(
    const StringView    _name,
    const TextureHandle _texh,
    const bool          _bOwnership)
{
    const auto it = m_textures.find(_name);
    if (it != m_textures.end())
    {
        TextureResource& tres = it->second;
        if (tres.bHasOwnership)
        {
            JAM_DESTROY_TEXTURE(tres.handle);
        }
    }

    Graphics&   gfx   = GetGraphics();
    TextureInfo tinfo = gfx.GetTextureInfo(_texh);

    TextureResource tres = {};
    tres.name            = _name;
    tres.handle          = _texh;
    tres.bHasOwnership   = _bOwnership;
    tres.bAutoResize     = false;
    tres.resizeScale     = 1.f;
    tres.type            = tinfo.type;
    tres.format          = tinfo.format;
    tres.bHasMips        = tinfo.numMips > 1;
    tres.numLayers       = tinfo.numLayers;
    tres.flags           = tinfo.flags;
    m_textures[_name]    = tres;

    m_bNeedCompile = true;
}

void RenderGraph::EmplaceTexture2D(
    const StringView             _name,
    const Int32                  _width,
    const Int32                  _height,
    const eTextureFormat         _format,
    const bool                   _bHasMips,
    const Int32                  _numLayers,
    const Flags<eTextureFlags>   _flags,
    const Span<const MemoryView> _initData)
{
    Graphics&           gfx  = GetGraphics();
    const TextureHandle texh = gfx.CreateTexture2D(_width, _height, _format, _bHasMips, _numLayers, _flags, _initData);
    InsertTexture(_name, texh, true);
}

void RenderGraph::EmplaceTexture2D(
    const StringView           _name,
    const float                _resizeScale,
    const eTextureFormat       _format,
    const bool                 _bHasMips,
    const Int32                _numLayers,
    const Flags<eTextureFlags> _flags)
{
    JAM_ASSERT(_resizeScale > 0.f, "resizeScale must be > 0 for '{}'.", _name);

    const auto it = m_textures.find(_name);
    if (it != m_textures.end())
    {
        TextureResource& tres = it->second;
        if (tres.bHasOwnership)
        {
            JAM_DESTROY_TEXTURE(tres.handle);
        }
    }

    TextureResource tres = {};
    tres.name            = _name;
    tres.handle          = kInvalidHandle;
    tres.type            = eTexture::Texture2D;
    tres.bHasOwnership   = true;
    tres.bAutoResize     = true;
    tres.resizeScale     = _resizeScale;
    tres.format          = _format;
    tres.bHasMips        = _bHasMips;
    tres.numLayers       = _numLayers;
    tres.flags           = _flags;
    m_textures[_name]    = tres;

    m_bNeedCompile = true;
}

void RenderGraph::EmplaceTexture3D(
    const StringView             _name,
    const Int32                  _width,
    const Int32                  _height,
    const Int32                  _depth,
    const eTextureFormat         _format,
    const bool                   _bHasMips,
    const Flags<eTextureFlags>   _flags,
    const Span<const MemoryView> _initData)
{
    Graphics&           gfx  = GetGraphics();
    const TextureHandle texh = gfx.CreateTexture3D(_width, _height, _depth, _format, _bHasMips, _flags, _initData);
    InsertTexture(_name, texh, true);
}

void RenderGraph::EmplaceTextureCube(
    const StringView           _name,
    const Int32                _width,
    const Int32                _height,
    const eTextureFormat       _format,
    const bool                 _bHasMips,
    const Int32                _numCubes,
    const Flags<eTextureFlags> _flags)
{
    Graphics&           gfx  = GetGraphics();
    const TextureHandle texh = gfx.CreateTextureCube(_width, _height, _format, _bHasMips, _numCubes, _flags);
    InsertTexture(_name, texh, true);
}

void RenderGraph::EmplaceTextureCube(
    const StringView           _name,
    const float                _resizeScale,
    const eTextureFormat       _format,
    const bool                 _bHasMips,
    const Int32                _numCubes,
    const Flags<eTextureFlags> _flags)
{
    JAM_ASSERT(_resizeScale > 0.f, "resizeScale must be > 0 for '{}'.", _name);

    const auto it = m_textures.find(_name);
    if (it != m_textures.end())
    {
        TextureResource& tres = it->second;
        if (tres.bHasOwnership)
        {
            JAM_DESTROY_TEXTURE(tres.handle);
        }
    }

    TextureResource tres = {};
    tres.name            = _name;
    tres.handle          = kInvalidHandle;
    tres.type            = eTexture::Texture2D;
    tres.bHasOwnership   = true;
    tres.bAutoResize     = true;
    tres.resizeScale     = _resizeScale;
    tres.format          = _format;
    tres.bHasMips        = _bHasMips;
    tres.numLayers       = _numCubes;
    tres.flags           = _flags;
    m_textures[_name]    = tres;

    m_bNeedCompile = true;
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
    const TextureResource& tres = m_textures.at(_name);
    return tres.handle;
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
    const auto it = m_cbuffers.find(_name);
    if (it != m_cbuffers.end())
    {
        ConstantBufferResource& cres = it->second;
        if (cres.bHasOwnership)
        {
            JAM_DESTROY_CONSTANT_BUFFER(cres.handle);
        }
    }

    ConstantBufferResource cres = {};
    cres.name                   = _name;
    cres.handle                 = _cbh;
    cres.bHasOwnership          = _bOwnership;
    m_cbuffers[_name]           = cres;

    m_bNeedCompile = true;
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
    const ConstantBufferResource& cres = m_cbuffers.at(_name);
    return cres.handle;
}

bool RenderGraph::HasConstantBuffer(const StringView _name) const
{
    return m_cbuffers.contains(_name);
}

void RenderGraph::InsertStorageBuffer(
    const StringView          _name,
    const StorageBufferHandle _sbh,
    const bool                _bOwnership)
{
    const auto it = m_storageBuffers.find(_name);
    if (it != m_storageBuffers.end())
    {
        StorageBufferResource& sbres = it->second;
        if (sbres.bHasOwnership)
        {
            JAM_DESTROY_STORAGE_BUFFER(sbres.handle);
        }
    }

    StorageBufferResource sbres = {};
    sbres.name                  = _name;
    sbres.handle                = _sbh;
    sbres.bHasOwnership         = _bOwnership;
    m_storageBuffers[_name]     = sbres;

    m_bNeedCompile = true;
}

void RenderGraph::EmplaceRawBuffer(
    const StringView                 _name,
    const Int32                      _byteWidth,
    const Flags<eStorageBufferFlags> _flags,
    const MemoryView                 _initDataOrEmpty)
{
    Graphics&                 gfx = GetGraphics();
    const StorageBufferHandle sbh = gfx.CreateRawBuffer(_byteWidth, _flags, _initDataOrEmpty);
    InsertStorageBuffer(_name, sbh, true);
}

void RenderGraph::EmplaceStructuredBuffer(
    const StringView                 _name,
    const Int32                      _numElems,
    const Int32                      _stride,
    const eStructureBuffer           _type,
    const Flags<eStorageBufferFlags> _flags,
    const MemoryView                 _initDataOrEmpty)
{
    Graphics&                 gfx = GetGraphics();
    const StorageBufferHandle sbh = gfx.CreateStructuredBuffer(_numElems, _stride, _type, _flags, _initDataOrEmpty);
    InsertStorageBuffer(_name, sbh, true);
}

void RenderGraph::EmplaceTypedBuffer(
    const StringView                 _name,
    const Int32                      _numElems,
    const eTypedBufferFormat         _dataType,
    const Flags<eStorageBufferFlags> _flags,
    const MemoryView                 _initDataOrEmpty)
{
    Graphics&                 gfx = GetGraphics();
    const StorageBufferHandle sbh = gfx.CreateTypedBuffer(_numElems, _dataType, _flags, _initDataOrEmpty);
    InsertStorageBuffer(_name, sbh, true);
}

void RenderGraph::EmplaceIndirectBuffer(const StringView _name, const Int32 _numDraws)
{
    Graphics&                 gfx = GetGraphics();
    const StorageBufferHandle sbh = gfx.CreateIndirectBuffer(_numDraws);
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
    const StorageBufferResource& sbres = m_storageBuffers.at(_name);
    JAM_ASSERT(sbres.handle != kInvalidHandle, "StorageBuffer '{}' not created.", _name);
    return sbres.handle;
}

bool RenderGraph::HasStorageBuffer(const StringView _name) const
{
    return m_storageBuffers.contains(_name);
}

void RenderGraph::InsertFrameBuffer(
    const StringView        _name,
    const FrameBufferHandle _fbh,
    const bool              _bOwnership)
{
    const auto it = m_frameBuffers.find(_name);
    if (it != m_frameBuffers.end())
    {
        FrameBufferResource& fbres = it->second;
        if (fbres.bHasOwnership)
        {
            JAM_DESTROY_FRAME_BUFFER(fbres.handle);
        }
    }

    FrameBufferResource fbres = {};
    fbres.name                = _name;
    fbres.handle              = _fbh;
    fbres.bHasOwnership       = _bOwnership;
    fbres.numRenderTargets    = 0;
    fbres.bHasDepthStencil    = false;
    fbres.bAutoResize         = false;
    m_frameBuffers[_name]     = fbres;

    m_bNeedCompile = true;
}

void RenderGraph::EmplaceFrameBuffer(
    const StringView               _name,
    const Span<const AttachmentRG> _attachments)
{
    JAM_ASSERT(!_attachments.empty(), "FrameBuffer '{}' must have at least one attachment.", _name);

    const auto it = m_frameBuffers.find(_name);
    if (it != m_frameBuffers.end())
    {
        FrameBufferResource& existing = it->second;
        if (existing.bHasOwnership)
        {
            JAM_DESTROY_FRAME_BUFFER(existing.handle);
        }
    }

    FrameBufferResource fbres = {};
    fbres.name                = _name;
    fbres.handle              = kInvalidHandle;
    fbres.bHasOwnership       = true;
    fbres.numRenderTargets    = 0;
    fbres.bHasDepthStencil    = false;
    fbres.bAutoResize         = false;

    for (const AttachmentRG& attachment: _attachments)
    {
        const TextureResource& tres = m_textures.at(attachment.name);
        if (tres.flags & eTextureFlags::DepthStencil)
        {
            JAM_ASSERT(!fbres.bHasDepthStencil, "FrameBuffer '{}' cannot have multiple depth/stencil attachments. Attachment '{}' is a duplicate.", _name, attachment.name);
            fbres.attachments[kDepthStencilSlot] = attachment.name;
            fbres.bHasDepthStencil               = true;
        }
        else
        {
            fbres.attachments[fbres.numRenderTargets++] = attachment.name;
        }

        fbres.bAutoResize |= tres.bAutoResize;
        JAM_ASSERT(fbres.bAutoResize == tres.bAutoResize, "All attachments of FrameBuffer '{}' must have the same auto-resize setting. Attachment '{}' differs.", _name, attachment.name);
    }

    // auto-resize인 경우 CompileIfNeed_ 에서 지연 생성. 아닐경우 즉시 생성
    if (!fbres.bAutoResize)
    {
        Array<Attachment, kMaxNumAttachmentSlots> attachments    = {};
        Int32                                     numAttachments = 0;
        for (const AttachmentRG& attachment: _attachments)
        {
            const TextureResource& tres = m_textures.at(attachment.name);
            JAM_ASSERT(tres.handle != kInvalidHandle, "Texture '{}' for FrameBuffer '{}' is not created.", attachment.name, _name);
            attachments[numAttachments].texh        = tres.handle;
            attachments[numAttachments].mip         = attachment.mip;
            attachments[numAttachments].layerOffset = attachment.layerOffset;
            attachments[numAttachments].numLayers   = attachment.numLayers;
            ++numAttachments;
        }

        Graphics& gfx = GetGraphics();
        fbres.handle  = gfx.CreateFrameBuffer({ attachments.data(), numAttachments }, false);
    }

    m_frameBuffers[_name] = fbres;
    m_bNeedCompile        = true;
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
    const FrameBufferResource& fbres = m_frameBuffers.at(_name);
    return fbres.handle;
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
    const auto it = m_shaders.find(_name);
    if (it != m_shaders.end())
    {
        ShaderResource& res = it->second;
        if (res.bHasOwnership)
        {
            JAM_DESTROY_SHADER(res.handle);
        }
    }

    ShaderResource res = {};
    res.name           = _name;
    res.handle         = _sh;
    res.bHasOwnership  = _bOwnership;
    m_shaders[_name]   = res;

    m_bNeedCompile = true;
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
    const ShaderResource& shres = m_shaders.at(_name);
    return shres.handle;
}

bool RenderGraph::HasShader(const StringView _name) const
{
    return m_shaders.contains(_name);
}

bool RenderGraph::HasProgram(const StringView _name) const
{
    return m_programs.contains(_name);
}

void RenderGraph::InsertProgram(
    const StringView    _name,
    const ProgramHandle _ph,
    const bool          _bOwnership)
{
    const auto it = m_programs.find(_name);
    if (it != m_programs.end())
    {
        ProgramResource& pres = it->second;
        if (pres.bHasOwnership)
        {
            JAM_DESTROY_PROGRAM(pres.handle);
        }
    }

    ProgramResource pres = {};
    pres.name            = _name;
    pres.handle          = _ph;
    pres.bHasOwnership   = _bOwnership;
    m_programs[_name]    = pres;

    m_bNeedCompile = true;
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
    const ShaderHandle vsh = _vsOrEmpty.IsEmpty() ? kInvalidHandle : GetShader(_vsOrEmpty);
    const ShaderHandle psh = _psOrEmpty.IsEmpty() ? kInvalidHandle : GetShader(_psOrEmpty);
    const ShaderHandle gsh = _gsOrEmpty.IsEmpty() ? kInvalidHandle : GetShader(_gsOrEmpty);
    const ShaderHandle hsh = _hsOrEmpty.IsEmpty() ? kInvalidHandle : GetShader(_hsOrEmpty);
    const ShaderHandle dsh = _dsOrEmpty.IsEmpty() ? kInvalidHandle : GetShader(_dsOrEmpty);

    Graphics&           gfx = GetGraphics();
    const ProgramHandle ph  = gfx.CreateProgram(vsh, psh, gsh, hsh, dsh, false);
    InsertProgram(_name, ph, true);
}

void RenderGraph::EmplaceComputeProgram(
    const StringView _name,
    const StringView _csName)
{
    const ShaderHandle  csh = GetShader(_csName);
    Graphics&           gfx = GetGraphics();
    const ProgramHandle ph  = gfx.CreateProgram(csh);
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
    const ProgramResource& pres = m_programs.at(_name);
    return pres.handle;
}

void RenderGraph::Clear(const Flags<eRenderGraphClearFlags> _flags)
{
    m_bNeedCompile = true;

    if (_flags.IsEnabled(eRenderGraphClearFlags::Pass))
    {
        m_infos.clear();
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

RenderGraph::PassInfo& RenderGraph::GetPassDesc_(const StringView name)
{
    const auto it = std::ranges::find_if(m_infos, [name](const PassInfo& _desc) { return _desc.name == name; });
    JAM_ASSERT(it != m_infos.end(), "Pass '{}' not found.", name);
    return *it;
}

const RenderGraph::PassInfo& RenderGraph::GetPassDesc_(StringView name) const
{
    const auto it = std::ranges::find_if(m_infos, [name](const PassInfo& _desc) { return _desc.name == name; });
    JAM_ASSERT(it != m_infos.end(), "Pass '{}' not found.", name);
    return *it;
}

bool RenderGraph::HasPass_(StringView name) const
{
    return std::ranges::any_of(m_infos, [name](const PassInfo& pd) { return pd.name == name; });
}

RenderGraph::UtilityPassBuilder::UtilityPassBuilder(
    RenderGraph* _pGraph,
    PassInfo*    _pDesc)
    : m_pGraph(_pGraph)
    , m_pInfo(_pDesc)
{
    m_pInfo->type        = ePass::Utility;
    m_pInfo->bSideEffect = true;
}

RenderGraph::UtilityPassBuilder& RenderGraph::UtilityPassBuilder::Enable(const bool _bEnable)
{
    m_pInfo->bEnable = _bEnable;
    return *this;
}

RenderGraph::UtilityPassBuilder& RenderGraph::UtilityPassBuilder::SetExecutor(const Delegate<void()>& _executor)
{
    JAM_ASSERT(_executor, "Executor cannot be null.");
    m_pInfo->executor = _executor;
    return *this;
}

RenderGraph::RenderPassBuilder::RenderPassBuilder(
    RenderGraph* _pGraph,
    PassInfo*    _pDesc)
    : m_pGraph(_pGraph)
    , m_pInfo(_pDesc)
{
    m_pInfo->type = ePass::Render;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::AsFinalPass(const bool _bEnable)
{
    m_pInfo->bFinalPass = _bEnable;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::AsSideEffect(const bool _bEnable)
{
    m_pInfo->bSideEffect = _bEnable;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::Enable(const bool _bEnable)
{
    m_pInfo->bEnable = _bEnable;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetProgram(const StringView _name)
{
    JAM_ASSERT(!_name.IsEmpty(), "Program name cannot be empty.");
    m_pInfo->programName = _name;
    m_pInfo->ph          = kInvalidHandle;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetFrameBuffer(const StringView _name)
{
    JAM_ASSERT(!_name.IsEmpty(), "FrameBuffer name cannot be empty.");
    m_pInfo->frameBufferName = _name;
    m_pInfo->fbh             = kInvalidHandle;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetViewport(const Viewport _vp)
{
    m_pInfo->viewport                             = _vp;
    m_pInfo->bUseFrameBufferResolutionForViewport = false;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetFrameBufferResolutionForViewport()
{
    m_pInfo->bUseFrameBufferResolutionForViewport = true;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetScissorRect(const Rect _rect)
{
    m_pInfo->scissorRect = _rect;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetRenderFlags(const Flags<eRenderFlags> _flags)
{
    m_pInfo->renderFlags = _flags;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetStencilFlags(
    const Flags<eStencilFlags> _fstencil,
    const Flags<eStencilFlags> _bstencil,
    const UInt8                _ref)
{
    m_pInfo->fstencil   = _fstencil;
    m_pInfo->bstencil   = _bstencil;
    m_pInfo->stencilRef = _ref;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetBlendFlags(
    const Flags<eBlendFlags> _flags,
    const Int32              _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < kMaxNumRenderTargetViewSlots, "Blend slot {} out of range.", _slot);

    for (Int32 i = 0; i < m_pInfo->numBlendFlagsBindCmds; ++i)
    {
        if (m_pInfo->blendFlagsBindCmds[i].slot == _slot)
        {
            m_pInfo->blendFlagsBindCmds[i].state = _flags;
            return *this;
        }
    }

    JAM_ASSERT(m_pInfo->numBlendFlagsBindCmds < kMaxNumRenderTargetViewSlots, "Too many blend states.");
    m_pInfo->blendFlagsBindCmds[m_pInfo->numBlendFlagsBindCmds++] = { _flags, _slot };
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetBlendFactor(const RGBA _factor)
{
    m_pInfo->blendFactor = _factor;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetRenderTargetClearCommand(
    const RGBA  _color,
    const Int32 _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < kMaxNumRenderTargetViewSlots, "ClearColor slot {} out of range.", _slot);

    for (Int32 i = 0; i < m_pInfo->numRenderTargetClearCmds; ++i)
    {
        if (m_pInfo->renderTargetClearCmds[i].slot == _slot)
        {
            m_pInfo->renderTargetClearCmds[i].color = _color;
            return *this;
        }
    }

    m_pInfo->renderTargetClearCmds[m_pInfo->numRenderTargetClearCmds++] = { _color, _slot };
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetDepthClearCommand(const float _value)
{
    m_pInfo->depthClearValue = _value;
    m_pInfo->bClearDepth     = true;
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetStencilClearCommand(const UInt8 _value)
{
    m_pInfo->stencilClearValue = _value;
    m_pInfo->bClearStencil     = true;
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
        m_pInfo->readBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Texture, _stage, _slot };
    }
    else
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumUnorderedAccessViewSlots, "Texture slot {} out of range.", _slot);
        m_pInfo->readWriteBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Texture, _stage, _slot };
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
        m_pInfo->readBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Buffer, _stage, _slot };
    }
    else
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumUnorderedAccessViewSlots, "Buffer slot {} out of range.", _slot);
        m_pInfo->readWriteBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Buffer, _stage, _slot };
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
    m_pInfo->cbufferBindCmds[slot] = { _name, kInvalidHandle, slot };
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
    m_pInfo->samplerBindCmds[slot] = { _state, _borderColor, _stage, _slot };
    return *this;
}

RenderGraph::RenderPassBuilder& RenderGraph::RenderPassBuilder::SetExecutor(const Delegate<void()>& _executor)
{
    JAM_ASSERT(_executor, "Executor cannot be null.");
    m_pInfo->executor = _executor;
    return *this;
}

RenderGraph::ComputePassBuilder::ComputePassBuilder(
    RenderGraph* _pGraph,
    PassInfo*    _pDesc)
    : m_pGraph(_pGraph)
    , m_pInfo(_pDesc)
{
    _pDesc->type = ePass::Compute;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::AsFinalPass(const bool _bEnable)
{
    m_pInfo->bFinalPass = _bEnable;
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::AsSideEffect(
    const bool _bEnable)
{
    m_pInfo->bSideEffect = _bEnable;
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::Enable(const bool _bEnable)
{
    m_pInfo->bEnable = _bEnable;
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::SetProgram(const StringView _name)
{
    JAM_ASSERT(!_name.IsEmpty(), "Compute program name cannot be empty.");
    m_pInfo->computeProgramName = _name;
    m_pInfo->computePh          = kInvalidHandle;
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
        m_pInfo->readBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Texture, slot };
    }
    else
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumUnorderedAccessViewSlots, "Texture slot {} out of range.", _slot);
        m_pInfo->readWriteBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Texture, slot };
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
        m_pInfo->readBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Buffer, slot };
    }
    else
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumUnorderedAccessViewSlots, "Buffer slot {} out of range.", _slot);
        m_pInfo->readWriteBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Buffer, slot };
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
    m_pInfo->cbufferBindCmds[slot] = { _name, kInvalidHandle, eShader::ComputeShader, _slot };
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::SetExecutor(
    const Delegate<void()>& _executor)
{
    m_pInfo->executor = _executor;
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
    m_pInfo->samplerBindCmds[slot] = { _state, _borderColor, eShader::ComputeShader, _slot };
    return *this;
}

}   // namespace Jam