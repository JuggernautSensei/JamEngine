#include "pch.h"
#include "PostProcessManager.h"
#include "RenderStates.h"

#define SHADER_PATH(_filename) "Resources/ImageFilter/" _filename

PostProcessManager& PostProcessManager::GetInstance()
{
    JAM_ASSERT(s_pInstance, "PostProcessManager instance does not exist. Call CreateInstance first.");
    return *s_pInstance;
}

void PostProcessManager::CreateInstance()
{
    JAM_ASSERT(s_pInstance == nullptr, "PostProcessManager instance already exists.");
    s_pInstance = new PostProcessManager();
}

void PostProcessManager::DestroyInstance()
{
    if (s_pInstance)
    {
        delete s_pInstance;
        s_pInstance = nullptr;
    }
}

PostProcessManager::RenderPassBuilder PostProcessManager::AddBlitFilter(
    RenderGraph&           _graph,
    const StringView       _name,
    const ImageFilterDesc& _desc)
{
    JAM_ASSERT(!_desc.input0TextureName.IsEmpty(), "Input0 texture name cannot be empty for Blit filter.");

    constexpr StringView kFilterName = "__BLIT__";
    _graph.InsertProgram(kFilterName, GetOrLoadProgram_(eImageFilter::Blit), false);

    RenderPassBuilder builder = _graph.AddRenderPass(_name)
                                    .SetProgram(kFilterName)
                                    .SetTexture(_desc.input0TextureName, eShader::PixelShader, kTInput0Slot)
                                    .SetSampler(kLinearClamp, eShader::PixelShader, kSLinearClampSlot)
                                    .SetBlendFlags(kOpaqueBlend, 0)
                                    .SetRenderFlags(eRenderFlags::None)
                                    .SetExecutor(Bind<&PostProcessManager::SubmitScreenTriangle>(this));

    PostProcess_(_graph, builder, _name, _desc);
    return builder;
}

PostProcessManager::RenderPassBuilder PostProcessManager::AddMixFilter(
    RenderGraph&           _graph,
    const StringView       _name,
    const ImageFilterDesc& _desc)
{
    JAM_ASSERT(!_desc.input0TextureName.IsEmpty(), "Input0 texture name cannot be empty for Mix filter.");
    JAM_ASSERT(!_desc.input1TextureName.IsEmpty(), "Input1 texture name cannot be empty for Mix filter.");

    constexpr StringView kFilterName = "__MIX__";

    _graph.InsertProgram(kFilterName, GetOrLoadProgram_(eImageFilter::Mix), false);
    _graph.EmplaceConstantBuffer(_name, sizeof(CBParam));

    RenderPassBuilder builder = _graph.AddRenderPass(_name)
                                    .SetProgram(kFilterName)
                                    .SetSampler(kLinearClamp, eShader::PixelShader, kSLinearClampSlot)
                                    .SetTexture(_desc.input1TextureName, eShader::PixelShader, kTInput1Slot)
                                    .SetBlendFlags(kOpaqueBlend, 0)
                                    .SetRenderFlags(eRenderFlags::None)
                                    .SetExecutor(Bind<&PostProcessManager::SubmitScreenTriangle>(this));

    PostProcess_(_graph, builder, _name, _desc);
    return builder;
}

PostProcessManager::RenderPassBuilder PostProcessManager::AddConvolutionFilter(
    RenderGraph&           _graph,
    const StringView       _name,
    const ImageFilterDesc& _desc)
{
    constexpr StringView kFilterNameV = "__CONVOLUTION_V__";
    constexpr StringView kFilterNameH = "__CONVOLUTION_H__";

    // common
    _graph.EmplaceConstantBuffer(_name, sizeof(CBKernel));

    // vertical
    _graph.InsertProgram(kFilterNameV, GetOrLoadProgram_(eImageFilter::Convolution1D_V), false);
    const String vname = String::Formatted("{} (V)", _name);

    RenderPassBuilder vbuilder = _graph.AddRenderPass(vname)
                                     .SetProgram(kFilterNameV)
                                     .SetSampler(kLinearClamp, eShader::PixelShader, kSLinearClampSlot)
                                     .SetBlendFlags(kOpaqueBlend, 0)
                                     .SetRenderFlags(eRenderFlags::None)
                                     .SetExecutor(Bind<&PostProcessManager::SubmitScreenTriangle>(this));

    ImageFilterDesc vdesc = _desc;
    vdesc.bFinalPass      = false;
    PostProcess_(_graph, vbuilder, vname, vdesc);

    // horizontal
    _graph.InsertProgram(kFilterNameH, GetOrLoadProgram_(eImageFilter::Convolution1D_H), false);

    RenderPassBuilder hbuilder = _graph.AddRenderPass(_name)
                                     .SetFrameBuffer(_name)
                                     .SetProgram(kFilterNameH)
                                     .SetSampler(kLinearClamp, eShader::PixelShader, kSLinearClampSlot)
                                     .SetTexture(vname, eShader::PixelShader, kTInput0Slot)
                                     .SetBlendFlags(kOpaqueBlend, 0)
                                     .SetRenderFlags(eRenderFlags::None)
                                     .SetExecutor(Bind<&PostProcessManager::SubmitScreenTriangle>(this));

    ImageFilterDesc hdesc   = _desc;
    hdesc.input0TextureName = vname;
    PostProcess_(_graph, hbuilder, _name, hdesc);
    return hbuilder;
}

