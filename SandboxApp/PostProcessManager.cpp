#include "pch.h"
#include "PostProcessManager.h"

#define SHADER_PATH(_filename) "Resources/ImageFilter/" _filename

PostProcessManager& PostProcessManager::GetInstance()
{
    JAM_ASSERT(s_instance, "PostProcessManager instance does not exist. Call CreateInstance first.");
    return *s_instance;
}

ProgramHandle PostProcessManager::GetProgram(
    const ePostProcess _type)
{
    const ProgramHandle ph = m_programs[_type];
    if (ph)
    {
        return ph;
    }

    constexpr ShaderCompilerInfo kVsInfo = { eShader::VertexShader, "VSMain", {}, eShaderCompileFlags::Default };
    constexpr ShaderCompilerInfo kPsInfo = { eShader::PixelShader, "PSMain", {}, eShaderCompileFlags::Default };

    const StringView     path = m_programPaths[_type];
    const ShaderCompiler vsc  = ShaderCompiler::CompileHLSLFile(path, kVsInfo);
    const ShaderCompiler psc  = ShaderCompiler::CompileHLSLFile(path, kPsInfo);
    JAM_ASSERT(vsc.IsSucceeded(), "Vertex shader compilation failed: {}", vsc.GetErrorMessage());
    JAM_ASSERT(psc.IsSucceeded(), "Pixel shader compilation failed: {}", psc.GetErrorMessage());

    Graphics&          gfx = GetGraphics();
    const ShaderHandle vsh = gfx.CreateShader(vsc.GetBytecode());
    const ShaderHandle psh = gfx.CreateShader(psc.GetBytecode());
    m_programs[_type]      = gfx.CreateProgram(vsh, psh, true);
    return m_programs[_type];
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

    constexpr UInt16 kIndices[] = {
        0, 1, 2
    };

    m_pScreenTriangle = Mesh::Load(kVerts, kIndices);

    m_programPaths[ePostProcess::Blit]        = SHADER_PATH("BlitShader.hlsl");
    m_programPaths[ePostProcess::Mix]         = SHADER_PATH("MixShader.hlsl");
    m_programPaths[ePostProcess::Convolution] = SHADER_PATH("ConvolutionShader.hlsl");
}

PostProcessManager::~PostProcessManager()
{
    for (ProgramHandle ph: m_programs)
    {
        JAM_DESTROY_PROGRAM(ph);
    }
}

void PostProcessManager::CreateInstance()
{
    JAM_ASSERT(s_instance, "PostProcessManager instance already exists.");
    s_instance = new PostProcessManager();
}

void PostProcessManager::DestroyInstance()
{
    if (s_instance)
    {
        delete s_instance;
        s_instance = nullptr;
    }
}