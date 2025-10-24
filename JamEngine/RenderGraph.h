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

private:
    std::vector<PassEx> m_passes    = {};
    bool                m_bCompiled = false;
};
}   // namespace jam
