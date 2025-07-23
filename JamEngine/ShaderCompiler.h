#pragma once

namespace jam
{

enum class eShaderCompileOption
{
    Default,
    Debug,       // = Defuat in debug mode
    Optimized,   // = Default in release mode
};

class ShaderCompiler
{
public:
    bool CompileHLSLFromFile(const fs::path& _filename, std::string_view _entryPoint, std::string_view _target, const D3D_SHADER_MACRO* _macros_orNull = nullptr, eShaderCompileOption _compileOption = eShaderCompileOption::Default);
    bool CompileHLSL(std::string_view _pSource, std::string_view _entryPoint, std::string_view _target, const D3D_SHADER_MACRO* _macros_orNull = nullptr, eShaderCompileOption _compileOption = eShaderCompileOption::Default);
    bool CompileCSOFromFile(const fs::path& _filename);
    bool CompileCSO(std::string_view _pSource);

    void GetCompiledShader(ID3DBlob** _out_ppBlob) const;
    bool SaveCompiledShader(const fs::path& _filename) const;

private:
    ComPtr<ID3DBlob> m_pCompiled = nullptr;
};

}   // namespace jam