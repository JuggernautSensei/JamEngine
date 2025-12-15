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
        JAM_ASSERT(m_nodes[0].pass.Validate(), "RenderGraph with a single pass contains an invalid RenderPass.");
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
    const Int32 numNodes = NarrowCast<Int32>(m_nodes.size());
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
                                    return prevBind.stage == _bind.stage
                                        && prevBind.slot == _bind.slot
                                        && prevBind.cbh == _bind.cbh;
                                }
                            }
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
                                    return prevBind.stage == _bind.stage
                                        && prevBind.slot == _bind.slot
                                        && MemEqual(prevBind.state, _bind.state);
                                }
                            }
                            return false;
                        });
    }

    // 가상으로 패스를 실행시키며 중복된 텍스처 바인딩 제거
    HashMap<UInt32, TextureHandle> prevTexBinds = {};
    for (Int32 i = 0; i < numNodes; ++i)
    {
        HashMap<UInt32, TextureHandle> texBinds = prevTexBinds;   // 현재 바인딩 정보
        Node&                          node     = m_nodes[i];
        RenderPass&                    pass     = node.pass;

        Vector<TextureBind> textureBinds;
        textureBinds.reserve(pass.m_textureBinds.size());

        // 유의미한 텍스처 바인딩만 수집
        for (const TextureBind& bind: pass.m_textureBinds)
        {
            const UInt32 key = CreateStageSlotKey_(bind.stage, bind.slot);
            const auto   it  = texBinds.find(key);

            // 같은 쉐이더 + 슬롯에 같은 텍스처가 바인딩되어 있는 경우 바인딩 스킵
            if (it != texBinds.end() && it->second == bind.texh)
            {
                continue;
            }

            // add binding
            textureBinds.emplace_back(bind);
            texBinds[key] = bind.texh;
        }

        // 유의미한 바인딩으로 교체
        pass.m_textureBinds.swap(textureBinds);
        prevTexBinds.swap(texBinds);   // 현재 바인딩 상태로 업데이트

        // unbinding
        if (node.bNeedUnbindResources)
        {
            for (const BindInfo& info: node.unbindTextures)
            {
                const UInt32 key = CreateStageSlotKey_(info.stage, info.slot);
                prevTexBinds.erase(key);
            }
        }
    }
}

void RenderGraph::AnalyzePassDependencies_()
{
    const Int32 numNodes = NarrowCast<Int32>(m_nodes.size());
    JAM_ASSERT(numNodes > 1, "RenderGraph must have at least two passes to analyze pass dependencies.");

    HashMap<UInt32, const TextureBind*> inputTextures;   // key: texture handle value, value: TextureBind*
    HashMap<UInt32, const TextureBind*> stageSlots;      // key: stage+slot, value: TextureBind*
    HashSet<UInt32>                     attachments;     // key: texture handle value

    for (Int32 i = 0; i < numNodes; ++i)
    {
        // 초기화
        inputTextures.clear();
        stageSlots.clear();
        attachments.clear();

        Node&             node = m_nodes[i];
        const RenderPass& pass = node.pass;
        JAM_ASSERT(pass.Validate(), "RenderPass at index {} is not valid.", i);

        // 현재 패스의 입력 텍스처 바인딩 수집
        // 입력 텍스처가 다음의 출력 텍스처로 사용되면 Conflict
        for (const TextureBind& bind: pass.m_textureBinds)
        {
            JAM_ASSERT(JAM_IS_VALID_HANDLE(bind.texh), "RenderPass at index {} has invalid texture handle.", i);
            const UInt32 key = CreateStageSlotKey_(bind.stage, bind.slot);

            inputTextures.emplace(bind.texh.value, &bind);
            stageSlots.emplace(key, &bind);
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
                const auto   it  = stageSlots.find(key);

                // 같은 슬롯 + 쉐이더에 다른 텍스처 바인딩.
                if (it != stageSlots.end() && it->second->texh != nextPassBind.texh)
                {
                    inputTextures.erase(it->second->texh.value);
                    stageSlots.erase(it);
                }
            }

            // 인풋 텍스처가 모두 해결되었으면 종료
            if (inputTextures.empty())
            {
                break;
            }

            // 다음 패스의 출력 프레임버퍼 확인.
            if (!JAM_IS_VALID_HANDLE(nextPass.m_fbh))
            {
                continue;
            }

            // 다음 패스의 프레임버퍼와 충돌 확인
            const FrameBufferInfo nextPassFbInfo = Graphics::GetFrameBufferInfo(nextPass.m_fbh);
            for (Int32 k = 0; k < nextPassFbInfo.numRenderTargets; ++k)
            {
                const TextureHandle texh = nextPassFbInfo.renderTargetTexhs[k];
                const auto          it   = inputTextures.find(texh.value);

                // 만약 충돌하면, unbindTextures에 추가
                if (it != inputTextures.end())
                {
                    const TextureBind* bind = it->second;
                    const UInt32       key  = CreateStageSlotKey_(bind->stage, bind->slot);

                    node.bNeedUnbindResources = true;
                    node.unbindTextures.emplace_back(bind->stage, bind->slot);   // unbind conflicting texture

                    // 해결됨. 추적종료
                    inputTextures.erase(texh.value);
                    stageSlots.erase(key);
                }
            }

            // depth stencil 충돌 확인
            if (JAM_IS_VALID_HANDLE(nextPassFbInfo.depthStencilTexh))
            {
                const TextureHandle texh = nextPassFbInfo.depthStencilTexh;
                const auto          it   = inputTextures.find(texh.value);

                if (it != inputTextures.end())
                {
                    const TextureBind* bind = it->second;
                    const UInt32       key  = CreateStageSlotKey_(bind->stage, bind->slot);

                    node.bNeedUnbindResources = true;
                    node.unbindTextures.emplace_back(bind->stage, bind->slot);

                    // remove from tracking
                    inputTextures.erase(texh.value);
                    stageSlots.erase(key);
                }
            }

            // 모든 인풋 텍스처가 해결되었으면 종료
            if (inputTextures.empty())
            {
                break;
            }
        }

        // 현재 패스의 출력이 다음 입력으로 사용되는지
        // 만약, 충돌한다면 프레임버퍼 언바인딩 필요
        if (!JAM_IS_VALID_HANDLE(pass.m_fbh))
        {
            continue;
        }

        // 현재 패스의 출력 프레임버퍼 어태치먼트 수집
        const FrameBufferInfo fbInfo = Graphics::GetFrameBufferInfo(pass.m_fbh);
        for (Int32 k = 0; k < fbInfo.numRenderTargets; ++k)
        {
            attachments.emplace(fbInfo.renderTargetTexhs[k].value);
        }

        if (JAM_IS_VALID_HANDLE(fbInfo.depthStencilTexh))
        {
            attachments.emplace(fbInfo.depthStencilTexh.value);
        }

        // 다음 패스의 입력 텍스처 바인딩과 충돌 확인
        const Int32       nextIdx  = (i + 1) % numNodes;
        const RenderPass& nextPass = m_nodes[nextIdx].pass;
        for (const TextureBind& nextPassBind: nextPass.m_textureBinds)
        {
            // 충돌 발견
            if (attachments.contains(nextPassBind.texh.value))
            {
                node.bNeedUnbindResources   = true;
                node.bNeedUnbindFrameBuffer = true;
                break;   // 프레임버퍼는 하나이므로 하나만 발견해도 충분
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

    const Int32 lastIdx = NarrowCast<Int32>(m_nodes.size()) - 1;

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
