#pragma once
#include "../JamEngine/RenderGraph.h"

struct ImageFilterDesc
{
    eTextureFormat format     = eTextureFormat::Unknown;
    float          scale      = 1.f;
    bool           bFinalPass = false;

    String input0TextureName = {};
    String input1TextureName = {};
};

class PostProcessManager
{
    enum class eImageFilter : UInt32
    {
        Blit,
        Mix,
        Convolution1D_V,
        Convolution1D_H,
    };

    struct CBParam
    {
        Vec4 param = Vec4::kZero;
    };

    struct CBKernel
    {
        float weights[16] = {};
        Int32 kernelLen   = 0;
        float padding[3]  = {};
    };

    constexpr static Int32 kTInput0Slot      = 0;
    constexpr static Int32 kTInput1Slot      = 1;
    constexpr static Int32 kSLinearClampSlot = 0;
    constexpr static Int32 kSLinearWrapSlot  = 1;

    struct ProgramCompileInfo
    {
        String              path   = {};
        Vector<ShaderMacro> macros = {};
    };

public:
    using RenderPassBuilder = RenderGraph::RenderPassBuilder;

    PostProcessManager(const PostProcessManager&)                = delete;
    PostProcessManager& operator=(const PostProcessManager&)     = delete;
    PostProcessManager(PostProcessManager&&) noexcept            = delete;
    PostProcessManager& operator=(PostProcessManager&&) noexcept = delete;

    [[nodiscard]] static PostProcessManager& GetInstance();
    static void                              CreateInstance();
    static void                              DestroyInstance();

    RenderPassBuilder AddBlitFilter(RenderGraph& _graph, StringView _name, const ImageFilterDesc& _desc);
    RenderPassBuilder AddMixFilter(RenderGraph& _graph, StringView _name, const ImageFilterDesc& _desc);
    RenderPassBuilder AddConvolutionFilter(RenderGraph& _graph, StringView _name, const ImageFilterDesc& _desc);

    void UpdateParam(const RenderGraph& _graph, StringView _name, Vec4 _param) const;
    void UpdateKernel(const RenderGraph& _graph, StringView _name, Span<const float> _weights) const;

    void SubmitScreenTriangle() const;

private:
    PostProcessManager();
    ~PostProcessManager();

    void                        PostProcess_(RenderGraph& _graph, RenderPassBuilder& _builder, StringView _name, const ImageFilterDesc& _desc) const;
    [[nodiscard]] ProgramHandle GetOrLoadProgram_(eImageFilter _filter);

    Ref<Mesh>                                   m_pScreenTriangle     = nullptr;
    EnumArray<eImageFilter, ProgramHandle>      m_programs            = {};
    EnumArray<eImageFilter, ProgramCompileInfo> m_programCompileInfos = {};

    inline static PostProcessManager* s_pInstance = nullptr;
};

[[nodiscard]] inline PostProcessManager& GetPostProcessManager()
{
    return PostProcessManager::GetInstance();
}