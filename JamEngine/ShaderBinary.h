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

struct ShaderCompileInfo
{
    eShader                    stage      = eShader::VertexShader;
    StringView                 entryPoint = "main";
    Span<const ShaderMacro>    macros     = {};
    Flags<eShaderCompileFlags> flags      = eShaderCompileFlags::Default;
};

class ShaderBinary
{
public:
    [[nodiscard]] static Expect<ShaderBinary> CompileFromHLSLFile(StringView _path, const ShaderCompileInfo& _info = {});
    [[nodiscard]] static Expect<ShaderBinary> CompileFromHLSLSource(StringView _source, const ShaderCompileInfo& _info = {});
    [[nodiscard]] static Expect<ShaderBinary> FromCSOFile(StringView _path);
    explicit ShaderBinary(MemoryView _mem);

    [[nodiscard]] MemoryView GetBytecode() const;

private:
    ShaderBinary() = default;

    Error CompileFromHLSLFile_(StringView _path, const ShaderCompileInfo& _info);
    Error CompileFromHLSLSource_(StringView _source, const ShaderCompileInfo& _info);
    Error LoadFromCSOFile_(StringView _path);

    Microsoft::WRL::ComPtr<ID3DBlob> m_pBlob = nullptr;
};

}   // namespace Jam