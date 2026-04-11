#include "pch.h"
#include "Utils.h"

ShaderHandle CompileShader(const std::string_view _source, const eShader _stage, const std::string_view _entryPoint)
{
    ShaderCodeBlob compiler;
    if (!compiler.CompileSource(_source, _stage, _entryPoint))
    {
        LogSystem::Error("Shader compilation failed. {}", GlobalBuffer::GetLastError());
        JAM_UNREACHABLE;
    }
    return Graphics::CreateShader(compiler.GetBytecode());
}