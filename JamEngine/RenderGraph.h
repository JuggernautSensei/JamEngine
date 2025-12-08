// RenderGraph.h
#pragma once
#include "RenderPass.h"

namespace jam
{

class RenderGraph
{
    struct BindInfo
    {
        eShader stage;
        Int32   slot;
    };

    struct PassDesc
    {
        RenderPass            pass                   = {};
        bool                  bNeedUnbindResources   = false;
        bool                  bNeedUnbindFrameBuffer = false;
        std::vector<BindInfo> unbindTextures         = {};
    };

public:
    RenderGraph()  = default;
    ~RenderGraph() = default;

    RenderGraph(const RenderGraph&)            = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;
    RenderGraph(RenderGraph&&)                 = default;
    RenderGraph& operator=(RenderGraph&&)      = default;

    RenderGraph& AddPass(RenderPass&& _pass);

    void               Compile();
    void               Execute() const;
    void               Execute(FrameBufferHandle _fbh, Viewport _viewport) const;
    void               Reset();
    [[nodiscard]] bool IsCompiled() const;

private:
    void DeduplicateResourceBindings_();
    void AnalyzePassDependencies_();
    void UnbindPassResources_(const PassDesc& _passDesc) const;

    [[nodiscard]] UInt32 MakeStageSlotKey_(eShader _stage, Int32 _slot) const;

    std::vector<PassDesc> m_passes    = {};
    bool                  m_bCompiled = false;
};

}   // namespace jam
