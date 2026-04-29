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

RenderGraph::PassBuilder RenderGraph::AddPass(const StringView _name)
{
    JAM_ASSERT(!HasPass_(_name), "Pass '{}' already exists.", _name);

    PassDesc& desc = m_descs.emplace_back();
    desc.name      = _name;
    m_bNeedCompile = true;
    return PassBuilder { *this, desc };
}

RenderGraph::PassBuilder RenderGraph::ModifyPass(const StringView _name)
{
    PassDesc* pDesc = FindPassDesc_(_name);
    JAM_ASSERT(pDesc, "Pass '{}' not found.", _name);
    m_bNeedCompile = true;
    return PassBuilder { *this, *pDesc };
}

RenderGraph::ComputePassBuilder RenderGraph::AddComputePass(const StringView _name)
{
    JAM_ASSERT(!HasPass_(_name), "Pass '{}' already exists.", _name);

    PassDesc& desc = m_descs.emplace_back();
    desc.name      = _name;
    m_bNeedCompile = true;
    return ComputePassBuilder { *this, desc };
}

RenderGraph::ComputePassBuilder RenderGraph::ModifyComputePass(const StringView _name)
{
    PassDesc* pDesc = FindPassDesc_(_name);
    JAM_ASSERT(pDesc, "Pass '{}' not found.", _name);
    m_bNeedCompile = true;
    return ComputePassBuilder { *this, *pDesc };
}

void RenderGraph::RemovePass(const StringView _name)
{
    if (eastl::erase_if(m_descs, [&](const PassDesc& d) { return d.name == _name; }) == 0)
    {
        JAM_ASSERT(0, "Pass '{}' not found.", _name);
    }

    m_bNeedCompile = true;
}