void PostProcessManager::UpdateParam(
    const RenderGraph& _graph,
    const StringView   _name,
    const Vec4         _param) const
{
    Graphics&                  gfx     = GetGraphics();
    const ConstantBufferHandle cbuffer = _graph.GetConstantBuffer(_name);

    CBParam cb = {};
    cb.param   = _param;
    gfx.UpdateBuffer(cbuffer, AsMemoryView(cb));
}

void PostProcessManager::UpdateKernel(
    const RenderGraph&      _graph,
    const StringView        _name,
    const Span<const float> _weights) const
{
    Graphics&                  gfx     = GetGraphics();
    const ConstantBufferHandle cbuffer = _graph.GetConstantBuffer(_name);

    JAM_ASSERT(_weights.size() <= 16, "Kernel size cannot exceed 16.");
    CBKernel cb  = {};
    cb.kernelLen = static_cast<Int32>(_weights.size());
    for (Int32 i = 0; i < cb.kernelLen; ++i)
    {
        cb.weights[i] = _weights[i];
    }
    gfx.UpdateBuffer(cbuffer, AsMemoryView(cb));
}

void PostProcessManager::SubmitScreenTriangle() const
{
    m_pScreenTriangle->Submit();
}

PostProcessManager::PostProcessManager()
{
    constexpr Vec2 kVerts[] = {
        { -1.f,  1.f },
        {  3.f,  1.f },
        { -1.f, -3.f },
    };
    constexpr UInt16 kIndices[] = { 0, 1, 2 };
    m_pScreenTriangle           = Mesh::Load(kVerts, kIndices);

    m_programCompileInfos[eImageFilter::Blit].path            = SHADER_PATH("BlitShader.hlsl");
    m_programCompileInfos[eImageFilter::Mix].path             = SHADER_PATH("MixShader.hlsl");
    m_programCompileInfos[eImageFilter::Convolution1D_V].path = SHADER_PATH("ConvolutionShader.hlsl");

    m_programCompileInfos[eImageFilter::Convolution1D_H].path   = SHADER_PATH("ConvolutionShader.hlsl");
    m_programCompileInfos[eImageFilter::Convolution1D_H].macros = {
        { "HORIZONTAL", "" }
    };
}

PostProcessManager::~PostProcessManager()
{
    for (ProgramHandle ph: m_programs)
    {
        JAM_DESTROY_PROGRAM(ph);
    }
}

ProgramHandle PostProcessManager::GetOrLoadProgram_(
    const eImageFilter _filter)
{
    ProgramHandle ph = m_programs[_filter];
    if (ph)
    {
        return ph;
    }

    constexpr ShaderLoadInfo kVscInfo = { eShader::VertexShader, "VSMain", {}, eShaderCompileFlags::Default };

    const ProgramCompileInfo& pci     = m_programCompileInfos[_filter];
    const ShaderLoadInfo      pscInfo = { eShader::PixelShader, "PSMain", pci.macros, eShaderCompileFlags::Default };

    const ShaderCompiler vsc = ShaderCompiler::CompileHLSLFile(pci.path, kVscInfo);
    const ShaderCompiler psc = ShaderCompiler::CompileHLSLFile(pci.path, pscInfo);
    JAM_ASSERT(vsc.IsSucceeded(), "Vertex shader compilation failed: {}", vsc.GetErrorMessage());
    JAM_ASSERT(psc.IsSucceeded(), "Pixel shader compilation failed: {}", psc.GetErrorMessage());

    Graphics&          gfx = GetGraphics();
    const ShaderHandle vsh = gfx.CreateShader(vsc.GetBytecode());
    const ShaderHandle psh = gfx.CreateShader(psc.GetBytecode());
    m_programs[_filter]    = gfx.CreateProgram(vsh, psh, true);
    return m_programs[_filter];
}

void PostProcessManager::PostProcess_(
    RenderGraph&           _graph,
    RenderPassBuilder&     _builder,
    const StringView       _name,
    const ImageFilterDesc& _desc) const
{
    if (_desc.bFinalPass)
    {
        _builder.AsFinalPass();
    }
    else
    {
        _graph.EmplaceTexture2D(_name, _desc.scale, _desc.format, 1, eTextureCreateFlags::RenderTargetAttachment);
        _graph.EmplaceFrameBuffer(_name, _name);
        _builder.SetFrameBuffer(_name);
    }
}