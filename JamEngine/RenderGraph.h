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

    struct Node
    {
        RenderPass pass = {};

        Vector<BindInfo> unbindTextures         = {};
        bool             bNeedUnbindResources   = false;
        bool             bNeedUnbindFrameBuffer = false;
    };

public:
    RenderGraph()  = default;
    ~RenderGraph() = default;

    RenderGraph(const RenderGraph&)            = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;
    RenderGraph(RenderGraph&&)                 = default;
    RenderGraph& operator=(RenderGraph&&)      = default;

    RenderGraph&       AddPass(RenderPass&& _pass);
    void               Compile();
    void               Execute() const;
    void               Execute(FrameBufferHandle _fbh, Viewport _viewport) const;
    void               Reset();
    [[nodiscard]] bool IsCompiled() const;

private:
    void                 DeduplicateResourceBindings_();
    void                 AnalyzePassDependencies_();
    void                 UnbindPassResourcesIfNeed_(const Node& _node) const;
    [[nodiscard]] UInt32 CreateStageSlotKey_(eShader _stage, Int32 _slot) const;

    Vector<Node> m_nodes     = {};
    bool         m_bCompiled = false;
};

}   // namespace jam
