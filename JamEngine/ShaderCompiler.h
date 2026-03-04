#pragma once
#include "GraphicsBase.h"
#include "MemoryBlock.h"

namespace Jam
{

struct ShaderMacro
{
    [[nodiscard]] bool IsNull() const
    {
        return name == nullptr && value == nullptr;
    }

    StringView name  = nullptr;
    StringView value = nullptr;
};

struct ShaderCompilerInfo
{
    eShader                    stage      = eShader::VertexShader;
    StringView                 entryPoint = "main";
    Span<const ShaderMacro>    macros     = {};
    Flags<eShaderCompileFlags> flags      = eShaderCompileFlags::None;
};

class ShaderCompiler
{
public:
    [[nodiscard]] static Expect<ShaderCompiler> CompileFromHLSLFile(StringView _path, const ShaderCompilerInfo& _info = {});
    [[nodiscard]] static Expect<ShaderCompiler> CompileFromHLSLSource(StringView _source, const ShaderCompilerInfo& _info = {});
    [[nodiscard]] CMemoryBlock                  GetBytecode() const;
    [[nodiscard]] StringView                    GetErrorMessage() const;

private:
    ShaderCompiler() = default;
    Error CompileFromHLSLFile_(StringView _path, const ShaderCompilerInfo& _desc = {});
    Error CompileHLSLSource_(StringView _source, const ShaderCompilerInfo& _desc = {});

    ComPtr<ID3DBlob> m_pBlob    = nullptr;
    String           m_errorMsg = {};
};

}   // namespace Jam