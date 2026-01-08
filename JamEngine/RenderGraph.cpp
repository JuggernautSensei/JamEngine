#include "pch.h"

#include "Graphics.h"
#include "RenderGraph.h"

namespace jam
{

RenderGraph& RenderGraph::AddPass(
    RenderPass&& _pass)
{
    JAM_ASSERT(!m_bCompiled, "Cannot add pass to a compiled RenderGraph.");
    m_nodes.emplace_back(std::move(_pass));
    return *this;
}

void RenderGraph::Compile()
{
    JAM_ASSERT(!m_bCompiled, "RenderGraph has already been compiled.");

    const size_t numNodes = m_nodes.size();
    JAM_ASSERT(numNodes > 0, "RenderGraph must have at least one pass to compile.");

    if (numNodes == 1)
    {
        // 단일 패스는 의존성 관리 필요없음
        JAM_ASSERT(m_nodes[0].pass.IsValid(), "RenderGraph with a single pass contains an invalid RenderPass.");
    }
    else
    {
        AnalyzePassDependencies_();       // 패스 의존성 분석
        DeduplicateResourceBindings_();   // 리소스 바인딩 중복 제거
    }

    m_bCompiled = true;
}

void RenderGraph::DeduplicateResourceBindings_()
{
    const Int32 numNodes = m_nodes.size();
    JAM_ASSERT(numNodes > 1, "RenderGraph must have at least two passes to deduplicate resource bindings.");

    // deduplicate constant buffers and samplers
    for (Int32 i = 0; i < numNodes; ++i)
    {
        RenderPass& pass = m_nodes[i].pass;

        // 이전 패스를 순회하며 중복된 상수 버퍼 바인딩 제거
        eastl::erase_if(pass.m_cbufferBinds,
                        [this, i](const ConstantBufferBind& _bind)
                        {
                            for (Int32 j = i - 1; j >= 0; --j)
                            {
                                for (const ConstantBufferBind& prevBind: m_nodes[j].pass.m_cbufferBinds)
                                {
                                    // 동일 스테이지 + 동일 슬롯에 바인딩된 상수 버퍼가 있다면 중복
                                    // 가장 근접한 경우만 체크하면 됨
                                    if (prevBind.stage == _bind.stage && prevBind.slot == _bind.slot)
                                    {
                                        return prevBind.cbh == _bind.cbh;
                                    }
                                }
                            }

                            // 중복된 바인딩이 없음
                            return false;
                        });

        // 이전 패스를 순회하며 중복된 샘플러 바인딩 제거
        eastl::erase_if(pass.m_samplerBinds,
                        [this, i](const SamplerBind& _bind)
                        {
                            for (Int32 j = i - 1; j >= 0; --j)
                            {
                                for (const SamplerBind& prevBind: m_nodes[j].pass.m_samplerBinds)
                                {
                                    // 동일 스테이지 + 동일 슬롯에 바인딩된 샘플러가 있다면 중복
                                    // 가장 근접한 경우만 체크하면 됨
                                    if (prevBind.stage == _bind.stage && prevBind.slot == _bind.slot)
                                    {
                                        return MemEqual(prevBind.state, _bind.state);
                                    }
                                }
                            }

                            // 중복된 바인딩이 없음
                            return false;
                        });
    }

    // 가상으로 패스를 실행시키며 중복된 텍스처 바인딩 제거
    HashMap<UInt32, TextureHandle> prevTexBindMap = {};
    for (Int32 i = 0; i < numNodes; ++i)
    {
        HashMap<UInt32, TextureHandle> texBindMap = prevTexBindMap;   // 현재 바인딩 정보
        Node&                          node       = m_nodes[i];
        RenderPass&                    pass       = node.pass;

        Vector<TextureBind> textureBinds;
        textureBinds.reserve(pass.m_textureBinds.size());

        // 유의미한 텍스처 바인딩만 수집
        for (const TextureBind& bind: pass.m_textureBinds)
        {
            const UInt32 key = CreateStageSlotKey_(bind.stage, bind.slot);
            const auto   it  = texBindMap.find(key);

            // 같은 쉐이더 + 슬롯에 같은 텍스처가 바인딩되어 있는 경우 바인딩 스킵
            if (it != texBindMap.end() && it->second == bind.texh)
            {
                continue;
            }

            // add binding
            textureBinds.emplace_back(bind);
            texBindMap[key] = bind.texh;
        }

        // 유의미한 바인딩으로 교체
        pass.m_textureBinds.swap(textureBinds);
        prevTexBindMap.swap(texBindMap);   // 현재 바인딩 상태로 업데이트

        // unbinding 시뮬레이션
        if (node.bNeedUnbindResources)
        {
            for (const BindInfo& info: node.unbindTextures)
            {
                const UInt32 key = CreateStageSlotKey_(info.stage, info.slot);
                prevTexBindMap.erase(key);
            }
        }
    }
}

void RenderGraph::AnalyzePassDependencies_()
{
    const Int32 numNodes = m_nodes.size();
    JAM_ASSERT(numNodes > 1, "RenderGraph must have at least two passes to analyze pass dependencies.");

    /*
    의존성 1: 현재 입력 텍스처가 다른 패스의 출력으로 사용되는가?
            -> 현재 패스를 제외한 모든 패스를 순회하며 확인
            -> 해당 슬롯에 다른 텍스처가 덮어씌워지는지 확인 -> yes: 충돌 가능성 없음
            -> no 인데 출력과 충돌하는 경우 -> unbind list에 추가
    의존성 2: 현재 출력 텍스처가 다음 패스의 입력으로 사용되는가?
            -> 다음 패스만 확인해도 충분
    */

    HashMap<UInt32, const TextureBind*> texhMap;         // key: texture handle value
    HashMap<UInt32, const TextureBind*> stageSlotMsp;    // key: stage + slot
    HashSet<UInt32>                     attachmentSet;   // key: texture handle value

    for (Int32 i = 0; i < numNodes; ++i)
    {
        // 초기화
        texhMap.clear();
        stageSlotMsp.clear();
        attachmentSet.clear();

        // 현재 패스
        Node&             node = m_nodes[i];
        const RenderPass& pass = node.pass;
        JAM_ASSERT(pass.IsValid(), "RenderPass at index {} is not valid.", i);

        // 의존성 1: 입력 텍스처 충돌 검사
        {
            // 현재 패스의 입력 텍스처 바인딩 수집, 입력 텍스처가 다음의 출력 텍스처로 사용되면 Conflict
            for (const TextureBind& bind: pass.m_textureBinds)
            {
                JAM_ASSERT(JAM_IS_VALID_HANDLE(bind.texh), "RenderPass at index {} has invalid texture handle.", i);
                const UInt32 key = CreateStageSlotKey_(bind.stage, bind.slot);

                texhMap.emplace(bind.texh.value, &bind);
                stageSlotMsp.emplace(key, &bind);
            }

            // 모든 패스들을 순회하며 충돌해결
            for (Int32 j = 1; j < numNodes; ++j)
            {
                const Int32       idx      = (i + j) % numNodes;
                const RenderPass& nextPass = m_nodes[idx].pass;

                // 같은 슬롯 + 쉐이더에 다른 텍스처가 바인딩 되면 conflict 발생하지 않음
                for (const TextureBind& nextPassBind: nextPass.m_textureBinds)
                {
                    JAM_ASSERT(JAM_IS_VALID_HANDLE(nextPassBind.texh), "RenderPass at index {} has invalid texture handle.", idx);
                    const UInt32 key = CreateStageSlotKey_(nextPassBind.stage, nextPassBind.slot);
                    const auto   it  = stageSlotMsp.find(key);

                    // 같은 슬롯 + 쉐이더에 다른 텍스처가 바인딩 되는 경우 추적 종료 (overwrite)
                    if (it != stageSlotMsp.end() && it->second->texh != nextPassBind.texh)
                    {
                        texhMap.erase(it->second->texh.value);
                        stageSlotMsp.erase(it);
                    }
                }

                // 인풋 텍스처가 모두 해결되었으면 종료
                if (texhMap.empty())
                {
                    break;
                }

                // 다음 패스의 출력 프레임버퍼 확인.
                // 유효하지 않으면 스킵 (= 백버퍼라는 의미. 백버퍼 자원은 입력으로 사용할 수 없음. 즉, 충돌 불가)
                if (!JAM_IS_VALID_HANDLE(nextPass.m_fbh))
                {
                    continue;
                }

                // 다음 패스의 프레임버퍼와 충돌 확인
                const FrameBufferInfo nextPassFbInfo = Graphics::GetFrameBufferInfo(nextPass.m_fbh);
                for (Int32 k = 0; k < nextPassFbInfo.numRenderTargets; ++k)
                {
                    const TextureHandle texh = nextPassFbInfo.renderTargetTexhs[k];
                    const auto          it   = texhMap.find(texh.value);

                    // 만약 충돌하면, unbindTextures에 추가
                    if (it != texhMap.end())
                    {
                        const TextureBind* bind = it->second;
                        const UInt32       key  = CreateStageSlotKey_(bind->stage, bind->slot);

                        node.bNeedUnbindResources = true;
                        node.unbindTextures.emplace_back(bind->stage, bind->slot);   // unbind conflicting texture

                        // 해결됨. 추적종료
                        texhMap.erase(texh.value);
                        stageSlotMsp.erase(key);
                    }
                }

                // depth stencil 충돌 확인
                if (JAM_IS_VALID_HANDLE(nextPassFbInfo.depthStencilTexh))
                {
                    const TextureHandle texh = nextPassFbInfo.depthStencilTexh;
                    const auto          it   = texhMap.find(texh.value);

                    if (it != texhMap.end())
                    {
                        const TextureBind* bind = it->second;
                        const UInt32       key  = CreateStageSlotKey_(bind->stage, bind->slot);

                        node.bNeedUnbindResources = true;
                        node.unbindTextures.emplace_back(bind->stage, bind->slot);

                        // remove from tracking
                        texhMap.erase(texh.value);
                        stageSlotMsp.erase(key);
                    }
                }

                // 모든 인풋 텍스처가 해결되었으면 종료
                if (texhMap.empty())
                {
                    break;
                }
            }
        }

        // 의존성 2: 출력 텍스처 충돌 검사
        {
            // 현재 패스의 출력이 다음 입력으로 사용되는지
            // 만약, 충돌한다면 프레임버퍼 언바인딩 필요

            // 유효하지 않은 프레임버퍼는 스킵
            if (!JAM_IS_VALID_HANDLE(pass.m_fbh))
            {
                continue;
            }

            // 현재 패스의 출력 프레임버퍼 어태치먼트 수집
            const FrameBufferInfo fbInfo = Graphics::GetFrameBufferInfo(pass.m_fbh);
            for (Int32 k = 0; k < fbInfo.numRenderTargets; ++k)
            {
                attachmentSet.emplace(fbInfo.renderTargetTexhs[k].value);
            }

            if (JAM_IS_VALID_HANDLE(fbInfo.depthStencilTexh))
            {
                attachmentSet.emplace(fbInfo.depthStencilTexh.value);
            }

            // 다음 패스의 입력 텍스처 바인딩과 충돌 확인
            const Int32       nextIdx  = (i + 1) % numNodes;
            const RenderPass& nextPass = m_nodes[nextIdx].pass;
            for (const TextureBind& nextPassBind: nextPass.m_textureBinds)
            {
                // 충돌 발견
                if (attachmentSet.contains(nextPassBind.texh.value))
                {
                    node.bNeedUnbindResources   = true;
                    node.bNeedUnbindFrameBuffer = true;
                    break;   // 프레임버퍼는 하나이므로 하나만 발견해도 충분
                }
            }
        }
    }
}

void RenderGraph::Execute() const
{
    JAM_ASSERT(m_bCompiled, "RenderGraph must be compiled before execution.");

    for (const Node& node: m_nodes)
    {
        node.pass.Execute();
        UnbindPassResourcesIfNeed_(node);
    }
}

void RenderGraph::Execute(const FrameBufferHandle _fbh, const Viewport _viewport) const
{
    JAM_ASSERT(m_bCompiled, "RenderGraph must be compiled before execution.");

    const Int32 lastIdx = m_nodes.size() - 1;

    for (Int32 i = 0; i <= lastIdx; ++i)
    {
        const Node& node = m_nodes[i];
        if (i == lastIdx)
        {
            node.pass.Execute(_fbh, _viewport);
        }
        else
        {
            node.pass.Execute();
        }

        UnbindPassResourcesIfNeed_(node);
    }
}

void RenderGraph::UnbindPassResourcesIfNeed_(const Node& _node) const
{
    if (!_node.bNeedUnbindResources)
    {
        return;
    }

    if (_node.bNeedUnbindFrameBuffer)
    {
        Graphics::SetFrameBuffer(k_invalidHandle);
    }

    for (const BindInfo& info: _node.unbindTextures)
    {
        Graphics::SetTexture(k_invalidHandle, info.stage, info.slot);
    }

    Graphics::Flush();
}

UInt32 RenderGraph::CreateStageSlotKey_(
    const eShader _stage,
    const Int32   _slot) const
{
    return (static_cast<UInt32>(_stage) << 16) | static_cast<UInt32>(_slot);
}

void RenderGraph::Reset()
{
    m_nodes.clear();
    m_bCompiled = false;
}

bool RenderGraph::IsCompiled() const
{
    return m_bCompiled;
}

}   // namespace jam
