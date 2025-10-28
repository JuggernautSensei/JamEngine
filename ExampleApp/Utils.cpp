#include "pch.h"
#include "Utils.h"

ShaderHandle CompileShader(const std::string_view _source, const eShader _stage, const std::string_view _entryPoint)
{
    ShaderCompiler compiler;
    eErrorCode     ec = compiler.CompileFromSource(_source, _stage, _entryPoint);
    if (IsError(ec))
    {
        Log::Error("Shader compilation failed. {}", GetErrorMessage());
        JAM_ASSERT(0, "Shader compilation failed.");
    }
    return Graphics::CreateShader(compiler.GetBytecode());
}