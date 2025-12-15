#include "pch.h"
#include "Utils.h"

ShaderHandle CompileShader(const std::string_view _source, const eShader _stage, const std::string_view _entryPoint)
{
    ShaderCompiler compiler;
    if (!compiler.CompileSource(_source, _stage, _entryPoint))
    {
        Log::Error("Shader compilation failed. {}", ErrorBuffer::Get());
        JAM_ASSERT(0, "Shader compilation failed.");
    }
    return Graphics::CreateShader(compiler.GetBytecode());
}