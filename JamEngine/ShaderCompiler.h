#pragma once
#include "GraphicsBase.h"
#include "MemView.h"

namespace jam
{

class ShaderCompiler
{
    struct Macro
    {
        [[nodiscard]] bool IsNull() const
        {
            return (name == nullptr) && (value == nullptr);
        }

        const char* name  = nullptr;
        const char* value = nullptr;
    };

public:
    ShaderCompiler()  = default;
    ~ShaderCompiler() = default;

    ShaderCompiler(const ShaderCompiler&)            = delete;
    ShaderCompiler& operator=(const ShaderCompiler&) = delete;
    ShaderCompiler(ShaderCompiler&&)                 = default;
    ShaderCompiler& operator=(ShaderCompiler&&)      = default;

    bool CompileFile(
        const fs::path&     _path,
        eShader             _stage,
        StringView          _entryPoint,
        eShaderCompileFlags _flags = eShaderCompileFlags_Default);

    bool CompileSource(
        StringView          _hlslSource,
        eShader             _stage,
        StringView          _entryPoint,
        eShaderCompileFlags _flags = eShaderCompileFlags_Default);

    bool LoadFile(const fs::path& _path);
    bool LoadSource(CMemView _bytecode);

    void AddMacro(StringView _name, StringView _value);
    void ResetMacros();

    [[nodiscard]] bool     IsValid() const;
    [[nodiscard]] CMemView GetBytecode() const;

private:
    [[nodiscard]] UINT       CompileFlagsFrom_(eShaderCompileFlags _flags) const;
    [[nodiscard]] StringView ShaderModelNameFrom_(eShader _stage) const;
    void                     PreProcess();

    ComPtr<ID3DBlob> m_pBlob      = nullptr;
    Vector<Macro>    m_macros     = {};
};

}   // namespace jam
