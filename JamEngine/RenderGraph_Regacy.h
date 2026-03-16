// RenderGraph_Regacy.h
#pragma once
#include "RenderPass_Regacy.h"

namespace Jam::Regacy
{

class RenderGraph_Regacy
{
    using RenderPass = RenderPass_Regacy;

    struct BindInfo
    {
        eShader stage = {};
        Int32   slot  = {};
    };

    struct Node
    {
        RenderPass_Regacy pass = {};

        Vector<BindInfo> unbindTextures         = {};
        bool             bNeedUnbindResources   = false;
        bool             bNeedUnbindFrameBuffer = false;
    };

public:
    RenderGraph_Regacy()  = default;
    ~RenderGraph_Regacy() = default;

    RenderGraph_Regacy(const RenderGraph_Regacy&)            = delete;
    RenderGraph_Regacy& operator=(const RenderGraph_Regacy&) = delete;
    RenderGraph_Regacy(RenderGraph_Regacy&&)                 = default;
    RenderGraph_Regacy& operator=(RenderGraph_Regacy&&)      = default;

    RenderGraph_Regacy&       AddPass(RenderPass_Regacy&& _pass);
    void               Compile();
    void               Execute() const;
    void               Execute(FrameBufferHandle _fbh, Viewport _viewport) const;
    void               Reset();
    [[nodiscard]] bool IsCompiled() const;

private:
    void                 DeduplicateResourceBindings_();
    void                 AnalyzePassDependencies_();
    void                 UnbindPassResourcesIfNeed_(const Node& _node) const;
    [[nodiscard]] UInt64 MakeStageSlotKey_(eShader _stage, Int32 _slot) const;

    Vector<Node> m_nodes     = {};
    bool         m_bCompiled = false;
};

}   // namespace Jam
