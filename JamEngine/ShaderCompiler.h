#pragma once
#include "GraphicsBase.h"
#include "MemView.h"

namespace jam
{

struct ShaderMacro
{
    [[nodiscard]] bool IsNullTerminator() const { return name == nullptr && value == nullptr; }

    const char* name  = nullptr;
    const char* value = nullptr;
};

class ShaderCompiler
{
public:
    ShaderCompiler()  = default;
    ~ShaderCompiler() = default;

    ShaderCompiler(const ShaderCompiler&)            = delete;
    ShaderCompiler& operator=(const ShaderCompiler&) = delete;
    ShaderCompiler(ShaderCompiler&&)                 = default;
    ShaderCompiler& operator=(ShaderCompiler&&)      = default;

    bool CompileFile(
        const fs::path&              _path,
        eShader                      _stage,
        std::string_view             _entryPoint,
        eShaderCompileFlags          _flags  = eShaderCompileFlags_Default,
        std::span<const ShaderMacro> _macros = {});

    bool CompileSource(
        std::string_view             _hlslSource,
        eShader                      _stage,
        std::string_view             _entryPoint,
        eShaderCompileFlags          _flags  = eShaderCompileFlags_Default,
        std::span<const ShaderMacro> _macros = {});

    bool LoadFile(const fs::path& _path);
    bool LoadSource(ConstMemView _bytecode);

    [[nodiscard]] ConstMemView GetBytecode() const;
    [[nodiscard]] bool         IsValid() const;

private:
    [[nodiscard]] UINT        CompileFlagsFrom_(eShaderCompileFlags _flags) const;
    [[nodiscard]] const char* ShaderModelNameFrom_(eShader _stage) const;

    ComPtr<ID3DBlob> m_pBlob = nullptr;
};

}   // namespace jam
