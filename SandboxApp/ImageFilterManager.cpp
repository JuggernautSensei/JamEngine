#include "pch.h"
#include "ImageFilterManager.h"
#include "RenderStates.h"

#define SCENE_PATH(_filename) "Resources/ImageFilter/" _filename

ImageFilterManager& ImageFilterManager::GetInstance()
{
    JAM_ASSERT(s_pInstance, "PostProcessManager instance does not exist. Call CreateInstance first.");
    return *s_pInstance;
}

void ImageFilterManager::CreateInstance()
{
    JAM_ASSERT(s_pInstance == nullptr, "PostProcessManager instance already exists.");
    s_pInstance = new ImageFilterManager();
}

void ImageFilterManager::DestroyInstance()
{
    if (s_pInstance)
    {
        delete s_pInstance;
        s_pInstance = nullptr;
    }
}

void ImageFilterManager::UpdateParams(
    const RenderGraph&        _graph,
    const StringView          _passName,
    const CBImageFilterParam& _param) const
{
    Graphics&                  gfx = Graphics::GetInstance();
    const ConstantBufferHandle cbh = _graph.GetConstantBuffer(MakeParamBufferName_(_passName));
    gfx.UpdateBuffer(cbh, AsMemoryView(_param));
}

void ImageFilterManager::UpdateKernel(
    const RenderGraph&         _graph,
    const StringView           _passName,
    const CBImageFilterKernel& _kernel) const
{
    Graphics&                  gfx = Graphics::GetInstance();
    const ConstantBufferHandle cbh = _graph.GetConstantBuffer(MakeKernelBufferName_(_passName));
    gfx.UpdateBuffer(cbh, AsMemoryView(_kernel));
}

void ImageFilterManager::SubmitScreenTriangle() const
{
    m_pScreenTriangle->Submit();
}

ImageFilterManager::ImageFilterManager()
{
    // screen triangle
    constexpr Vec2 kVerts[] = {
        { -1.f,  1.f },
        {  3.f,  1.f },
        { -1.f, -3.f },
    };
    constexpr UInt16 kIndices[] = { 0, 1, 2 };
    m_pScreenTriangle           = Mesh::Load(kVerts, kIndices);

    m_imageFilterSpecs[eImageFilter::Blit].programName = "__BLIT__";
    m_imageFilterSpecs[eImageFilter::Blit].path        = SCENE_PATH("BlitShader.hlsl");

    m_imageFilterSpecs[eImageFilter::Mix].programName = "__MIX__";
    m_imageFilterSpecs[eImageFilter::Mix].path        = SCENE_PATH("MixShader.hlsl");
    m_imageFilterSpecs[eImageFilter::Mix].bNeedParam  = true;
    m_imageFilterSpecs[eImageFilter::Mix].bNeedInput1 = true;

    m_imageFilterSpecs[eImageFilter::Mult].programName = "__MULT__";
    m_imageFilterSpecs[eImageFilter::Mult].path        = SCENE_PATH("MultiplyShader.hlsl");
    m_imageFilterSpecs[eImageFilter::Mult].bNeedInput1 = true;

    m_imageFilterSpecs[eImageFilter::Convolution1DY].programName = "__CONVOLUTION1DY__";
    m_imageFilterSpecs[eImageFilter::Convolution1DY].path        = SCENE_PATH("ConvolutionShader.hlsl");
    m_imageFilterSpecs[eImageFilter::Convolution1DY].bNeedKernel = true;

    m_imageFilterSpecs[eImageFilter::Convolution1DX].programName = "__CONVOLUTION1DX__";
    m_imageFilterSpecs[eImageFilter::Convolution1DX].path        = SCENE_PATH("ConvolutionShader.hlsl");
    m_imageFilterSpecs[eImageFilter::Convolution1DX].macros.emplace_back("HORIZONTAL", "");
    m_imageFilterSpecs[eImageFilter::Convolution1DX].bNeedKernel = true;

    m_imageFilterSpecs[eImageFilter::ToneMapping].programName = "__TONE_MAPPING__";
    m_imageFilterSpecs[eImageFilter::ToneMapping].path        = SCENE_PATH("ToneMappingShader.hlsl");
    m_imageFilterSpecs[eImageFilter::ToneMapping].bNeedParam  = true;

    m_imageFilterSpecs[eImageFilter::FXAA].programName = "__FXAA__";
    m_imageFilterSpecs[eImageFilter::FXAA].path        = SCENE_PATH("FXAAShader.hlsl");
}

