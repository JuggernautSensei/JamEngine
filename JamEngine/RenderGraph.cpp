#include "pch.h"

#include "Graphics.h"
#include "RenderGraph.h"

namespace jam
{

RenderGraph& RenderGraph::AddPass(RenderPass&& _pass)
{
    JAM_ASSERT(!m_bCompiled, "Cannot add pass to a compiled RenderGraph.");
    PassDesc& passDesc = m_passes.emplace_back();
    passDesc.pass      = std::move(_pass);
    return *this;
}

void RenderGraph::Compile()
{
    JAM_ASSERT(m_bCompiled == false, "RenderGraph has already been compiled.");

    const Int32 numPasses = NarrowCast<Int32>(m_passes.size());
    JAM_ASSERT(numPasses > 0, "RenderGraph must have at least one pass to compile.");

    // if only one pass, just validate and return
    if (numPasses == 1)
    {
        PassDesc& curPass = m_passes[0];
        JAM_ASSERT(curPass.pass.Validate(), "RenderPass at index 0 is not valid.");
    }
    else
    {
        AnalyzePassDependencies_();
        DeduplicateResourceBindings_();
    }

    m_bCompiled = true;
}

void RenderGraph::DeduplicateResourceBindings_()
{
    const Int32 numPasses = NarrowCast<Int32>(m_passes.size());
    if (numPasses <= 1)
    {
        return;
    }

    for (Int32 i = 0; i < numPasses; ++i)
    {
        RenderPass& curPass = m_passes[i].pass;

        // remove duplicate constant buffers across previous passes
        std::erase_if(curPass.m_cbufferBinds,
                      [this, i](const ConstantBufferBind& curItem)
                      {
                          for (Int32 j = i - 1; j >= 0; --j)
                          {
                              const RenderPass& prevPass = m_passes[j].pass;

                              for (const ConstantBufferBind& prevItem: prevPass.m_cbufferBinds)
                              {
                                  if (prevItem.stage == curItem.stage && prevItem.slot == curItem.slot)
                                  {
                                      // same resource in same slot -> remove
                                      // different resource in same slot -> keep
                                      return prevItem.handle == curItem.handle;
                                  }
                              }
                          }
                          return false;   // keep if no previous binding in same slot
                      });

        // remove duplicate samplers across previous passes
        std::erase_if(curPass.m_samplerBinds,
                      [this, i](const SamplerBind& curItem)
                      {
                          for (Int32 j = i - 1; j >= 0; --j)
                          {
                              const RenderPass& prevPass = m_passes[j].pass;

                              for (const SamplerBind& prevItem: prevPass.m_samplerBinds)
                              {
                                  if (prevItem.stage == curItem.stage && prevItem.slot == curItem.slot)
                                  {
                                      // same state in same slot -> remove
                                      // different state in same slot -> keep
                                      return MemEqual(prevItem.state, curItem.state);
                                  }
                              }
                          }
                          return false;   // keep if no previous binding in same slot
                      });
    }

    // execute virtual binding to remove redundant texture bindings

    HashMap<UInt32, UInt32> prevBound;
    for (Int32 i = 0; i < numPasses; ++i)
    {
        PassDesc&   passDesc = m_passes[i];
        RenderPass& curPass  = passDesc.pass;

        // local bound
        HashMap<UInt32, UInt32> localBound = prevBound;

        std::vector<TextureBind> newBinds;
        newBinds.reserve(curPass.m_textureBinds.size());

        for (const TextureBind& curItem: curPass.m_textureBinds)
        {
            const UInt32 key   = MakeStageSlotKey_(curItem.stage, curItem.slot);
            const UInt32 value = curItem.handle.value;

            auto it = localBound.find(key);
            if (it != localBound.end() && it->second == value)
            {
                // if the same resource is already bound in the same slot, skip it
                continue;
            }

            // add to new binds
            newBinds.push_back(curItem);
            localBound[key] = value;
        }

        // update texture binds
        curPass.m_textureBinds.swap(newBinds);

        // update previous bound
        prevBound.swap(localBound);

        // simulate unbinding resources
        if (passDesc.bNeedUnbindResources)
        {
            for (const BindInfo& info: passDesc.unbindTextures)
            {
                const UInt32 key = MakeStageSlotKey_(info.stage, info.slot);
                prevBound.erase(key);
            }
        }
    }
}

void RenderGraph::AnalyzePassDependencies_()
{
    const Int32 numPasses = NarrowCast<Int32>(m_passes.size());

    // temporary containers for tracking texture bindings and attachments
    HashMap<UInt32, const TextureBind*> handleMap;
    HashMap<UInt32, const TextureBind*> stageSlotMap;
    HashSet<UInt32>                     attachmentsSet;

    for (Int32 i = 0; i < numPasses; ++i)
    {
        handleMap.clear();
        stageSlotMap.clear();
        attachmentsSet.clear();

        PassDesc&         curPassDesc = m_passes[i];
        const RenderPass& curPass     = m_passes[i].pass;
        JAM_ASSERT(curPass.Validate(), "RenderPass at index {} is not valid.", i);

        // build maps of all texture inputs for the current pass
        for (const TextureBind& bind: curPass.m_textureBinds)
        {
            JAM_ASSERT(JAM_IS_VALID_HANDLE(bind.handle), "RenderPass at index {} has invalid texture handle in its inputs.", i);
            handleMap.emplace(bind.handle.value, &bind);
            stageSlotMap.emplace(MakeStageSlotKey_(bind.stage, bind.slot), &bind);
        }

        // check all subsequent passes (wrapping around) for conflicts
        for (Int32 offset = 1; offset < numPasses; ++offset)
        {
            const Int32       j        = (i + offset) % numPasses;
            const RenderPass& nextPass = m_passes[j].pass;

            // remove bindings that are overwritten by next pass in same slot
            for (const TextureBind& texBind: nextPass.m_textureBinds)
            {
                JAM_ASSERT(JAM_IS_VALID_HANDLE(texBind.handle), "RenderPass at index {} has invalid texture handle in its inputs.", j);
                auto it = stageSlotMap.find(MakeStageSlotKey_(texBind.stage, texBind.slot));
                if ((it != stageSlotMap.end()) && (it->second->handle != texBind.handle))
                {
                    // binding is overwritten in next pass, so no need to track it anymore
                    const TextureBind* curInput = it->second;
                    handleMap.erase(curInput->handle.value);
                    stageSlotMap.erase(it);
                }
            }

            // no more inputs to check
            if (handleMap.empty())
            {
                break;
            }

            // skip if next pass has no framebuffer
            if (!JAM_IS_VALID_HANDLE(nextPass.m_fbh))
            {
                continue;
            }

            // check if any current pass inputs are used as render targets in next pass
            const FrameBufferInfo nextFbInfo = Graphics::GetFrameBufferInfo(nextPass.m_fbh);
            for (Int32 k = 0; k < nextFbInfo.numRenderTargets; ++k)
            {
                TextureHandle renderTargetTexh = nextFbInfo.renderTargetTexhs[k];
                if (handleMap.contains(renderTargetTexh.value))
                {
                    // mark texture for unbinding before next pass
                    const TextureBind* texInput      = handleMap.at(renderTargetTexh.value);
                    curPassDesc.bNeedUnbindResources = true;
                    curPassDesc.unbindTextures.emplace_back(texInput->stage, texInput->slot);

                    stageSlotMap.erase(MakeStageSlotKey_(texInput->stage, texInput->slot));
                    handleMap.erase(renderTargetTexh.value);
                }
            }

            // check depth stencil texture conflict
            TextureHandle dsTexHandle = nextFbInfo.depthStencilTexh;
            if (JAM_IS_VALID_HANDLE(dsTexHandle) && handleMap.contains(dsTexHandle.value))
            {
                // mark depth stencil texture for unbinding before next pass
                const TextureBind* texInput      = handleMap.at(dsTexHandle.value);
                curPassDesc.bNeedUnbindResources = true;
                curPassDesc.unbindTextures.emplace_back(texInput->stage, texInput->slot);

                stageSlotMap.erase(MakeStageSlotKey_(texInput->stage, texInput->slot));
                handleMap.erase(dsTexHandle.value);
            }

            // no more inputs to check
            if (handleMap.empty())
            {
                break;
            }
        }

        if (!JAM_IS_VALID_HANDLE(curPass.m_fbh))
        {
            continue;
        }

        // collect all attachment textures from current pass framebuffer
        FrameBufferInfo curFbhInfo = Graphics::GetFrameBufferInfo(curPass.m_fbh);
        for (Int32 k = 0; k < curFbhInfo.numRenderTargets; ++k)
        {
            TextureHandle renderTargetTexh = curFbhInfo.renderTargetTexhs[k];
            attachmentsSet.emplace(renderTargetTexh.value);
        }

        if (JAM_IS_VALID_HANDLE(curFbhInfo.depthStencilTexh))
        {
            attachmentsSet.emplace(curFbhInfo.depthStencilTexh.value);
        }

        // check if next pass uses any of current pass's attachments as input
        const Int32       nextIndex = (i + 1) % numPasses;
        const RenderPass& nextPass  = m_passes[nextIndex].pass;
        for (const TextureBind& nextInputTex: nextPass.m_textureBinds)
        {
            if (attachmentsSet.contains(nextInputTex.handle.value))
            {
                // need to unbind framebuffer before next pass can use its textures
                curPassDesc.bNeedUnbindResources   = true;
                curPassDesc.bNeedUnbindFrameBuffer = true;
                break;
            }
        }
    }
}

void RenderGraph::Execute() const
{
    JAM_ASSERT(m_bCompiled, "RenderGraph must be compiled before execution.");

    for (const PassDesc& passEx: m_passes)
    {
        passEx.pass.Execute();

        if (passEx.bNeedUnbindResources)
        {
            UnbindPassResources_(passEx);
        }
    }
}

void RenderGraph::Execute(const FrameBufferHandle _fbh, const Viewport _viewport) const
{
    JAM_ASSERT(m_bCompiled, "RenderGraph must be compiled before execution.");

    const Int32 numPasses = NarrowCast<Int32>(m_passes.size());
    const Int32 lastIdx   = numPasses - 1;

    for (Int32 idx = 0; idx < numPasses; ++idx)
    {
        const PassDesc& desc = m_passes[idx];

        if (idx == lastIdx)
        {
            desc.pass.Execute(_fbh, _viewport);
        }
        else
        {
            desc.pass.Execute();
        }

        if (desc.bNeedUnbindResources)
        {
            UnbindPassResources_(desc);
        }
    }
}

void RenderGraph::UnbindPassResources_(const PassDesc& _passDesc) const
{
    if (_passDesc.bNeedUnbindFrameBuffer)
    {
        Graphics::SetFrameBuffer(k_invalidHandle);
    }

    for (const BindInfo& texInfo: _passDesc.unbindTextures)
    {
        Graphics::SetTexture(k_invalidHandle, texInfo.stage, texInfo.slot);
    }

    Graphics::Flush();
}

UInt32 RenderGraph::MakeStageSlotKey_(const eShader _stage, const Int32 _slot) const
{
    return (static_cast<UInt32>(_stage) << 16) | static_cast<UInt32>(_slot);
}

void RenderGraph::Reset()
{
    m_passes.clear();
    m_bCompiled = false;
}

bool RenderGraph::IsCompiled() const
{
    return m_bCompiled;
}

}   // namespace jam
