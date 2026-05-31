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
    [[nodiscard]] static ShaderCompiler CompileHLSLFile(StringView _path, const ShaderCompilerInfo& _desc = {});
    [[nodiscard]] static ShaderCompiler CompileHLSLSource(StringView _source, const ShaderCompilerInfo& _desc = {});

    [[nodiscard]] MemoryView GetBytecode() const;
    [[nodiscard]] StringView GetErrorMessage() const;
    [[nodiscard]] Error      GetError() const;
    [[nodiscard]] bool       IsSucceeded() const;
    [[nodiscard]] bool       IsFailed() const;

private:
    void CompileFromHLSLFile_(StringView _path, const ShaderCompilerInfo& _desc);
    void CompileHLSLSource_(StringView _source, const ShaderCompilerInfo& _desc);
    ShaderCompiler() = default;

    ComPtr<ID3DBlob> m_pBlob    = nullptr;
    String           m_errorMsg = {};
    Error            m_error    = {};
};

}   // namespace Jam