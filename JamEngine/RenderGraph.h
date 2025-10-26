#pragma once
#include "RenderPass.h"

namespace jam
{

class RenderGraph
{
    struct TextureBindInfo
    {
        eShader stage;
        Int32   slot;
    };

    struct PassEx
    {
        RenderPass                   pass                   = {};
        bool                         bNeedUnbind            = false;
        bool                         bNeedUnbindFrameBuffer = false;
        std::vector<TextureBindInfo> unbindTextures         = {};
    };

public:
    RenderGraph& AddPass(const RenderPass& _pass);

    void Compile();
    void Execute() const;
    void Execute(FrameBufferHandle _fbh, Viewport _viewport) const;   // 원하는 프레임버퍼와 뷰포트로 오버라이드하여 실행

private:
    std::vector<PassEx> m_passes    = {};
    bool                m_bCompiled = false;
};
}   // namespace jam
