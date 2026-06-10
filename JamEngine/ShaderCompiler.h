#pragma once
#include "GraphicsBase.h"
#include "MemoryView.h"

namespace Jam
{

struct ShaderMacro
{
    [[nodiscard]] bool IsNull() const
    {
        return name.IsEmpty() && value.IsEmpty();
    }

    StringView name  = {};
    StringView value = {};
};

struct ShaderCompilerInfo
{
    eShader                    stage      = eShader::VertexShader;
    StringView                 entryPoint = "main";
    Span<const ShaderMacro>    macros     = {};
    Flags<eShaderCompileFlags> flags      = eShaderCompileFlags::Default;
};

class ShaderCompiler
{
public:
    [[nodiscard]] static Expect<ShaderCompiler> CompileHLSLFile(StringView _path, const ShaderCompilerInfo& _desc = {});
    [[nodiscard]] static Expect<ShaderCompiler> CompileHLSLSource(StringView _source, const ShaderCompilerInfo& _desc = {});

    [[nodiscard]] MemoryView GetBytecode() const;

private:
    Error CompileFromHLSLFile_(StringView _path, const ShaderCompilerInfo& _desc);
    Error CompileHLSLSource_(StringView _source, const ShaderCompilerInfo& _desc);
    ShaderCompiler() = default;

    Microsoft::WRL::ComPtr<ID3DBlob> m_pBlob = nullptr;
};

}   // namespace Jam