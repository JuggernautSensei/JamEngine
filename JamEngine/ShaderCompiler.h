#pragma once
#include "GraphicsBase.h"
#include "MemView.h"

namespace jam
{

struct ShaderMacro
{
    [[nodiscard]] bool IsNull() const
    {
        return (name == nullptr) && (value == nullptr);
    }

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

    bool CompileHLSLFile(const fs::path& _path, eShader _stage, StringView _entryPoint, std::span<const ShaderMacro> _macrosOrEmpty, eShaderCompileFlags _flags = eShaderCompileFlags_Default);
    bool CompileHLSLSource(StringView _source, eShader _stage, StringView _entryPoint, std::span<const ShaderMacro> _macrosOrEmpty, eShaderCompileFlags _flags = eShaderCompileFlags_Default);

    bool LoadCSOFile(const fs::path& _path);
    bool LoadCSOMemory(CMemView _bytecode);

    [[nodiscard]] ShaderHandle GetShader() const;

private:
    [[nodiscard]] UINT       CompileFlagsFrom_(eShaderCompileFlags _flags) const;
    [[nodiscard]] StringView ShaderModelNameFrom_(eShader _stage) const;

    ShaderHandle m_sh = k_invalidHandle;
};

}   // namespace jam
