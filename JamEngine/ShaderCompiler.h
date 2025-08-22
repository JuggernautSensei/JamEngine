#pragma once

namespace jam
{

enum class eShaderCompileOption
{
    Default,
    Debug,       // = Defuat in debug mode
    Optimized,   // = Default in release mode
};

struct ShaderMacro
{
    std::string_view name;    // macro displayName
    std::string_view value;   // macro value
};

class ShaderCompiler
{
public:
    bool CompileHLSLFromFile(const fs::path&              _filename,
                             std::string_view             _entryPoint,
                             std::string_view             _target,
                             eShaderCompileOption         _compileOption = eShaderCompileOption::Default,
                             std::span<const ShaderMacro> _macrosOrEmpty = {});

    bool CompileHLSL(std::string_view             _pSource,
                     std::string_view             _entryPoint,
                     std::string_view             _target,
                     eShaderCompileOption         _compileOption = eShaderCompileOption::Default,
                     std::span<const ShaderMacro> _macrosOrEmpty = {});

    bool LoadCSOFromFile(const fs::path& _filename);
    bool LoadCSO(const void* _pSource, size_t _sourceSize);

    void GetCompiledShader(ID3DBlob** _out_ppBlob) const;
    bool SaveCompiledShader(const fs::path& _filename) const;

private:
    ComPtr<ID3DBlob> m_pCompiled = nullptr;
};

}   // namespace jam