void RenderGraph::CompileIfNeed_()
{
    JAM_ASSERT(!m_descs.empty(), "RenderGraph must have at least one pass.");

    ////////// 불필요한 컴파일 방지 //////////

    Graphics&   gfx         = GetGraphics();
    const Vec2i res         = gfx.GetBackBufferResolution();
    const bool  bNeedResize = (m_lastWidth != res.x) || (m_lastHeight != res.y);
    if (!m_bNeedCompile && !bNeedResize)
    {
        return;
    }

    ////////// 필요시 텍스처 리사이즈 //////////

    if (bNeedResize)
    {
        JAM_FUNCTION_MSG_PROFILE("Resize Textures");

        m_lastWidth  = res.w;
        m_lastHeight = res.h;

        // 텍스처 리사이즈
        for (TextureResource& tex: m_textures | std::views::values)
        {
            // 소유권이 없거나 자동 리사이즈가 아닌 텍스처는 건너뜀
            if (!tex.bHasOwnership || !tex.bAutoResize)
            {
                continue;
            }

            const Int32 width  = static_cast<Int32>(Ceil(res.w * tex.resizeScale));
            const Int32 height = static_cast<Int32>(Ceil(res.h * tex.resizeScale));
            JAM_ASSERT(width > 0 && height > 0, "Auto-resize produced invalid size ({}, {}) for '{}'.", width, height, tex.name);
            JAM_DESTROY_TEXTURE(tex.handle);
            tex.handle = gfx.CreateTexture2D(width, height, tex.format, false, tex.arraySize, tex.flags);
        }

        // 프레임버퍼 재생성

        for (FrameBufferResource& fb: m_frameBuffers | std::views::values)
        {
            // 소유권이 없으면 재생성하지않음
            if (!fb.bHasOwnership || !fb.bAutoResize)
            {
                continue;
            }

            // 프레임버퍼 생성
            Array<TextureHandle, kMaxNumAttachmentSlots> attachments;
            for (Int32 i = 0; i < fb.numAttachments; ++i)
            {
                const TextureResource& texRes = m_textures.at(fb.attachments[i]);
                attachments[i]                = texRes.handle;
            }

            JAM_DESTROY_FRAME_BUFFER(fb.handle);
            fb.handle = gfx.CreateFrameBuffer({ attachments.data(), fb.numAttachments }, false);
        }
    }

    ////////// 만약 리사이즈만 했다면 재컴파일은 불필요 //////////

    if (bNeedResize && !m_bNeedCompile)
    {
        JAM_FUNCTION_MSG_PROFILE("Not Compile Resize Only");

        // 이미 컴파일된 패스 리스트를 수정할 필요가 없기에
        // 컴파일된 패스의 텍스처와 프레임버퍼 핸들만 갱신해주면 됨

        // Pass 갱신
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
            if (!pDesc->bIsComputePass)
            {
                if (pDesc->frameBufferName.IsEmpty())
                {
                    pDesc->fbh = kInvalidHandle;

                    if (pDesc->bUseViewportFromFrameBufferResoulution)
                    {
                        pDesc->viewport = Viewport { static_cast<UInt16>(res.w), static_cast<UInt16>(res.h) };
                    }
                }
                else
                {
                    const FrameBufferResource& fbRes = m_frameBuffers.at(pDesc->frameBufferName);
                    pDesc->fbh                       = fbRes.handle;

                    if (pDesc->bUseViewportFromFrameBufferResoulution)
                    {
                        const FrameBufferInfo& fbInfo  = gfx.GetFrameBufferInfo(pDesc->fbh);
                        const TextureHandle    texh    = fbInfo.bHasDepthStencil ? fbInfo.depthStencilTexh : fbInfo.renderTargetTexhs[0];
                        const TextureInfo&     texInfo = gfx.GetTextureInfo(texh);
                        pDesc->viewport                = Viewport { static_cast<UInt16>(texInfo.width), static_cast<UInt16>(texInfo.height) };
                    }
                }
            }
        }

        return;
    }

    JAM_FUNCTION_MSG_PROFILE("Compile RenderGraph");

    ////////// PassDesc에 실제 리소스 매핑 //////////

    for (PassDesc& desc: m_descs)
    {
        if (!desc.bEnable)   // 비활성화 패스는 패스
        {
            continue;
        }

        // graphics pass
        if (!desc.programName.IsEmpty())
        {
            const ProgramResource& program = m_programs.at(desc.programName);
            desc.ph                        = program.handle;
        }

        // compute pass
        if (!desc.computeProgramName.IsEmpty())
        {
            const ProgramResource& program = m_programs.at(desc.computeProgramName);
            desc.computePh                 = program.handle;
        }

        // 쓰기 정보 갱신
        desc.writeResources.clear();
        if (!desc.bIsComputePass)
        {
            if (desc.frameBufferName.IsEmpty())
            {
                desc.fbh = kInvalidHandle;

                if (desc.bUseViewportFromFrameBufferResoulution)
                {
                    desc.viewport = Viewport { static_cast<UInt16>(res.w), static_cast<UInt16>(res.h) };
                }
            }
            else   // 백버퍼를 사용
            {
                const FrameBufferResource& fbRes = m_frameBuffers.at(desc.frameBufferName);
                desc.fbh                         = fbRes.handle;

                // 뷰포트 갱신
                if (desc.bUseViewportFromFrameBufferResoulution)
                {
                    const FrameBufferInfo& fbInfo  = gfx.GetFrameBufferInfo(desc.fbh);
                    const TextureHandle    texh    = fbInfo.bHasDepthStencil ? fbInfo.depthStencilTexh : fbInfo.renderTargetTexhs[0];
                    const TextureInfo&     texInfo = gfx.GetTextureInfo(texh);
                    desc.viewport                  = Viewport { static_cast<UInt16>(texInfo.width), static_cast<UInt16>(texInfo.height) };
                }

                // 룩업 갱신
                desc.writeResources.reserve(fbRes.numAttachments);
                for (Int32 i = 0; i < fbRes.numAttachments; ++i)
                {
                    desc.writeResources.insert(fbRes.attachments[i]);
                }
            }
        }

        // 읽기 정보 갱신
        desc.readResources.clear();
        desc.readResources.reserve(desc.readBindCmds.size());
        for (ResourceBindCommand& cmd: desc.readBindCmds | std::views::values)
        {
            // 바인드
            if (cmd.type == eShaderResource::Texture)
            {
                const TextureResource& texRes = m_textures.at(cmd.name);
                cmd.handleValue               = texRes.handle.GetValue();
            }
            else
            {
                const StorageBufferResource& sbRes = m_storageBuffers.at(cmd.name);
                cmd.handleValue                    = sbRes.handle.GetValue();
            }

            // 룩업
            desc.readResources.insert(cmd.name);
        }

        // 읽고쓰기 정보 갱신
        desc.readWriteResources.clear();
        desc.readWriteResources.reserve(desc.readWriteBindCmds.size());
        for (ResourceBindCommand& cmd: desc.readWriteBindCmds | std::views::values)
        {
            // 바인드
            if (cmd.type == eShaderResource::Texture)
            {
                const TextureResource& texRes = m_textures.at(cmd.name);
                cmd.handleValue               = texRes.handle.GetValue();
            }
            else
            {
                const StorageBufferResource& sbRes = m_storageBuffers.at(cmd.name);
                cmd.handleValue                    = sbRes.handle.GetValue();
            }

            // 룩업
            desc.readWriteResources.insert(cmd.name);
        }

        // 상수 버퍼 바인드
        for (ConstantBufferBindCommand& cmd: desc.cbufferBindCmds | std::views::values)
        {
            const ConstantBufferResource& cbRes = m_cbuffers.at(cmd.name);
            cmd.cbh                             = cbRes.handle;
        }
    }

    ////////// 검증 ///////////

    // 디버그 검증
    // 한 패스에서 어떤 리소스가 읽기/쓰기/읽고쓰기 접근중 2개 이상으로 바인딩 하는중인가? -> 오류
    if (JAM_DEBUG_ENABLED)
    {
        StringHashMap<eResourceAccessEx> used = {};
        for (const PassDesc& desc: m_descs)
        {
            if (!desc.bEnable)   // 비활성화 패스는 패스
            {
                continue;
            }

            used.clear();
            used.reserve(desc.readBindCmds.size() + desc.readWriteBindCmds.size() + desc.writeResources.size());

            for (const ResourceBindCommand& cmd: desc.readBindCmds | std::views::values)
            {
                const auto it = used.find(cmd.name);
                JAM_ASSERT(it == used.end() || it->second == eResourceAccessEx::Read, "Pass '{}' binds resource '{}' multiple times with different access types. A resource cannot be bound more than once in a pass, even if the bind types are different.", desc.name, cmd.name);
                used[cmd.name] = eResourceAccessEx::Read;
            }

            for (const ResourceBindCommand& bind: desc.readWriteBindCmds | std::views::values)
            {
                const auto it = used.find(bind.name);
                JAM_ASSERT(it == used.end() || it->second == eResourceAccessEx::ReadWrite, "Pass '{}' binds resource '{}' multiple times with different access types. A resource cannot be bound more than once in a pass, even if the bind types are different.", desc.name, bind.name);
                used[bind.name] = eResourceAccessEx::ReadWrite;
            }

            for (const String& resource: desc.writeResources)
            {
                const auto it = used.find(resource);
                JAM_ASSERT(it == used.end() || it->second == eResourceAccessEx::Write, "Pass '{}' binds resource '{}' multiple times with different access types. A resource cannot be bound more than once in a pass, even if the bind types are different.", desc.name, resource);
                used[resource] = eResourceAccessEx::Write;
            }
        }
    }

    // 의존성 그래프를 구성하고 죽은 Pass 제거 Culling
    {
        // 어떤 리소스를 생성하는 모든 패스들을 수집
        StringHashMap<HashSet<Int32>> productors = {};
        for (Int32 i = 0; i < m_descs.size(); ++i)
        {
            const PassDesc& desc = m_descs[i];
            if (!desc.bEnable)   // 비활성화 패스는 패스
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

        // 의존성 역방향 간선
        Vector<HashSet<Int32>> revGraph(m_descs.size());
        for (Int32 i = 0; i < m_descs.size(); ++i)
        {
            const PassDesc& desc = m_descs[i];
            if (!desc.bEnable)   // 비활성화 패스는 패스
            {
                continue;
            }

            // Read Resources 의존성 검사
            for (const String& read: desc.readResources)
            {
                const auto it = productors.find(read);
                if (it != productors.end())
                {
                    for (const Int32 j: it->second)
                    {
                        if (j != i)   // 자기 자신 제외 및 중복 간선 체크
                        {
                            revGraph[i].insert(j);
                        }
                    }
                }
            }

            // ReadWrite Resources 의존성 검사
            for (const String& readWrite: desc.readWriteResources)
            {
                const auto it = productors.find(readWrite);
                if (it != productors.end())
                {
                    for (const Int32 j: it->second)
                    {
                        if (j != i)   // 자기 자신 제외 및 중복 간선 체크
                        {
                            revGraph[i].insert(j);
                        }
                    }
                }
            }
        }

        // 최종 출력 패스 수집. 부작용 패스도 최종 패스로 간주하여 제거되지 않도록 함.
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

        // 역방향 탐색 후 컬링
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

        // 최종 패스
        m_runtimePasses.clear();
        for (Int32 i = 0; i < m_descs.size(); ++i)
        {
            if (survived[i])
            {
                m_runtimePasses.push_back({ &m_descs[i] });
            }
        }
    }

    // 패스와 패스 사이 리소스 해저드 해결 및 바인딩 최적화
    // 가상 바인딩으로 직접 해저드 검출
    // 현재 compiledPasses에는 PassDesc의 포인터만 존재하는 상태 -> 이를 채워줘야 함

    HashMap<Slot, String, SlotHasher>  readSlots      = {};
    HashMap<Slot, String, SlotHasher>  readWriteSlots = {};
    HashMap<Slot, String, SlotHasher>  cbufferSlots   = {};
    HashMap<Slot, Sampler, SlotHasher> samplerSlots   = {};

    struct ResourceBindingState
    {
        eResourceAccessEx         access = eResourceAccessEx::Read;
        HashSet<Slot, SlotHasher> slots  = {};
    };

    // 리소스별로 어떤 슬롯에 어떻게 바인딩 되어있는지 추적하는 맵
    StringHashMap<ResourceBindingState> resourceBindingStates   = {};
    HashSet<Slot, SlotHasher>           tmpReadUnbindSlots      = {};
    HashSet<Slot, SlotHasher>           tmpReadWriteUnbindSlots = {};

    // 루프를 2회 도는 이유
    // 첫 번째 프레임 Pass[i]와 두 번째 프레임 Pass[i-1]는 충돌할 수 있음
    // 이걸 감지하기 위해선 2회 루프가 필요함
    // 주의, 언바인드 리스트는 위와 같은 이유로 루프를 2회 돌면서 채우지만
    // 바인드 커맨드는 첫 번째 루프에서만 채워야함.
    // 바인드 커맨드는 중복된 명령을 허용하지 않는데, 루프를 2회 돌면서 채우게 되면, 마땅히 채워야할 명령어가 최적화되어 사라지는 문제가 발생함.
    const SizeT numPasses = m_runtimePasses.size();
    for (SizeT i = 0; i < numPasses * 2; ++i)
    {
        const bool      bIsFirstLoop  = i < numPasses;
        const bool      bIsSecondLoop = !bIsFirstLoop;
        const SizeT     idx           = i % numPasses;
        Pass&           pass          = m_runtimePasses[idx];
        const PassDesc& desc          = *pass.pDesc;

        if (bIsFirstLoop)
        {
            // constant buffer
            for (const ConstantBufferBindCommand& cmd: desc.cbufferBindCmds | std::views::values)
            {
                const auto it = cbufferSlots.find(cmd.slot);
                // 빈 슬롯이거나, 슬롯은 같지만 다른 상수 버퍼를 바인딩 하는 경우에만 바인드 커맨드를 추가함.
                if (it == cbufferSlots.end() || it->second != cmd.name)
                {
                    cbufferSlots[cmd.slot] = cmd.name;
                    pass.cbufferBindCmds.push_back(cmd);
                }
            }

            // sampler state
            for (const SamplerFlagsBindCommand& cmd: desc.samplerBindCmds | std::views::values)
            {
                const auto it = samplerSlots.find(cmd.slot);
                // 빈 슬롯이거나, 슬롯은 같지만 다른 샘플러 상태를 바인딩 하는 경우에만 바인드 커맨드를 추가함.
                if (it == samplerSlots.end() || it->second != cmd.state)
                {
                    samplerSlots[cmd.slot] = cmd.state;
                    pass.samplerBindCmds.push_back(cmd);
                }
            }
        }

        // init resource unbind helper
        tmpReadUnbindSlots.clear();
        tmpReadWriteUnbindSlots.clear();

        // read resource
        for (const ResourceBindCommand& cmd: desc.readBindCmds | std::views::values)
        {
            const auto it      = resourceBindingStates.find(cmd.name);
            bool       bAddCmd = bIsFirstLoop;
            if (it != resourceBindingStates.end())   // 해당 리소스가 이미 바인드 되어있는 경우
            {
                const ResourceBindingState& state = it->second;
                if (state.access == eResourceAccessEx::ReadWrite)
                {
                    // 기존 리소스가 rw로 바인딩 되어있었다면. 모든 rw 바인드 해제
                    tmpReadWriteUnbindSlots.insert(state.slots.begin(), state.slots.end());
                }
                else if (state.access == eResourceAccessEx::Write)
                {
                    pass.bUnbindFrameBuffer = true;   // 프레임버퍼 언바인드
                }
                else if (state.access == eResourceAccessEx::Read && state.slots.contains(cmd.slot))   // 이미 같은 슬롯에 read로 바인딩 되어있다면, 굳이 바인드 커맨드를 추가할 필요 없음
                {
                    bAddCmd = false;
                }
            }

            if (bAddCmd)
            {
                pass.readBindCmds.push_back(cmd);
            }
        }

        // readwrite resource
        for (const ResourceBindCommand& cmd: desc.readWriteBindCmds | std::views::values)
        {
            const auto it      = resourceBindingStates.find(cmd.name);
            bool       bAddCmd = bIsFirstLoop;
            if (it != resourceBindingStates.end())   // 해당 리소스가 이미 바인드 되어있는 경우
            {
                const ResourceBindingState& state = it->second;
                if (state.access == eResourceAccessEx::ReadWrite)
                {
                    // 기존 리소스가 rw로 바인딩 되어있었다면. 모든 rw 바인드 해제
                    tmpReadWriteUnbindSlots.insert(state.slots.begin(), state.slots.end());
                }
                else if (state.access == eResourceAccessEx::Write)
                {
                    pass.bUnbindFrameBuffer = true;   // 프레임버퍼 언바인드
                }
                else if (state.access == eResourceAccessEx::Read && state.slots.contains(cmd.slot))   // 이미 같은 슬롯에 read로 바인딩 되어있다면, 굳이 바인드 커맨드를 추가할 필요 없음
                {
                    bAddCmd = false;
                }
            }

            if (bAddCmd)
            {
                pass.readWriteBindCmds.push_back(cmd);
            }
        }

        // write resource
        for (const String& write: desc.writeResources)
        {
            const auto it = resourceBindingStates.find(write);
            if (it != resourceBindingStates.end())   // 해당 리소스가 이미 바인드 되어있는 경우
            {
                const ResourceBindingState& state = it->second;
                if (state.access == eResourceAccessEx::ReadWrite)
                {
                    // 기존 리소스가 rw로 바인딩 되어있었다면. 모든 rw 바인드 해제
                    tmpReadWriteUnbindSlots.insert(state.slots.begin(), state.slots.end());
                }
                else if (state.access == eResourceAccessEx::Read)
                {
                    // 기존에 read로 바인딩 되어있었다면, 모든 바인드 해제
                    tmpReadUnbindSlots.insert(state.slots.begin(), state.slots.end());
                }
            }
        }

        // unbind 슬롯 갱신. 두 번째 루프의 unbind list가 최종
        if (bIsSecondLoop)
        {
            for (const Slot slot: tmpReadUnbindSlots)
            {
                pass.readUnbindSlots.push_back(slot);
            }

            for (const Slot slot: tmpReadWriteUnbindSlots)
            {
                pass.readWriteUnbindSlots.push_back(slot);
            }
        }

        // 가상 unbind

        // read resource unbind
        for (const Slot slot: tmpReadUnbindSlots)
        {
            // resource bind state 갱신
            const String&         name  = readSlots.at(slot);
            ResourceBindingState& state = resourceBindingStates.at(name);
            state.slots.erase(slot);
            if (state.slots.empty())
            {
                resourceBindingStates.erase(name);
            }

            readSlots.erase(slot);
        }

        // readwrite resource unbind
        for (const Slot slot: tmpReadWriteUnbindSlots)
        {
            // resource bind state 갱신
            const String&         name  = readWriteSlots.at(slot);
            ResourceBindingState& state = resourceBindingStates.at(name);
            state.slots.erase(slot);
            if (state.slots.empty())
            {
                resourceBindingStates.erase(name);
            }

            readWriteSlots.erase(slot);
        }

        // write resource unbind
        for (const String& write: desc.writeResources)
        {
            const auto it = resourceBindingStates.find(write);
            if (it != resourceBindingStates.end())
            {
                resourceBindingStates.erase(it);
            }
        }

        // 가상 bind
        for (const ResourceBindCommand& cmd: desc.readBindCmds | std::views::values)
        {
            readSlots[cmd.slot]                    = cmd.name;
            resourceBindingStates[cmd.name].access = eResourceAccessEx::Read;
            resourceBindingStates[cmd.name].slots.insert(cmd.slot);
        }

        for (const ResourceBindCommand& cmd: desc.readWriteBindCmds | std::views::values)
        {
            readWriteSlots[cmd.slot]               = cmd.name;
            resourceBindingStates[cmd.name].access = eResourceAccessEx::ReadWrite;
            resourceBindingStates[cmd.name].slots.insert(cmd.slot);
        }

        for (const String& write: desc.writeResources)
        {
            resourceBindingStates[write].access = eResourceAccessEx::Write;
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
}

void RenderGraph::Execute()
{
    Execute_(kInvalidHandle, Viewport { 0, 0 }, false);
}

void RenderGraph::Execute(
    const FrameBufferHandle _fbh,
    const Viewport          _vp)
{
    Execute_(_fbh, _vp, true);
}

void RenderGraph::Execute_(
    const FrameBufferHandle _fbh,
    const Viewport          _vp,
    const bool              _bUseExternalOutput)
{
    CompileIfNeed_();

    Graphics&   gfx       = GetGraphics();
    const SizeT numPasses = m_runtimePasses.size();

    for (SizeT i = 0; i < numPasses; ++i)
    {
        const Pass&     pass = m_runtimePasses[i];
        const PassDesc& desc = *pass.pDesc;

        if (desc.computePh)
        {
            ////////// 언바인드 //////////

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

            ////////// 바인드 ///////////

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
            ////////// 언바인드 //////////

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

            ////////// 바인드 ///////////

            FrameBufferHandle fbh = desc.fbh;
            Viewport          vp  = desc.viewport;
            if (_bUseExternalOutput && desc.bUseExternalOutput)
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

            // 드로우 콜
            if (desc.executor)
            {
                desc.executor();
            }
        }
    }
}

void RenderGraph::AddTexture(
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

void RenderGraph::AddTexture2D(
    const StringView                 _name,
    const Int32                      _width,
    const Int32                      _height,
    const eTextureFormat             _format,
    const Int32                      _arraySize,
    const Flags<eTextureCreateFlags> _flags,
    const Span<const MemoryView>     _initData)
{
    Graphics&           gfx  = GetGraphics();
    const TextureHandle texh = gfx.CreateTexture2D(_width, _height, _format, false, _arraySize, _flags, _initData);
    AddTexture(_name, texh, true);
}

void RenderGraph::AddTexture2D(
    const StringView                 _name,
    const float                      _resizeScale,
    const eTextureFormat             _format,
    const Int32                      _arraySize,
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
    res.arraySize     = _arraySize;
    res.flags         = _flags;
    m_bNeedCompile    = true;
}

void RenderGraph::AddTexture3D(
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
    AddTexture(_name, texh, true);
}

void RenderGraph::AddTextureCube(
    const StringView                 _name,
    const Int32                      _width,
    const Int32                      _height,
    const eTextureFormat             _format,
    const Int32                      _numCubes,
    const Flags<eTextureCreateFlags> _flags)
{
    Graphics&           gfx  = GetGraphics();
    const TextureHandle texh = gfx.CreateTextureCube(_width, _height, _format, false, _numCubes, _flags);
    AddTexture(_name, texh, true);
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
    const TextureHandle h = FindTextureOrNull(_name);
    JAM_ASSERT(h != kInvalidHandle, "Texture '{}' not found.", _name);
    return h;
}

TextureHandle RenderGraph::FindTextureOrNull(const StringView _name) const
{
    const auto it = m_textures.find(_name);
    return (it != m_textures.end()) ? it->second.handle : kInvalidHandle;
}

void RenderGraph::AddConstantBuffer(
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

void RenderGraph::AddConstantBuffer(
    const StringView _name,
    const UInt32     _byteWidth)
{
    Graphics&                  gfx = GetGraphics();
    const ConstantBufferHandle cbh = gfx.CreateConstantBuffer(_byteWidth);
    JAM_ASSERT(cbh != kInvalidHandle, "Failed to create constant buffer '{}'.", _name);
    AddConstantBuffer(_name, cbh, true);
}

void RenderGraph::RemoveConstantBuffer(
    const StringView _name)
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

ConstantBufferHandle RenderGraph::GetConstantBuffer(
    const StringView _name) const
{
    const ConstantBufferHandle h = FindConstantBufferOrNull(_name);
    JAM_ASSERT(h != kInvalidHandle, "ConstantBuffer '{}' not found.", _name);
    return h;
}

ConstantBufferHandle RenderGraph::FindConstantBufferOrNull(
    const StringView _name) const
{
    const auto it = m_cbuffers.find(_name);
    return (it != m_cbuffers.end()) ? it->second.handle : kInvalidHandle;
}

void RenderGraph::AddStorageBuffer(
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

void RenderGraph::AddRawBuffer(
    const StringView                       _name,
    const Int32                            _byteWidth,
    const Flags<eStorageBufferCreateFlags> _flags,
    const MemoryView                       _initDataOrEmpty)
{
    Graphics&                 gfx = GetGraphics();
    const StorageBufferHandle sbh = gfx.CreateRawBuffer(_byteWidth, _flags, _initDataOrEmpty);
    AddStorageBuffer(_name, sbh, true);
}

void RenderGraph::AddStructuredBuffer(
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
    AddStorageBuffer(_name, sbh, true);
}

void RenderGraph::AddTypedBuffer(
    const StringView                       _name,
    const Int32                            _numElements,
    const eStorageBufferData               _dataType,
    const Flags<eStorageBufferCreateFlags> _flags,
    const MemoryView                       _initDataOrEmpty)
{
    Graphics&                 gfx = GetGraphics();
    const StorageBufferHandle sbh = gfx.CreateTypedBuffer(_numElements, _dataType, _flags, _initDataOrEmpty);
    AddStorageBuffer(_name, sbh, true);
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
    const StorageBufferHandle h = FindStorageBufferOrNull(_name);
    JAM_ASSERT(h != kInvalidHandle, "StorageBuffer '{}' not found.", _name);
    return h;
}

StorageBufferHandle RenderGraph::FindStorageBufferOrNull(const StringView _name) const
{
    const auto it = m_storageBuffers.find(_name);
    return (it != m_storageBuffers.end()) ? it->second.handle : kInvalidHandle;
}

void RenderGraph::AddFrameBuffer(
    const StringView        _name,
    const FrameBufferHandle _fbh,
    const bool              _bOwnership)
{
    FrameBufferResource& res = m_frameBuffers[_name];
    if (res.bHasOwnership)
    {
        JAM_DESTROY_FRAME_BUFFER(res.handle);
    }
    res.name          = _name;
    res.handle        = _fbh;
    res.bHasOwnership = _bOwnership;
    res.attachments.fill({});
    res.numAttachments = 0;
    res.bAutoResize    = false;
    m_bNeedCompile     = true;
}

void RenderGraph::AddFrameBuffer(
    const StringView             _name,
    const Span<const StringView> _attachments)
{
    JAM_ASSERT(!_attachments.empty(), "FrameBuffer '{}' must have at least one attachment.", _name);

    FrameBufferResource& res = m_frameBuffers[_name];
    if (res.bHasOwnership)
    {
        JAM_DESTROY_FRAME_BUFFER(res.handle);
    }

    res.name           = _name;
    res.bHasOwnership  = true;
    res.numAttachments = static_cast<Int32>(_attachments.size());
    res.bAutoResize    = false;

    Array<TextureHandle, kMaxNumAttachmentSlots> handles = {};
    for (Int32 i = 0; i < res.numAttachments; ++i)
    {
        const StringView attachName = _attachments[i];
        res.attachments[i]          = attachName;

        const TextureResource& texRes = m_textures.at(attachName);
        if (!texRes.bAutoResize)
        {
            JAM_ASSERT(texRes.handle, "Texture '{}' not found for attachment '{}' of FrameBuffer '{}'.", attachName, attachName, _name);
            handles[i] = texRes.handle;
        }

        res.bAutoResize |= texRes.bAutoResize;
        JAM_ASSERT(res.bAutoResize == texRes.bAutoResize, "All attachments of FrameBuffer '{}' must have the same auto-resize setting. Attachment '{}' has a different setting.", _name, attachName);
    }

    // auto resize인 경우 CompileIfNeed_ 함수에서 지연 생성
    if (!res.bAutoResize)
    {
        Graphics& gfx = GetGraphics();
        res.handle    = gfx.CreateFrameBuffer({ handles.data(), res.numAttachments }, false);
    }
    m_bNeedCompile = true;
}

void RenderGraph::AddFrameBuffer(
    const StringView _name,
    const StringView _attachment)
{
    StringView attachments[] = { _attachment };
    AddFrameBuffer(_name, attachments);
}

void RenderGraph::RemoveFrameBuffer(
    const StringView _name)
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

FrameBufferHandle RenderGraph::GetFrameBuffer(
    const StringView _name) const
{
    const FrameBufferHandle h = FindFrameBufferOrNull(_name);
    JAM_ASSERT(h != kInvalidHandle, "FrameBuffer '{}' not found.", _name);
    return h;
}

FrameBufferHandle RenderGraph::FindFrameBufferOrNull(
    const StringView _name) const
{
    const auto it = m_frameBuffers.find(_name);
    return (it != m_frameBuffers.end()) ? it->second.handle : kInvalidHandle;
}

void RenderGraph::AddShader(
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

void RenderGraph::AddShader(
    const StringView _name,
    const MemoryView _bytecode)
{
    Graphics& gfx = GetGraphics();
    AddShader(_name, gfx.CreateShader(_bytecode), true);
}

void RenderGraph::RemoveShader(
    const StringView _name)
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

ShaderHandle RenderGraph::GetShader(
    const StringView _name) const
{
    const ShaderHandle h = FindShaderOrNull(_name);
    JAM_ASSERT(h != kInvalidHandle, "Shader '{}' not found.", _name);
    return h;
}

ShaderHandle RenderGraph::FindShaderOrNull(
    const StringView _name) const
{
    const auto it = m_shaders.find(_name);
    return (it != m_shaders.end()) ? it->second.handle : kInvalidHandle;
}

void RenderGraph::AddProgram(
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

void RenderGraph::AddProgram(
    const StringView _name,
    const StringView _vsOrEmpty,
    const StringView _psOrEmpty)
{
    AddProgram(_name, _vsOrEmpty, _psOrEmpty, {}, {}, {});
}

void RenderGraph::AddProgram(
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
    AddProgram(_name, ph, true);
}

void RenderGraph::AddComputeProgram(
    const StringView _name,
    const StringView _csName)
{
    const ShaderHandle  csh = GetShader(_csName);
    Graphics&           gfx = GetGraphics();
    const ProgramHandle ph  = gfx.CreateProgram(csh);
    AddProgram(_name, ph, true);
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

ProgramHandle RenderGraph::GetProgram(
    const StringView _name) const
{
    const ProgramHandle h = FindProgramOrNull(_name);
    JAM_ASSERT(h != kInvalidHandle, "Program '{}' not found.", _name);
    return h;
}

ProgramHandle RenderGraph::FindProgramOrNull(
    const StringView _name) const
{
    const auto it = m_programs.find(_name);
    return (it != m_programs.end()) ? it->second.handle : kInvalidHandle;
}

void RenderGraph::Clear(
    const Flags<eRenderGraphClearFlags> _flags)
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

RenderGraph::PassDesc* RenderGraph::FindPassDesc_(
    const StringView name)
{
    const auto it = std::ranges::find_if(m_descs, [name](const PassDesc& _desc) { return _desc.name == name; });
    return (it != m_descs.end()) ? &(*it) : nullptr;
}

bool RenderGraph::HasPass_(StringView name) const
{
    return std::ranges::any_of(m_descs, [name](const PassDesc& pd) { return pd.name == name; });
}

////////// PASS BUILDER ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RenderGraph::PassBuilder::PassBuilder(
    RenderGraph& _graph,
    PassDesc&    _desc)
    : m_graph(_graph)
    , m_desc(_desc)
{
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetProgram(const StringView _name)
{
    JAM_ASSERT(!_name.IsEmpty(), "Program name cannot be empty.");

    m_desc.programName = _name;
    m_desc.ph          = kInvalidHandle;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::ReleaseProgram()
{
    m_desc.programName.Clear();
    m_desc.ph = kInvalidHandle;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetFrameBuffer(
    const StringView _name)
{
    JAM_ASSERT(!_name.IsEmpty(), "FrameBuffer name cannot be empty.");

    m_desc.frameBufferName = _name;
    m_desc.fbh             = kInvalidHandle;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::ReleaseFrameBuffer()
{
    m_desc.frameBufferName.Clear();
    m_desc.fbh = kInvalidHandle;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetViewport(
    const Viewport _vp)
{
    m_desc.viewport                               = _vp;
    m_desc.bUseViewportFromFrameBufferResoulution = false;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetViewportFromFrameBufferResolution()
{
    m_desc.bUseViewportFromFrameBufferResoulution = true;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetScissorRect(const Rect _rect)
{
    m_desc.scissorRect = _rect;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetAsFinalPass(
    const bool _bEnable)
{
    m_desc.bFinalPass = _bEnable;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetUseExternalOutput(
    const bool _bEnable)
{
    m_desc.bUseExternalOutput = _bEnable;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetEnabled(
    const bool _bEnable)
{
    m_desc.bEnable = _bEnable;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetRenderFlags(
    const Flags<eRenderFlags> _flags)
{
    m_desc.renderFlags = _flags;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetStencilFlags(
    const Flags<eStencilFlags> _fstencil,
    const Flags<eStencilFlags> _bstencil,
    const UInt8                _ref)
{
    m_desc.fstencil   = _fstencil;
    m_desc.bstencil   = _bstencil;
    m_desc.stencilRef = _ref;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetBlendFlags(
    const Flags<eBlendFlags> _flags,
    const Int32              _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < kMaxNumRenderTargetViewSlots, "Blend slot {} out of range.", _slot);

    for (Int32 i = 0; i < m_desc.numBlendFlagsBindCmds; ++i)
    {
        if (m_desc.blendFlagsBindCmds[i].slot == _slot)
        {
            m_desc.blendFlagsBindCmds[i].state = _flags;
            return *this;
        }
    }

    JAM_ASSERT(m_desc.numBlendFlagsBindCmds < kMaxNumRenderTargetViewSlots, "Too many blend states.");
    m_desc.blendFlagsBindCmds[m_desc.numBlendFlagsBindCmds++] = { _flags, _slot };
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetBlendFactor(
    const RGBA _factor)
{
    m_desc.blendFactor = _factor;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetRenderTargetClearCommand(
    const RGBA  _color,
    const Int32 _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < kMaxNumRenderTargetViewSlots, "ClearColor slot {} out of range.", _slot);

    for (Int32 i = 0; i < m_desc.numRenderTargetClearCmds; ++i)
    {
        if (m_desc.renderTargetClearCmds[i].slot == _slot)
        {
            m_desc.renderTargetClearCmds[i].color = _color;
            return *this;
        }
    }

    m_desc.renderTargetClearCmds[m_desc.numRenderTargetClearCmds++] = { _color, _slot };
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::ReleaseRenderTargetCommand(
    const Int32 _slot)
{
    for (Int32 i = 0; i < m_desc.numRenderTargetClearCmds; ++i)
    {
        if (m_desc.renderTargetClearCmds[i].slot == _slot)
        {
            const Int32 last                   = m_desc.numRenderTargetClearCmds - 1;
            m_desc.renderTargetClearCmds[i]    = m_desc.renderTargetClearCmds[last];
            m_desc.renderTargetClearCmds[last] = {};
            --m_desc.numRenderTargetClearCmds;
            break;
        }
    }
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetDepthClearCommand(const float _value)
{
    m_desc.depthClearValue = _value;
    m_desc.bClearDepth     = true;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::ReleaseDepthClearCommand()
{
    m_desc.bClearDepth = false;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetStencilClearCommand(const UInt8 _value)
{
    m_desc.stencilClearValue = _value;
    m_desc.bClearStencil     = true;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::ReleaseStencilClearCommand()
{
    m_desc.bClearStencil = false;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetTexture(
    const StringView      _name,
    const eShader         _stage,
    const Int32           _slot,
    const eResourceAccess _access)
{
    JAM_ASSERT(m_graph.m_textures.contains(_name), "Texture '{}' not registered.", _name);

    const Slot slot = { _stage, _slot };
    if (_access == eResourceAccess::Read)
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumShaderResourceViewSlots, "Texture slot {} out of range.", _slot);
        m_desc.readBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Texture, _stage, _slot };
    }
    else
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumUnorderedAccessViewSlots, "Texture slot {} out of range.", _slot);
        m_desc.readWriteBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Texture, _stage, _slot };
    }
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetBuffer(
    const StringView      _name,
    const eShader         _stage,
    const Int32           _slot,
    const eResourceAccess _access)
{
    JAM_ASSERT(m_graph.m_storageBuffers.contains(_name), "StorageBuffer '{}' not registered.", _name);

    const Slot slot = { _stage, _slot };
    if (_access == eResourceAccess::Read)
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumShaderResourceViewSlots, "Buffer slot {} out of range.", _slot);
        m_desc.readBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Buffer, _stage, _slot };
    }
    else
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumUnorderedAccessViewSlots, "Buffer slot {} out of range.", _slot);
        m_desc.readWriteBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Buffer, _stage, _slot };
    }
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::ReleaseResource(
    const eShader         _stage,
    const Int32           _slot,
    const eResourceAccess _access)
{
    const Slot slot = { _stage, _slot };
    if (_access == eResourceAccess::Read)
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumShaderResourceViewSlots, "Resource slot {} out of range.", _slot);
        m_desc.readBindCmds.erase(slot);
    }
    else
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumUnorderedAccessViewSlots, "Resource slot {} out of range.", _slot);
        m_desc.readWriteBindCmds.erase(slot);
    }
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetConstantBuffer(
    const StringView _name,
    const eShader    _stage,
    const Int32      _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < kMaxNumConstantBufferSlots, "CBuffer slot {} out of range.", _slot);
    JAM_ASSERT(m_graph.m_cbuffers.contains(_name), "ConstantBuffer '{}' not registered.", _name);

    const Slot slot              = { _stage, _slot };
    m_desc.cbufferBindCmds[slot] = { _name, kInvalidHandle, slot };
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::ReleaseConstantBuffer(
    const eShader _stage,
    const Int32   _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < kMaxNumConstantBufferSlots, "CBuffer slot {} out of range.", _slot);
    m_desc.cbufferBindCmds.erase({ _stage, _slot });
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetSampler(
    const Flags<eSamplerFlags> _state,
    const eShader              _stage,
    const Int32                _slot)
{
    return SetSampler(_state, RGBA::kBlack, _stage, _slot);
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetSampler(
    const Flags<eSamplerFlags> _state,
    const RGBA                 _borderColor,
    const eShader              _stage,
    const Int32                _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < kMaxNumSamplerSlots, "Sampler slot {} out of range.", _slot);
    const Slot slot              = { _stage, _slot };
    m_desc.samplerBindCmds[slot] = { _state, _borderColor, _stage, _slot };
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::ReleaseSamplerFlags(
    const eShader _stage,
    const Int32   _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < kMaxNumSamplerSlots, "Sampler slot {} out of range.", _slot);
    m_desc.samplerBindCmds.erase({ _stage, _slot });
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetExecutor(
    const Delegate<void()>& _executor)
{
    JAM_ASSERT(_executor, "Executor cannot be null.");
    m_desc.executor = _executor;
    return *this;
}

////////// COMPUTE PASS BUILDER ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RenderGraph::ComputePassBuilder::ComputePassBuilder(
    RenderGraph& _graph,
    PassDesc&    _desc)
    : m_graph(_graph)
    , m_desc(_desc)
{
    _desc.bIsComputePass = true;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::SetAsFinalPass(
    const bool _bEnable)
{
    m_desc.bFinalPass = _bEnable;
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::SetEnable(
    const bool _bEnable)
{
    m_desc.bEnable = _bEnable;
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::SetComputeProgram(
    const StringView _name)
{
    JAM_ASSERT(!_name.IsEmpty(), "Compute program name cannot be empty.");

    m_desc.computeProgramName = _name;
    m_desc.computePh          = kInvalidHandle;
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::ReleaseComputeProgram()
{
    m_desc.computeProgramName.Clear();
    m_desc.computePh = kInvalidHandle;
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
        m_desc.readBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Texture, slot };
    }
    else
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumUnorderedAccessViewSlots, "Texture slot {} out of range.", _slot);
        m_desc.readWriteBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Texture, slot };
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
        m_desc.readBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Buffer, slot };
    }
    else
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumUnorderedAccessViewSlots, "Buffer slot {} out of range.", _slot);
        m_desc.readWriteBindCmds[slot] = { _name, kInvalidHandleValue, eShaderResource::Buffer, slot };
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
        m_desc.readBindCmds.erase(slot);
    }
    else
    {
        JAM_ASSERT(_slot >= 0 && _slot < kMaxNumUnorderedAccessViewSlots, "Buffer slot {} out of range.", _slot);
        m_desc.readWriteBindCmds.erase(slot);
    }
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::SetConstantBuffer(
    const StringView _name,
    const Int32      _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < kMaxNumConstantBufferSlots, "CBuffer slot {} out of range.", _slot);
    JAM_ASSERT(m_graph.m_cbuffers.contains(_name), "ConstantBuffer '{}' not registered.", _name);
    const Slot slot              = { eShader::ComputeShader, _slot };
    m_desc.cbufferBindCmds[slot] = { _name, kInvalidHandle, eShader::ComputeShader, _slot };
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::ReleaseConstantBuffer(
    const Int32 _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < kMaxNumConstantBufferSlots, "CBuffer slot {} out of range.", _slot);
    m_desc.cbufferBindCmds.erase({ eShader::ComputeShader, _slot });
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::ReleaseSamplerState(
    const Int32 _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < kMaxNumSamplerSlots, "Sampler slot {} out of range.", _slot);
    m_desc.samplerBindCmds.erase({ eShader::ComputeShader, _slot });
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
    const Slot slot              = { eShader::ComputeShader, _slot };
    m_desc.samplerBindCmds[slot] = { _state, _borderColor, eShader::ComputeShader, _slot };
    return *this;
}

RenderGraph::ComputePassBuilder& RenderGraph::ComputePassBuilder::SetNumThreadGroups(
    const Int32 _numThreadGroupsX,
    const Int32 _numThreadGroupsY,
    const Int32 _numThreadGroupsZ)
{
    JAM_ASSERT(_numThreadGroupsX > 0 && _numThreadGroupsY > 0 && _numThreadGroupsZ > 0, "Number of thread groups must be greater than zero.");
    m_desc.numThreadGroupsX = _numThreadGroupsX;
    m_desc.numThreadGroupsY = _numThreadGroupsY;
    m_desc.numThreadGroupsZ = _numThreadGroupsZ;
    return *this;
}

}   // namespace Jam