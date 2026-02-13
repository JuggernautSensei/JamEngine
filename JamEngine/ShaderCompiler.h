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

struct ShaderCompilerDesc
{
    eShader                    stage      = eShader::VertexShader;
    StringView                 entryPoint = "main";
    Span<const ShaderMacro>    macros     = {};
    Flags<eShaderCompileFlags> flags      = eShaderCompileFlags::None;
};

class ShaderCompiler
{
public:
    [[nodiscard]] static Expect<ShaderCompiler> CompileFromHLSLFile(const Path& _path, const ShaderCompilerDesc& _desc = {});
    [[nodiscard]] static Expect<ShaderCompiler> CompileFromHLSLSource(StringView _source, const ShaderCompilerDesc& _desc = {});
    [[nodiscard]] CMemoryBlock                  GetBytecode() const;

private:
    ShaderCompiler() = default;
    Error CompileFromHLSLFile_(const Path& _path, const ShaderCompilerDesc& _desc = {});
    Error CompileHLSLSource_(StringView _source, const ShaderCompilerDesc& _desc = {});

    ComPtr<ID3DBlob> m_pBlob = nullptr;
};

}   // namespace Jam