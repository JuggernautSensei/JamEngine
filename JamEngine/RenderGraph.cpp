#include "pch.h"
#include "RenderGraph.h"

#include "Graphics.h"
#include "ImGuiEngine.h"
#include "TinyRingBuffer.h"

namespace Jam
{

RenderGraph::~RenderGraph()
{
    Reset_();
}

RenderGraph::PassBuilder RenderGraph::AddPass(const StringView _name)
{
    JAM_ASSERT(!HasPass_(_name), "Pass '{}' already exists.", _name);

    PassDesc& desc = m_passDescs.emplace_back();
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

void RenderGraph::RemovePass(const StringView _name)
{
    if (eastl::erase_if(m_passDescs, [&](const PassDesc& d) { return d.name == _name; }) == 0)
    {
        JAM_ASSERT(0, "Pass '{}' not found.", _name);
    }

    m_bNeedCompile = true;
}

void RenderGraph::Compile(const Int32 _width, const Int32 _height)
{
    JAM_ASSERT(_width > 0 && _height > 0, "Invalid back buffer size ({}, {}).", _width, _height);
    JAM_ASSERT(!m_passDescs.empty(), "RenderGraph must have at least one pass.");

    Graphics&  gfx         = GetGraphics();
    const bool bNeedResize = (m_lastWidth != _width || m_lastHeight != _height);

    // Resize와 컴파일 둘 다 필요없다면 early out
    if (!m_bNeedCompile && !bNeedResize)
    {
        return;
    }

    // 1. 필요시 Resize
    if (bNeedResize)
    {
        m_lastWidth  = _width;
        m_lastHeight = _height;

        // 중복 생성 방지
        StringHashSet<> resizedTextures = {};
        resizedTextures.reserve(m_textures.size());

        StringHashSet<> resizedFrameBuffers = {};
        resizedFrameBuffers.reserve(m_frameBuffers.size());

        for (const PassDesc& desc: m_passDescs)
        {
            // 빈 이름 = 백버퍼.
            // JamEngine의 경우. 리사이즈 전에 백버퍼가 반드시 리사이즈 되며.
            // 백버퍼를 따로 입력으로 사용할 수 없도록 설계되어있음. (백엔드 설계상 막아놓음)
            if (desc.frameBufferName.IsEmpty() || resizedFrameBuffers.contains(desc.frameBufferName))
            {
                continue;
            }

            auto fbIt = m_frameBuffers.find(desc.frameBufferName);
            JAM_ASSERT(fbIt != m_frameBuffers.end(), "FrameBuffer '{}' not found.", desc.frameBufferName);
            FrameBufferResource& fbRes = fbIt->second;

            bool bAnyResized = false;
            for (Int32 i = 0; i < fbRes.numAttachments; ++i)
            {
                const String& attachName = fbRes.attachments[i];
                if (resizedTextures.contains(attachName))
                {
                    bAnyResized = true;
                    continue;   // 이미 리사이된 텍스처
                }

                auto texIt = m_textures.find(attachName);
                JAM_ASSERT(texIt != m_textures.end(), "Texture '{}' not found.", attachName);
                TextureResource& texRes = texIt->second;
                if (!texRes.bAutoResize)
                {
                    continue;   // 자동 리사이즈가 비활성화된 텍스처
                }

                const Int32 w = NumericCast<Int32>(Ceil(_width * texRes.resizeScale));
                const Int32 h = NumericCast<Int32>(Ceil(_height * texRes.resizeScale));
                JAM_ASSERT(w > 0 && h > 0, "Auto-resize produced invalid size ({}, {}) for '{}'.", w, h, attachName);

                JAM_DESTROY_TEXTURE(texRes.handle);
                texRes.handle = gfx.CreateTexture2D(w, h, texRes.format, texRes.arraySize, texRes.flags);
                resizedTextures.insert(attachName);
                bAnyResized = true;
            }

            // 텍스처가 하나도 리사이즈되지 않았다면 스킵
            if (!bAnyResized)
            {
                continue;
            }

            // 텍스처가 바뀌었으면 프레임 버퍼도 재생성
            Array<TextureHandle, k_maxNumAttachmentSlots> handles = {};
            for (Int32 i = 0; i < fbRes.numAttachments; ++i)
            {
                auto texIt = m_textures.find(fbRes.attachments[i]);
                JAM_ASSERT(texIt != m_textures.end(), "Texture not found during FB rebuild.");
                handles[i] = texIt->second.handle;
            }

            JAM_DESTROY_FRAME_BUFFER(fbRes.handle);
            fbRes.handle = gfx.CreateFrameBuffer({ handles.data(), fbRes.numAttachments }, false);
            resizedFrameBuffers.insert(desc.frameBufferName);
        }
    }

    // Name -> Handle 매핑 갱신. 리사이즈 후 혹은 컴파일 전 PassDesc의 spec을 바꿨을 경우.
    for (PassDesc& desc: m_passDescs)
    {
        JAM_ASSERT(desc.executor, "Pass '{}' has no executor.", desc.name);

        // 프로그램
        if (!desc.programName.IsEmpty() && !desc.ph)
        {
            const auto it = m_programs.find(desc.programName);
            JAM_ASSERT(it != m_programs.end(), "Program '{}' not found.", desc.programName);
            desc.ph = it->second.handle;
        }

        // 외부에서 출력을 주입받는 형태라면, 출력 fallback으로 백버퍼 설정
        if (desc.bUseExternalOutput)
        {
            const Vec2i res = gfx.GetBackBufferResolution();
            desc.viewport   = { 0, 0, NumericCast<UInt16>(res.x), NumericCast<UInt16>(res.y) };
            desc.fbh        = k_nullHandle;
        }
        else
        {
            // 프레임 버퍼. 리사이즈 후라면 항상 갱신
            if (!desc.frameBufferName.IsEmpty() && (!desc.fbh || bNeedResize))
            {
                const auto it = m_frameBuffers.find(desc.frameBufferName);
                JAM_ASSERT(it != m_frameBuffers.end(), "FrameBuffer '{}' not found.", desc.frameBufferName);
                desc.fbh = it->second.handle;
                JAM_ASSERT(desc.fbh, "FrameBuffer handle is null in pass '{}'.", desc.name);
            }

            // 뷰포트 자동 설정
            if (desc.bUseFrameBufferResolutionAsViewport)
            {
                if (desc.fbh)
                {
                    const FrameBufferInfo& fbInfo = gfx.GetFrameBufferInfo(desc.fbh);
                    const TextureHandle    texh   = fbInfo.bHasDepthStencil ? fbInfo.depthStencilTexh : fbInfo.renderTargetTexhs[0];

                    // framebuffer는 무조건 하나의 이상의 텍스처를 가지고 있음.
                    const TextureInfo& texInfo = gfx.GetTextureInfo(texh);
                    desc.viewport              = { 0, 0, NumericCast<UInt16>(texInfo.width), NumericCast<UInt16>(texInfo.height) };
                }
                else   // 백버퍼를 사용
                {
                    const Vec2i res = gfx.GetBackBufferResolution();
                    desc.viewport   = { 0, 0, NumericCast<UInt16>(res.x), NumericCast<UInt16>(res.y) };
                }
            }
        }

        // 텍스처 바인드
        for (TextureBind& bind: desc.textureBinds | std::views::values)
        {
            // 리사이즈가 된 경우, 특정 핸들이 바뀌었을 수도 있으므로 갱신
            if (bind.texh && !bNeedResize)
            {
                continue;
            }

            const auto it = m_textures.find(bind.name);
            JAM_ASSERT(it != m_textures.end(), "Texture '{}' not found.", bind.name);
            bind.texh = it->second.handle;
        }

        // 상수 버퍼 바인드
        for (ConstantBufferBind& bind: desc.cbufferBinds | std::views::values)
        {
            if (bind.cbh)
            {
                continue;
            }

            const auto it = m_cbuffers.find(bind.name);
            JAM_ASSERT(it != m_cbuffers.end(), "ConstantBuffer '{}' not found.", bind.name);
            bind.cbh = it->second.handle;
        }
    }

    // 리사이즈만 된 경우 핸들 갱신만으로 충분
    if (bNeedResize && !m_bNeedCompile)
    {
        return;
    }

    // 검증 전, 각 패스의 writers 갱신
    for (PassDesc& desc: m_passDescs)
    {
        desc.numWriteTextures = 0;
        if (desc.frameBufferName.IsEmpty())   // 백버퍼
        {
            continue;
        }

        const auto it = m_frameBuffers.find(desc.frameBufferName);
        JAM_ASSERT(it != m_frameBuffers.end(), "FrameBuffer '{}' not found.", desc.frameBufferName);
        const FrameBufferResource& fbRes = it->second;
        for (Int32 i = 0; i < fbRes.numAttachments; ++i)
        {
            desc.writeTextures[desc.numWriteTextures++] = fbRes.attachments[i];
        }
    }

    // 디버그 검증
    // 1. 한 패스에서 입력과 출력이 겹치는가?
    // 2. 한 텍스처가 read -> write -> read 혹은 write -> read -> write 되는 패턴이 있는가?
    //    해당 패턴은 위상정렬이 꼬이는 패턴. 실제로 발생할순 있지만, 잘 쓰이는 패턴도 아니고, 지원하려면 구현 복잡도가 꽤 올라가기 때문에, 일단은 금지함.
    if (JAM_DEBUG_ENABLED)
    {
        // 패스 개개의 검증.
        // 입력과 출력에 동일한 텍스처를 사용하는지 체크

        StringHashSet<> inputTextures = {};
        inputTextures.reserve(m_textures.size());
        for (const PassDesc& desc: m_passDescs)
        {
            if (desc.frameBufferName.IsEmpty())
            {
                continue;
            }

            inputTextures.clear();
            for (const TextureBind& bind: desc.textureBinds | std::views::values)
            {
                inputTextures.insert(bind.name);
            }

            const auto fbIt = m_frameBuffers.find(desc.frameBufferName);
            JAM_ASSERT(fbIt != m_frameBuffers.end(), "FrameBuffer '{}' not found.", desc.frameBufferName);
            const FrameBufferResource& fbRes = fbIt->second;
            for (Int32 i = 0; i < fbRes.numAttachments; ++i)
            {
                if (inputTextures.contains(fbRes.attachments[i]))
                {
                    JAM_ASSERT(0, "Validation failed: Pass '{}' reads and writes the same texture '{}'. In-place read-write is not supported.", desc.name, fbRes.attachments[i]);
                }
            }
        }

        // TexA에 대해서
        // TexA에 쓰기 -> TexA을 읽기 -> TexA에 쓰기. (한 그래프 내에서 읽기와 쓰기가 반복되는 경우)
        // 이런 경우는 실제로는 발생할 수 있지만, 현재 구현에서는 지원하지 않음.
        // 현재 구현에서는 위상 정렬이 꼬여버림. 실제로 잘 쓰지않는 패턴이기도 하고, 지원하려면 구현 복잡도가 꽤 올라가기 때문에, 일단은 금지함.
        // 흔히 쓰이는 포스트 프로세스의 핑퐁 패턴도, 패스마다 텍스처를 따로 만들어서 해결할 수 있기 때문에, 큰 제약은 아님.

        enum class eAccessType
        {
            Read,
            Write
        };

        struct State
        {
            bool        bWritten       = false;   // 지금까지의 패스들이 이 텍스처에 대해서 write한 적이 있는가?
            bool        bRead          = false;   // 지금까지의 패스들이 이 텍스처에 대해서 read한 적이 있는가?
            eAccessType lastAccessType = {};      // 마지막으로 이 텍스처에 접근한 방식. 읽기인지 쓰기인지.
        };

        StringHashMap<State> textureStates = {};
        textureStates.reserve(m_textures.size());
        for (const PassDesc& desc: m_passDescs)
        {
            // 이미 read된 적 있는 게 있으면 오류로 처리
            for (Int32 j = 0; j < desc.numWriteTextures; ++j)
            {
                const String& texture = desc.writeTextures[j];
                State&        state   = textureStates[texture];
                if (state.bWritten && state.lastAccessType == eAccessType::Read)   // write -> reader -> write (error)
                {
                    JAM_ASSERT(0, "Validation failed: Texture '{}' is written after being read in pass '{}'. Ping-ponging is not supported.", texture, desc.name);
                }

                state.bWritten       = true;
                state.lastAccessType = eAccessType::Write;
            }

            // 이미 write된 적 있는 게 있으면 오류로 처리
            for (const TextureBind& bind: desc.textureBinds | std::views::values)
            {
                const String& texture = bind.name;
                State&        state   = textureStates[texture];
                if (state.bRead && state.lastAccessType == eAccessType::Write)   // read -> writer -> read (error)
                {
                    JAM_ASSERT(0, "Validation failed: Texture '{}' is read after being written in pass '{}'. Ping-ponging is not supported.", texture, desc.name);
                }

                state.bRead          = true;
                state.lastAccessType = eAccessType::Read;
            }
        }
    }

    // 위상 정렬 kahn 알고리즘
    // https://en.wikipedia.org/wiki/Topological_sorting#Kahn's_algorithm
    // 1. 의존성 분석을 위해서 출력 텍스처를 최후로 write하는 Pass 목록을 만든다.
    //    여러 Pass가 같은 텍스처를 write할 수 있지만, 마지막에 write하는 Pass가 가장 중요하기 때문에, 1개의 pass만 기록해도 충분하다.
    // 2. 각 Pass의 read 텍스처를 순회하며, 해당 텍스처를 write하는 Pass가 있다면 간선을 구성한다.
    //    이 방향성 있는 간선은 의존성을 의미한다. 이는 인접리스트 형태로 구성된다.
    // 3. 2.에서 구성된 그래프를 바탕으로 kahn 위상 정렬을 수행한다.
    // 4. 최종 출력에 영향을 못미치는 Pass를 컬링한다. 이는 최종 출력에 영향을 미치는 Pass에서 역방향으로 그래프를 탐색해서, 도달 가능한 Pass만 살리는 방식으로 구현한다.
    {
        const Int32 numPasses = NumericCast<Int32>(m_passDescs.size());

        // 텍스처를 최후로 write하는 Pass 인덱스 목록.
        StringHashMap<Int32> textureWriter = {};
        textureWriter.reserve(m_textures.size());
        for (Int32 i = 0; i < numPasses; ++i)
        {
            PassDesc& desc = m_passDescs[i];
            for (Int32 j = 0; j < desc.numWriteTextures; ++j)
            {
                const String& texture  = desc.writeTextures[j];
                textureWriter[texture] = i;
            }
        }

        // 인접 리스트 형태로 구성
        Vector<Vector<Int32>> edges(numPasses);                   // 간선. Pass_i에서 j로 나가는 간선의 개수
        Vector<Int32>         inDegree(numPasses, 0);             // 차수. Pass_i로 들어오는 간선의 개수
        Vector<bool>          dupEdgeChecker(numPasses, false);   // 간선 중복 방지용. 패스마다 초기화해서 사용

        // 내 입력을 출력하는 Pass들을 찾아서 간선 구성
        for (Int32 i = 0; i < numPasses; ++i)
        {
            std::ranges::fill(dupEdgeChecker, false);   // 패스마다 초기화

            PassDesc& desc = m_passDescs[i];
            for (const TextureBind& bind: desc.textureBinds | std::views::values)
            {
                const String& readTexture = bind.name;
                const auto    it          = textureWriter.find(readTexture);
                if (it == textureWriter.end())   // read texture에 대한 writer가 없다? -> 의존성이 없는 리소스.
                {
                    continue;
                }

                // 자기 자신에게 간선이 생기는 경우는, 해당 Pass가 같은 텍스처를 read와 write 둘 다 하는 경우임. 이는 오류
                const Int32 j = it->second;
                JAM_ASSERT(j != i, "Pass '{}' both reads and writes texture '{}'. In-place read-write is not supported.", desc.name, readTexture);
                if (dupEdgeChecker[j])
                {
                    continue;
                }

                edges[j].push_back(i);      // j -> i. 의존성
                dupEdgeChecker[j] = true;   // 중복 간선 방지
                ++inDegree[i];
            }
        }

        // 위상 정렬 결과. 값은 Pass 인덱스
        Vector<Int32> sortedIdx = {};
        sortedIdx.reserve(numPasses);

        // in-degree가 0인 Pass들
        Tiny::RingBuffer<Int32> zeroInDegrees = {};
        zeroInDegrees.Reserve(numPasses);
        for (Int32 i = 0; i < numPasses; ++i)
        {
            if (inDegree[i] == 0)
            {
                zeroInDegrees.PushBack(i);
            }
        }

        // in-degree가 0인 Pass부터
        while (!zeroInDegrees.IsEmpty())
        {
            const Int32 cur = zeroInDegrees.Front();
            zeroInDegrees.PopFront();
            sortedIdx.push_back(cur);

            // cur에서 나가는 간선 제거.
            for (const Int32 next: edges[cur])
            {
                if (--inDegree[next] == 0)   // InDegree가 0이 된 다음 Pass는 실행 준비 완료된 Pass이므로, 큐에 삽입
                {
                    zeroInDegrees.PushBack(next);
                }
            }
        }

        JAM_ASSERT(CmpEqual(sortedIdx.size(), numPasses), "Cyclic dependency detected in RenderGraph! {} passes sorted, {} descared)", sortedIdx.size(), numPasses);

        // 컬링. 출력에 영향을 미치지 못하는 Pass는 제거.
        // 만약, 출력에 영향을 미치지 못하더라도 남기고 싶다면 bSideEffect 를 체크해놔야함.

        // 역방향 간선 구성
        Vector<bool>          alive(numPasses, false);
        Vector<Vector<Int32>> revEdges(numPasses);
        for (Int32 i = 0; i < numPasses; ++i)
        {
            for (const Int32 j: edges[i])
            {
                revEdges[j].push_back(i);
            }
        }

        // bSideEffect 혹은 위상정렬된 마지막 패스를 출력으로 간주
        Vector<Int32> stack = {};
        stack.reserve(numPasses);
        for (Int32 i = 0; i < numPasses; ++i)
        {
            if (m_passDescs[i].bSideEffect || m_passDescs[i].bUseExternalOutput)   // 외부 출력이 있는 패스의 경우도 무조건 살아남아야함.
            {
                stack.push_back(i);
            }
        }

        if (!sortedIdx.empty())
        {
            stack.push_back(sortedIdx.back());
        }

        // 컬링 수행
        while (!stack.empty())
        {
            const Int32 cur = stack.back();
            stack.pop_back();
            if (alive[cur])
            {
                continue;
            }

            alive[cur] = true;
            for (const Int32 prev: revEdges[cur])   // 역방향 간선
            {
                if (!alive[prev])
                {
                    stack.push_back(prev);
                }
            }
        }   // 도달하지 못했다면 출력에 영향을 미치지 못하는 패스라는 뜻. 컬링.

        // 결과 수집
        m_compiledPasses.clear();
        m_compiledPasses.reserve(numPasses);
        for (const Int32 idx: sortedIdx)
        {
            if (!alive[idx])
            {
                continue;   // 컬링
            }

            Pass& pass = m_compiledPasses.emplace_back();
            pass.pDesc = &m_passDescs[idx];
        }
    }

    // Pass 간의 리소스 충돌을 분석하여 언바인드 커맨드 생성
    // 케이스 1: 이전 입력이 다음 패스의 출력과 겹치는 경우
    // 케이스 2: 이전 출력이 다음 패스의 입력과 겹치는 경우
    {
        // 언바인드 리스트 초기화
        for (Pass& cp: m_compiledPasses)
        {
            cp.unbindTextures.clear();
            cp.bUnbindFrameBuffer = false;
        }

        const SizeT numPasses = m_compiledPasses.size();

        // 케이스 1
        // 슬롯 x를 가장 마지막에 바인딩하는 패스 이후에 x를 언바인드하는 커맨드 생성
        // 단, 읽기만 하는 텍스처는 언바인드할 필요가 없음.

        // write하는 텍스처 목록 수집.
        StringHashSet<> writeTextures = {};
        writeTextures.reserve(m_textures.size());
        for (SizeT i = 0; i < numPasses; ++i)
        {
            const PassDesc& desc = *m_compiledPasses[i].pDesc;
            for (Int32 j = 0; j < desc.numWriteTextures; ++j)
            {
                const String& writeTexture = desc.writeTextures[j];
                writeTextures.insert(writeTexture);
            }
        }

        // 각 (staget, slot) 쌍에 대해서 마지막으로 바인딩하는 패스 인덱스를 추적
        HashMap<Slot, SizeT, SlotHasher> lastBindPassIdx = {};
        lastBindPassIdx.reserve(64);
        for (SizeT i = 0; i < numPasses; ++i)
        {
            const PassDesc& desc = *m_compiledPasses[i].pDesc;
            for (const TextureBind& bind: desc.textureBinds | std::views::values)
            {
                if (!writeTextures.contains(bind.name))
                {
                    continue;   // 읽기만 하는 텍스처는 언바인드할 필요가 없음
                }

                const Slot slot       = { bind.stage, bind.slot };
                lastBindPassIdx[slot] = i;
            }
        }

        // 각 슬롯을 마지막으로 사용하는 패스에 언바인드 삽입
        for (const auto& [slot, passIdx]: lastBindPassIdx)
        {
            m_compiledPasses[passIdx].unbindTextures.push_back(slot);
        }

        // 케이스 2
        // 프레임 버퍼의 attachment 텍스처가 이후 패스에서 읽기로 사용되는 경우, 해당 패스에 프레임 버퍼 언바인드 커맨드 생성
        for (SizeT i = 0; i < numPasses; ++i)
        {
            const PassDesc& desc = *m_compiledPasses[i].pDesc;
            if (desc.frameBufferName.IsEmpty() || desc.bUseExternalOutput)   // bUseExternalOutput = true: render graph 외부 리소스가 바인딩되기 때문에 충돌 분석에서 제외
            {
                continue;
            }

            bool bNeedUnbind = false;
            for (SizeT j = i + 1; j < numPasses && !bNeedUnbind; ++j)
            {
                const PassDesc& laterDesc = *m_compiledPasses[j].pDesc;
                for (Int32 k = 0; k < desc.numWriteTextures; ++k)
                {
                    const String& texture = desc.writeTextures[k];
                    for (const TextureBind& bind: laterDesc.textureBinds | std::views::values)
                    {
                        if (bind.name == texture)
                        {
                            bNeedUnbind = true;
                            break;
                        }
                    }

                    if (bNeedUnbind)
                    {
                        break;
                    }
                }
            }

            m_compiledPasses[i].bUnbindFrameBuffer = bNeedUnbind;
        }
    }

    // 바인딩 최적화. 중복된 바인딩 제거. 가상 바인딩 테이블을 만들어서 추적
    {
        HashMap<Slot, String, SlotHasher>       texInputs = {};
        HashMap<Slot, String, SlotHasher>       cbInputs  = {};
        HashMap<Slot, SamplerState, SlotHasher> samInputs = {};

        texInputs.reserve(64);
        cbInputs.reserve(64);
        samInputs.reserve(64);

        for (Pass& cp: m_compiledPasses)
        {
            const PassDesc& desc = *cp.pDesc;

            cp.textures.clear();
            cp.cbuffers.clear();
            cp.samplers.clear();

            for (const TextureBind& bind: desc.textureBinds | std::views::values)
            {
                const Slot slot = { bind.stage, bind.slot };
                const auto it   = texInputs.find(slot);
                if (it == texInputs.end() || it->second != bind.name)
                {
                    cp.textures.push_back(bind);
                    texInputs[slot] = bind.name;
                }
            }

            for (const ConstantBufferBind& bind: desc.cbufferBinds | std::views::values)
            {
                const Slot slot = { bind.stage, bind.slot };
                const auto it   = cbInputs.find(slot);
                if (it == cbInputs.end() || it->second != bind.name)
                {
                    cp.cbuffers.push_back(bind);
                    cbInputs[slot] = bind.name;
                }
            }

            for (const SamplerBind& bind: desc.samplerBinds | std::views::values)
            {
                const Slot slot = { bind.stage, bind.slot };
                const auto it   = samInputs.find(slot);
                if (it == samInputs.end() || !MemEqual(it->second, bind.state))
                {
                    cp.samplers.push_back(bind);
                    samInputs[slot] = bind.state;
                }
            }

            // 언바인드된 슬롯은 가상 테이블에서도 제거
            for (const Slot& slot: cp.unbindTextures)
            {
                texInputs.erase(slot);
            }
        }
    }

    m_bNeedCompile = false;
}

void RenderGraph::Execute()
{
    JAM_ASSERT(!m_bNeedCompile, "Call Compile() before Execute().");
    const auto [w, h] = GetGraphics().GetBackBufferResolution();
    Execute(k_nullHandle, { 0, 0, NumericCast<UInt16>(w), NumericCast<UInt16>(h) });
}

void RenderGraph::Execute(
    const FrameBufferHandle _fbh,
    const Viewport          _vp)
{
    JAM_ASSERT(!m_bNeedCompile, "Call Compile() before Execute().");

    Graphics&   gfx       = GetGraphics();
    const SizeT numPasses = m_compiledPasses.size();

    for (SizeT i = 0; i < numPasses; ++i)
    {
        const Pass&     pass = m_compiledPasses[i];
        const PassDesc& desc = *pass.pDesc;
        JAM_ASSERT(desc.executor, "Pass '{}' has no executor.", desc.name);

        const FrameBufferHandle fbh = desc.bUseExternalOutput ? _fbh : desc.fbh;
        const Viewport          vp  = desc.bUseExternalOutput ? _vp : desc.viewport;

        gfx.SetProgram(desc.ph);
        gfx.SetFrameBuffer(fbh);
        gfx.SetViewport(vp);
        gfx.SetRenderState(desc.renderState);
        gfx.SetStencil(desc.fstencil, desc.bstencil, desc.stencilRef);

        for (Int32 j = 0; j < desc.numBlendBinds; ++j)
        {
            const BlendStateBind& bind = desc.blendBinds[j];
            gfx.SetBlendState(bind.state, bind.slot);
        }

        gfx.SetBlendFactor(desc.blendFactor);

        for (Int32 j = 0; j < desc.numClearColorBinds; ++j)
        {
            const ClearColorBind& bind = desc.clearColorBinds[j];
            gfx.ClearColor(fbh, bind.color, bind.slot);
        }

        if (desc.bClearDepth || desc.bClearStencil)
        {
            gfx.ClearDepthStencil(fbh, desc.bClearDepth, desc.bClearStencil, desc.clearDepthValue, desc.clearStencilValue);
        }

        for (const ConstantBufferBind& bind: pass.cbuffers)
        {
            gfx.SetConstantBuffer(bind.cbh, bind.stage, bind.slot);
        }

        for (const SamplerBind& bind: pass.samplers)
        {
            gfx.SetSampler(bind.state, bind.stage, bind.slot);
        }

        for (const TextureBind& bind: pass.textures)
        {
            gfx.SetTexture(bind.texh, bind.stage, bind.slot);
        }

        // 드로우 콜
        desc.executor();

        // 자동 언바인드
        for (const Slot& slot: pass.unbindTextures)
        {
            gfx.SetTexture(k_nullHandle, slot.stage, slot.index);
        }

        if (pass.bUnbindFrameBuffer)
        {
            gfx.SetFrameBuffer(k_nullHandle);
        }
    }
}

bool RenderGraph::NeedCompile() const
{
    return m_bNeedCompile;
}

void RenderGraph::AddTexture(
    const StringView    _name,
    const TextureHandle _texh,
    const bool          _bOwnership)
{
    JAM_ASSERT(!m_textures.contains(_name), "Texture '{}' already registered.", _name);

    TextureResource& res = m_textures[_name];
    res.name             = _name;
    res.handle           = _texh;
    res.bHasOwnership    = _bOwnership;
    m_bNeedCompile       = true;
}

void RenderGraph::AddTexture2D(
    const StringView                 _name,
    const Int32                      _width,
    const Int32                      _height,
    const eTextureFormat             _format,
    const Int32                      _arraySize,
    const Flags<eTextureCreateFlags> _flags,
    const Span<const MemoryView>   _initData)
{
    const TextureHandle texh = GetGraphics().CreateTexture2D(_width, _height, _format, _arraySize, _flags, _initData);
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
    JAM_ASSERT(!m_textures.contains(_name), "Texture '{}' already registered.", _name);

    TextureResource& res = m_textures[_name];
    res.name             = _name;
    res.bHasOwnership    = true;
    res.bAutoResize      = true;
    res.resizeScale      = _resizeScale;
    res.format           = _format;
    res.arraySize        = _arraySize;
    res.flags            = _flags;
    m_bNeedCompile       = true;
}

void RenderGraph::AddTexture3D(
    const StringView                 _name,
    const Int32                      _width,
    const Int32                      _height,
    const Int32                      _depth,
    const eTextureFormat             _format,
    const Flags<eTextureCreateFlags> _flags,
    const Span<const MemoryView>   _initData)
{
    const TextureHandle texh = GetGraphics().CreateTexture3D(_width, _height, _depth, _format, _flags, _initData);
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
    const TextureHandle texh = GetGraphics().CreateTextureCube(_width, _height, _format, _numCubes, _flags);
    AddTexture(_name, texh, true);
}

TextureHandle RenderGraph::GetTexture(const StringView _name) const
{
    const TextureHandle h = FindTextureOrNull(_name);
    JAM_ASSERT(h != k_nullHandle, "Texture '{}' not found.", _name);
    return h;
}

TextureHandle RenderGraph::FindTextureOrNull(const StringView _name) const
{
    const auto it = m_textures.find(_name);
    return (it != m_textures.end()) ? it->second.handle : k_nullHandle;
}

void RenderGraph::AddConstantBuffer(
    const StringView           _name,
    const ConstantBufferHandle _cbh,
    const bool                 _bOwnership)
{
    JAM_ASSERT(!m_cbuffers.contains(_name), "ConstantBuffer '{}' already registered.", _name);

    ConstantBufferResource& res = m_cbuffers[_name];
    res.name                    = _name;
    res.handle                  = _cbh;
    res.bHasOwnership           = _bOwnership;
    m_bNeedCompile              = true;
}

void RenderGraph::AddConstantBuffer(const StringView _name, const UInt32 _byteWidth)
{
    const ConstantBufferHandle cbh = GetGraphics().CreateConstantBuffer(_byteWidth);
    JAM_ASSERT(cbh != k_nullHandle, "Failed to create constant buffer '{}'.", _name);
    AddConstantBuffer(_name, cbh, true);
}

ConstantBufferHandle RenderGraph::GetConstantBuffer(const StringView _name) const
{
    const ConstantBufferHandle h = FindConstantBufferOrNull(_name);
    JAM_ASSERT(h != k_nullHandle, "ConstantBuffer '{}' not found.", _name);
    return h;
}

ConstantBufferHandle RenderGraph::FindConstantBufferOrNull(const StringView _name) const
{
    const auto it = m_cbuffers.find(_name);
    return (it != m_cbuffers.end()) ? it->second.handle : k_nullHandle;
}

void RenderGraph::AddFrameBuffer(
    const StringView        _name,
    const FrameBufferHandle _fbh,
    const bool              _bOwnership)
{
    JAM_ASSERT(!m_frameBuffers.contains(_name), "FrameBuffer '{}' already registered.", _name);
    SetFrameBuffer(_name, _fbh, _bOwnership);
}

void RenderGraph::SetFrameBuffer(
    const StringView        _name,
    const FrameBufferHandle _fbh,
    const bool              _bOwnership)
{
    FrameBufferResource& res = m_frameBuffers[_name];
    res.name                 = _name;
    res.handle               = _fbh;
    res.bHasOwnership        = _bOwnership;
}

void RenderGraph::AddFrameBuffer(
    const StringView             _name,
    const Span<const StringView> _attachments)
{
    JAM_ASSERT(!m_frameBuffers.contains(_name), "FrameBuffer '{}' already registered.", _name);
    SetFrameBuffer(_name, _attachments);
}

void RenderGraph::SetFrameBuffer(
    const StringView             _name,
    const Span<const StringView> _attachments)
{
    JAM_ASSERT(!_attachments.empty(), "FrameBuffer '{}' must have at least one attachment.", _name);

    Array<TextureHandle, k_maxNumAttachmentSlots> handles = {};
    FrameBufferResource&                          res     = m_frameBuffers[_name];
    res.name                                              = _name;
    res.bHasOwnership                                     = true;
    res.numAttachments                                    = NumericCast<Int32>(_attachments.size());

    for (Int32 i = 0; i < res.numAttachments; ++i)
    {
        const StringView attachName = _attachments[i];
        const auto       texIt      = m_textures.find(attachName);
        JAM_ASSERT(texIt != m_textures.end(), "Attachment texture '{}' not found.", attachName);

        handles[i]         = texIt->second.handle;
        res.attachments[i] = attachName;
    }

    res.handle     = GetGraphics().CreateFrameBuffer({ handles.data(), res.numAttachments }, false);
    m_bNeedCompile = true;
}

FrameBufferHandle RenderGraph::GetFrameBuffer(const StringView _name) const
{
    const FrameBufferHandle h = FindFrameBufferOrNull(_name);
    JAM_ASSERT(h != k_nullHandle, "FrameBuffer '{}' not found.", _name);
    return h;
}

FrameBufferHandle RenderGraph::FindFrameBufferOrNull(const StringView _name) const
{
    const auto it = m_frameBuffers.find(_name);
    return (it != m_frameBuffers.end()) ? it->second.handle : k_nullHandle;
}

void RenderGraph::AddShader(
    const StringView   _name,
    const ShaderHandle _sh,
    const bool         _bOwnership)
{
    JAM_ASSERT(!m_shaders.contains(_name), "Shader '{}' already registered.", _name);

    ShaderResource& res = m_shaders[_name];
    res.name            = _name;
    res.handle          = _sh;
    res.bHasOwnership   = _bOwnership;
    m_bNeedCompile      = true;
}

void RenderGraph::AddShader(const StringView _name, const MemoryView _bytecode)
{
    AddShader(_name, GetGraphics().CreateShader(_bytecode), true);
}

ShaderHandle RenderGraph::GetShader(const StringView _name) const
{
    const ShaderHandle h = FindShaderOrNull(_name);
    JAM_ASSERT(h != k_nullHandle, "Shader '{}' not found.", _name);
    return h;
}

ShaderHandle RenderGraph::FindShaderOrNull(const StringView _name) const
{
    const auto it = m_shaders.find(_name);
    return (it != m_shaders.end()) ? it->second.handle : k_nullHandle;
}

void RenderGraph::AddProgram(
    const StringView    _name,
    const ProgramHandle _ph,
    const bool          _bOwnership)
{
    JAM_ASSERT(!m_programs.contains(_name), "Program '{}' already registered.", _name);

    ProgramResource& res = m_programs[_name];
    res.name             = _name;
    res.handle           = _ph;
    res.bHasOwnership    = _bOwnership;
    m_bNeedCompile       = true;
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
    const ShaderHandle  vsh = _vsOrEmpty.IsEmpty() ? k_nullHandle : GetShader(_vsOrEmpty);
    const ShaderHandle  psh = _psOrEmpty.IsEmpty() ? k_nullHandle : GetShader(_psOrEmpty);
    const ShaderHandle  gsh = _gsOrEmpty.IsEmpty() ? k_nullHandle : GetShader(_gsOrEmpty);
    const ShaderHandle  hsh = _hsOrEmpty.IsEmpty() ? k_nullHandle : GetShader(_hsOrEmpty);
    const ShaderHandle  dsh = _dsOrEmpty.IsEmpty() ? k_nullHandle : GetShader(_dsOrEmpty);
    const ProgramHandle ph  = GetGraphics().CreateProgram(vsh, psh, dsh, hsh, gsh);
    AddProgram(_name, ph, true);
}

ProgramHandle RenderGraph::GetProgram(
    const StringView _name) const
{
    const ProgramHandle h = FindProgramOrNull(_name);
    JAM_ASSERT(h != k_nullHandle, "Program '{}' not found.", _name);
    return h;
}

ProgramHandle RenderGraph::FindProgramOrNull(
    const StringView _name) const
{
    const auto it = m_programs.find(_name);
    return (it != m_programs.end()) ? it->second.handle : k_nullHandle;
}

void RenderGraph::Reset_()
{
    m_passDescs.clear();
    m_compiledPasses.clear();

    for (auto& res: m_cbuffers | std::views::values)
    {
        if (res.bHasOwnership)
        {
            JAM_DESTROY_CONSTANT_BUFFER(res.handle);
        }
    }
    m_cbuffers.clear();

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

    for (auto& res: m_shaders | std::views::values)
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

RenderGraph::PassDesc* RenderGraph::FindPassDesc_(StringView name)
{
    const auto it = std::ranges::find_if(m_passDescs, [name](const PassDesc& pd) { return pd.name == name; });
    return (it != m_passDescs.end()) ? &(*it) : nullptr;
}

bool RenderGraph::HasPass_(StringView name) const
{
    return std::ranges::any_of(m_passDescs, [name](const PassDesc& pd) { return pd.name == name; });
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetSideEffect(const bool _bEnable)
{
    m_desc.bSideEffect = _bEnable;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetProgram(const StringView _name)
{
    JAM_ASSERT(!_name.IsEmpty(), "Program name cannot be empty.");
    JAM_ASSERT(m_graph.m_programs.contains(_name), "Program '{}' not registered.", _name);
    m_desc.programName = _name;
    m_desc.ph          = k_nullHandle;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::ReleaseProgram()
{
    m_desc.programName.Clear();
    m_desc.ph = k_nullHandle;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetFrameBuffer(
    const StringView _name)
{
    JAM_ASSERT(!_name.IsEmpty(), "FrameBuffer name cannot be empty.");
    JAM_ASSERT(m_graph.m_frameBuffers.contains(_name), "FrameBuffer '{}' not registered.", _name);

    // m_desc.writeTextures는 compile 시점에 갱신됨
    m_desc.frameBufferName = _name;
    m_desc.fbh             = k_nullHandle;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::ReleaseFrameBuffer()
{
    // m_desc.writeTextures는 compile 시점에 갱신됨
    m_desc.frameBufferName.Clear();
    m_desc.fbh = k_nullHandle;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetViewport(
    const Viewport _vp)
{
    m_desc.viewport                            = _vp;
    m_desc.bUseFrameBufferResolutionAsViewport = false;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetFrameBufferResolutionAsViewport()
{
    m_desc.bUseFrameBufferResolutionAsViewport = true;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetUseExternalOutput(
    const bool _bUseExternalOutput)
{
    m_desc.bUseExternalOutput = _bUseExternalOutput;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetRenderState(const RenderState _state)
{
    m_desc.renderState = _state;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetStencilState(
    const StencilState _front,
    const StencilState _back,
    const UInt8        _ref)
{
    m_desc.fstencil   = _front;
    m_desc.bstencil   = _back;
    m_desc.stencilRef = _ref;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetBlendState(
    const BlendState _state,
    const Int32      _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < k_maxNumRenderTargetViewSlots, "Blend slot {} out of range.", _slot);

    for (Int32 i = 0; i < m_desc.numBlendBinds; ++i)
    {
        if (m_desc.blendBinds[i].slot == _slot)
        {
            m_desc.blendBinds[i].state = _state;
            return *this;
        }
    }

    JAM_ASSERT(m_desc.numBlendBinds < k_maxNumRenderTargetViewSlots, "Too many blend states.");
    m_desc.blendBinds[m_desc.numBlendBinds++] = { _state, _slot };
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetBlendFactor(const RGBA _factor)
{
    m_desc.blendFactor = _factor;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetClearColor(
    const RGBA  _color,
    const Int32 _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < k_maxNumRenderTargetViewSlots, "ClearColor slot {} out of range.", _slot);

    for (Int32 i = 0; i < m_desc.numClearColorBinds; ++i)
    {
        if (m_desc.clearColorBinds[i].slot == _slot)
        {
            m_desc.clearColorBinds[i].color = _color;
            return *this;
        }
    }

    JAM_ASSERT(m_desc.numClearColorBinds < k_maxNumRenderTargetViewSlots, "Too many clear colors.");
    m_desc.clearColorBinds[m_desc.numClearColorBinds++] = { _color, _slot };
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::ReleaseClearColor(const Int32 _slot)
{
    for (Int32 i = 0; i < m_desc.numClearColorBinds; ++i)
    {
        if (m_desc.clearColorBinds[i].slot == _slot)
        {
            const Int32 last             = m_desc.numClearColorBinds - 1;
            m_desc.clearColorBinds[i]    = m_desc.clearColorBinds[last];
            m_desc.clearColorBinds[last] = {};
            --m_desc.numClearColorBinds;
            break;
        }
    }
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetClearDepth(const float _value)
{
    m_desc.clearDepthValue = _value;
    m_desc.bClearDepth     = true;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::ReleaseClearDepth()
{
    m_desc.bClearDepth = false;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetClearStencil(const UInt8 _value)
{
    m_desc.clearStencilValue = _value;
    m_desc.bClearStencil     = true;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::ReleaseClearStencil()
{
    m_desc.bClearStencil = false;
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetTexture(
    const StringView _name,
    const eShader    _stage,
    const Int32      _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < k_maxNumShaderResourceViewSlots, "Texture slot {} out of range.", _slot);
    JAM_ASSERT(m_graph.m_textures.contains(_name), "Texture '{}' not registered.", _name);
    const Slot slot           = { _stage, _slot };
    m_desc.textureBinds[slot] = { _name, k_nullHandle, _stage, _slot };
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetConstantBuffer(
    const StringView _name,
    const eShader    _stage,
    const Int32      _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < k_maxNumConstantBufferSlots, "CBuffer slot {} out of range.", _slot);
    JAM_ASSERT(m_graph.m_cbuffers.contains(_name), "ConstantBuffer '{}' not registered.", _name);

    const Slot slot           = { _stage, _slot };
    m_desc.cbufferBinds[slot] = { _name, k_nullHandle, _stage, _slot };
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetSamplerState(
    const SamplerState _state,
    const eShader      _stage,
    const Int32        _slot)
{
    JAM_ASSERT(_slot >= 0 && _slot < k_maxNumSamplerSlots, "Sampler slot {} out of range.", _slot);

    const Slot slot           = { _stage, _slot };
    m_desc.samplerBinds[slot] = { _state, _stage, _slot };
    return *this;
}

RenderGraph::PassBuilder& RenderGraph::PassBuilder::SetExecutor(
    const Delegate<void()>& _executor)
{
    JAM_ASSERT(_executor, "Executor cannot be null.");
    m_desc.executor = _executor;
    return *this;
}

}   // namespace Jam