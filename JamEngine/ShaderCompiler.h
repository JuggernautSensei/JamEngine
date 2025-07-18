#pragma once

namespace jam
{

class ShaderCompiler
{
public:
    bool CompileHLSLFromFile(const fs::path& _filename, std::string_view _entryPoint, std::string_view _target, const D3D_SHADER_MACRO* _macros, bool _bDebug);
    bool CompileHLSL(std::string_view _pSource, std::string_view _entryPoint, std::string_view _target, const D3D_SHADER_MACRO* _macros, bool _bDebug);
    bool CompileCSOFromFile(const fs::path& _filename);
    bool CompileCSO(std::string_view _pSource);

    NODISCARD ComPtr<ID3DBlob> GetCompiledBlob() const { return m_pCompiled; }
    bool                       SaveCompiledShader(const fs::path& _filename) const;

private:
    ComPtr<ID3DBlob> m_pCompiled = nullptr;

    static UINT k_debugCompileFlags;
    static UINT k_releaseCompileFlags;
};

}   // namespace jam