ImageFilterManager::~ImageFilterManager()
{
    for (ProgramHandle ph: m_programs[0])
    {
        JAM_DESTROY_PROGRAM(ph);
    }

    for (ProgramHandle ph: m_programs[1])
    {
        JAM_DESTROY_PROGRAM(ph);
    }
}

String ImageFilterManager::MakeParamBufferName_(String _passName) const
{
    return String::Formatted("{}_CB_Param", _passName);
}

String ImageFilterManager::MakeKernelBufferName_(String _passName) const
{
    return String::Formatted("{}_CB_Kernel", _passName);
}

RenderGraph::RenderPassBuilder ImageFilterManager::AddImageFilter(
    RenderGraph&           _graph,
    const StringView       _name,
    const ImageFilterInfo& _desc)
{
    JAM_ASSERT(!_desc.input0Texture.IsEmpty(), "Input0 texture name cannot be empty for Tone Mapping filter.");

    const ImageFilterSpec& spec = m_imageFilterSpecs[_desc.filter];
    _graph.InsertProgram(spec.programName, GetOrLoadProgram_(_desc.filter, _desc.bUseTexture2DArrayInput0, _desc.bUseTexture2DArrayInput1), false);

    // add pass
    RenderGraph::RenderPassBuilder builder =
        _graph.AddRenderPass(_name)
            .SetProgram(spec.programName)
            .SetTexture(_desc.input0Texture, eShader::PixelShader, kTInput0Slot);

    // constants
    if (spec.bNeedParam || _desc.bUseTexture2DArrayInput0 || _desc.bUseTexture2DArrayInput1)
    {
        if (_desc.paramCBufferOrEmpty.IsEmpty())
        {
            const String name = MakeParamBufferName_(_name);
            _graph.EmplaceConstantBuffer(name, sizeof(CBImageFilterParam));
            builder.SetConstantBuffer(name, eShader::PixelShader, CBImageFilterParam::kSlot);
        }
        else
        {
            builder.SetConstantBuffer(_desc.paramCBufferOrEmpty, eShader::PixelShader, CBImageFilterParam::kSlot);
        }
    }

    // constants
    if (spec.bNeedKernel)
    {
        if (_desc.kernelCBufferOrEmpty.IsEmpty())
        {
            const String name = MakeKernelBufferName_(_name);
            _graph.EmplaceConstantBuffer(name, sizeof(CBImageFilterKernel));
            builder.SetConstantBuffer(name, eShader::PixelShader, CBImageFilterKernel::kSlot);
        }
        else
        {
            builder.SetConstantBuffer(_desc.kernelCBufferOrEmpty, eShader::PixelShader, CBImageFilterKernel::kSlot);
        }
    }

    // output
    if (_desc.output == eImageFilterOutput::Inject)
    {
        JAM_ASSERT(!_desc.frameBuffer.IsEmpty(), "Frame buffer name cannot be empty when output is set.");
        builder.SetFrameBuffer(_desc.frameBuffer);
    }
    else if (_desc.output == eImageFilterOutput::Create)
    {
        // create in-place
        // _name Texture
        // _name FrameBuffer

        const String texName = String::Formatted("{}_T", _name);
        const String fbName  = String::Formatted("{}_FB", _name);

        if (_desc.bUseAutoResize)
        {
            _graph.EmplaceTexture2D(texName, _desc.scale, _desc.format, false, 1, eTextureFlags::RenderTarget);
        }
        else
        {
            _graph.EmplaceTexture2D(texName, _desc.width, _desc.height, _desc.format, false, 1, eTextureFlags::RenderTarget);
        }

        _graph.EmplaceFrameBuffer(fbName, texName);
        builder.SetFrameBuffer(fbName);
    }
    else
    {
        builder.AsFinalPass();
    }

    // common states
    builder.SetSampler(kLinearClamp, eShader::PixelShader, kSLinearClampSlot)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetRenderFlags(eRenderFlags::None);

    // executor
    if (_desc.executorOrNull)
    {
        builder.SetExecutor(_desc.executorOrNull);
    }
    else
    {
        builder.SetExecutor(Bind<&ImageFilterManager::SubmitScreenTriangle>(this));
    }

    // input 0
    if (_desc.bUseTexture2DArrayInput0)
    {
        builder.SetTexture(_desc.input0Texture, eShader::PixelShader, kTArrayInput0Slot);
    }
    else
    {
        builder.SetTexture(_desc.input0Texture, eShader::PixelShader, kTInput0Slot);
    }

    // input 1
    if (spec.bNeedInput1)
    {
        if (_desc.bUseTexture2DArrayInput1)
        {
            builder.SetTexture(_desc.input1Texture, eShader::PixelShader, kTArrayInput1Slot);
        }
        else
        {
            builder.SetTexture(_desc.input1Texture, eShader::PixelShader, kTInput1Slot);
        }
    }

    return builder;
}

