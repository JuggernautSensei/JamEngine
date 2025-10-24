#include "pch.h"

#include "RenderGraph.h"

#include "Graphics.h"

namespace jam
{

JAM_NODISCARD UInt64 MakeSlotKey(const eShader _stage, const Int32 _slot)
{
    return (static_cast<UInt64>(_stage) << 32) | static_cast<UInt64>(_slot);
}

}   // namespace jam

namespace jam
{

RenderGraph& RenderGraph::AddPass(const RenderPass& _pass)
{
    JAM_ASSERT(!m_bCompiled, "Cannot add pass to a compiled RenderGraph.");

    m_passes.emplace_back(_pass);
    return *this;
}

void RenderGraph::Compile()
{
    JAM_ASSERT(m_bCompiled == false, "RenderGraph has already been compiled.");

    Int32 passCount = NarrowCast<Int32>(m_passes.size());
    JAM_ASSERT(passCount > 0, "RenderGraph must have at least one pass to compile.");

    // 단일 패스인 경우 의존성 검사가 필요 없음
    if (passCount == 1)
    {
        PassEx& curPass = m_passes[0];
        JAM_ASSERT(curPass.pass.Validate(), "RenderPass at index 0 is not valid.");
        m_bCompiled = true;
        return;
    }

    // 다중 패스인 경우 의존성 검사 수행

    HashMap<UInt32, const TextureBindItem*> handleBasedMap;      // 핸들 기반 맵
    HashMap<UInt64, const TextureBindItem*> slotBasedMap;        // 슬롯 기반 맵
    HashSet<UInt32>                         frameBufferTexSet;   // 프레임 버퍼 텍스처 핸들 집합

    for (Int32 i = 0; i < passCount; ++i)
    {
        handleBasedMap.clear();
        slotBasedMap.clear();
        frameBufferTexSet.clear();

        PassEx&           curPassEx = m_passes[i];
        const RenderPass& curPass   = curPassEx.pass;
        JAM_ASSERT(curPass.Validate(), "RenderPass at index {} is not valid.", i);   // 패스 유효성 검사

        // 현재 패스의 입력 리소스 수집
        const std::vector<TextureBindItem>& curInputs = curPass.GetTextureBindList();
        for (const TextureBindItem& inputTex: curInputs)
        {
            handleBasedMap.emplace(inputTex.handle.value, &inputTex);
            slotBasedMap.emplace(MakeSlotKey(inputTex.stage, inputTex.slot), &inputTex);
        }

        // 현재 패스의 입력이 이후 패스의 출력과 충돌하는지 검사
        for (Int32 offset = 1; offset < passCount; ++offset)
        {
            const Int32       j        = (i + offset) % passCount;
            const RenderPass& nextPass = m_passes[j].pass;

            // 같은 슬롯 + 다른 리소스인 경우 충돌 문제 해소
            for (const TextureBindItem& nextInputTex: nextPass.GetTextureBindList())
            {
                auto it = slotBasedMap.find(MakeSlotKey(nextInputTex.stage, nextInputTex.slot));
                if ((it != slotBasedMap.end()) && (it->second->handle != nextInputTex.handle))
                {
                    const TextureBindItem* curInput = it->second;
                    handleBasedMap.erase(curInput->handle.value);   // 핸들 기반 맵에서도 제거 (충돌 가능성이 없어짐)
                    slotBasedMap.erase(it);                         // 슬롯 기반 맵에서 제거
                }
            }

            // 더 이상 검사할 핸들이 없으면 조기 종료
            if (handleBasedMap.empty())
                break;

            // 다음 패스의 출력 리소스와 충돌 검사
            FrameBufferHandle nextFbh    = nextPass.GetFrameBuffer();
            FrameBufferInfo   nextFbInfo = Graphics::GetFrameBufferInfo(nextFbh);

            // 렌더 타겟 검사
            for (Int32 k = 0; k < nextFbInfo.renderTargetCount; ++k)
            {
                TextureHandle rtTexHandle = nextFbInfo.renderTargetTextureHandles[k];
                if (handleBasedMap.contains(rtTexHandle.value))   // 충돌 발견
                {
                    const TextureBindItem* texInput = handleBasedMap.at(rtTexHandle.value);
                    curPassEx.bNeedUnbind           = true;
                    curPassEx.unbindTextures.emplace_back(texInput->stage, texInput->slot);

                    // 맵에서 제거
                    slotBasedMap.erase(MakeSlotKey(texInput->stage, texInput->slot));
                    handleBasedMap.erase(rtTexHandle.value);
                }
            }

            // 깊이 스텐실 버퍼 검사
            TextureHandle dsTexHandle = nextFbInfo.depthStencilTextureHandle;
            if (JAM_IS_VALID_HANDLE(dsTexHandle) && handleBasedMap.contains(dsTexHandle.value))
            {
                const TextureBindItem* texInput = handleBasedMap.at(dsTexHandle.value);
                curPassEx.bNeedUnbind           = true;
                curPassEx.unbindTextures.emplace_back(texInput->stage, texInput->slot);

                // 맵에서 제거
                slotBasedMap.erase(MakeSlotKey(texInput->stage, texInput->slot));
                handleBasedMap.erase(dsTexHandle.value);
            }

            // 더 이상 검사할 핸들이 없으면 조기 종료
            if (handleBasedMap.empty())
                break;
        }

        // 현재 패스의 출력이 다음 패스의 입력인지 검사
        FrameBufferHandle curFbh     = curPass.GetFrameBuffer();
        FrameBufferInfo   curFbhInfo = Graphics::GetFrameBufferInfo(curFbh);

        // 현재 패스의 모든 출력 리소스 수집
        for (Int32 k = 0; k < curFbhInfo.renderTargetCount; ++k)
        {
            TextureHandle rtTexHandle = curFbhInfo.renderTargetTextureHandles[k];
            frameBufferTexSet.emplace(rtTexHandle.value);
        }

        if (JAM_IS_VALID_HANDLE(curFbhInfo.depthStencilTextureHandle))
        {
            frameBufferTexSet.emplace(curFbhInfo.depthStencilTextureHandle.value);
        }

        // 다음 패스의 입력 리소스와 충돌 검사
        const Int32       nextIndex = (i + 1) % passCount;
        const RenderPass& nextPass  = m_passes[nextIndex].pass;
        for (const TextureBindItem& nextInputTex: nextPass.GetTextureBindList())
        {
            if (frameBufferTexSet.contains(nextInputTex.handle.value))
            {
                curPassEx.bNeedUnbind            = true;
                curPassEx.bNeedUnbindFrameBuffer = true;
                break;   // 프레임버퍼 언바인딩만 하면 되므로 더 이상 검사할 필요 없음
            }
        }
    }

    m_bCompiled = true;
}

void RenderGraph::Execute() const
{
    JAM_ASSERT(m_bCompiled, "RenderGraph must be compiled before execution.");

    for (const PassEx& passEx: m_passes)
    {
        // 각 패스를 실행
        passEx.pass.Execute();

        // 필요시 리소스를 언바인딩
        if (passEx.bNeedUnbind)
        {
            // 프레임버퍼 언바인딩
            if (passEx.bNeedUnbindFrameBuffer)
            {
                Graphics::SetFrameBuffer(k_invalidHandle);
            }

            // 바인딩된 텍스처 언바인딩
            for (const TextureBindInfo& texInfo: passEx.unbindTextures)
            {
                Graphics::SetTexture(k_invalidHandle, texInfo.stage, texInfo.slot);
            }
        }
    }
}

}   // namespace jam