ProgramHandle ImageFilterManager::GetOrLoadProgram_(
    const eImageFilter _filter,
    const bool         _bUseTexture2DArrayInput0,
    const bool         _bUseTexture2DArrayInput1)
{
    const Int32   index = MakeProgramsIndex_(_bUseTexture2DArrayInput0, _bUseTexture2DArrayInput1);
    ProgramHandle ph    = m_programs[index][_filter];
    if (ph)
    {
        return ph;
    }

    // pixel shader compile info
    const ImageFilterSpec& pci    = m_imageFilterSpecs[_filter];
    Vector<ShaderMacro>    macros = pci.macros;
    if (_bUseTexture2DArrayInput0)
    {
        macros.push_back({ "TEXTURE2DARRAY_INPUT0", "" });
    }
    if (_bUseTexture2DArrayInput1)
    {
        macros.push_back({ "TEXTURE2DARRAY_INPUT1", "" });
    }

    ShaderCompileInfo pscInfo = {};
    pscInfo.stage             = eShader::PixelShader;
    pscInfo.macros            = macros;
    pscInfo.entryPoint        = "PSMain";
    pscInfo.flags             = eShaderCompileFlags::Default;

    constexpr ShaderCompileInfo kVscInfo = { eShader::VertexShader, "VSMain", {}, eShaderCompileFlags::Default };
    const Expect<ShaderBinary>  vsc      = ShaderBinary::CompileHLSLFile(pci.path, kVscInfo);
    const Expect<ShaderBinary>  psc      = ShaderBinary::CompileHLSLFile(pci.path, pscInfo);
    JAM_ASSERT(vsc, "Vertex shader compilation failed: {}", vsc.GetError());
    JAM_ASSERT(psc, "Pixel shader compilation failed: {}", psc.GetError());

    Graphics&          gfx            = GetGraphics();
    const ShaderHandle vsh            = gfx.CreateShader(vsc->GetBytecode());
    const ShaderHandle psh            = gfx.CreateShader(psc->GetBytecode());
    return m_programs[index][_filter] = gfx.CreateProgram(vsh, psh, true);
}

Int32 ImageFilterManager::MakeProgramsIndex_(
    const bool _bUseTexture2DArrayInput0,
    const bool _bUseTexture2DArrayInput1) const
{
    Int32 index = 0;
    if (_bUseTexture2DArrayInput0)
    {
        index |= 0b01;
    }
    if (_bUseTexture2DArrayInput1)
    {
        index |= 0b10;
    }
    return index;
}
