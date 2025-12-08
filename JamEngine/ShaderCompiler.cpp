#include "pch.h"

#include "ShaderCompiler.h"

#include "FileUtilities.h"
#include "DialogUtilities.h"

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

namespace jam
{

static UINT CreateCompileFlags(const eShaderCompileFlags _flags)
{
    UINT compileFlags = 0;

    if (_flags & eShaderCompileFlags_Debug)
        compileFlags |= D3DCOMPILE_DEBUG;
    if (_flags & eShaderCompileFlags_SkipOptimization)
        compileFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
    if (_flags & eShaderCompileFlags_PackMatrixRowMajor)
        compileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
    if (_flags & eShaderCompileFlags_PackMatrixColumnMajor)
        compileFlags |= D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;
    if (_flags & eShaderCompileFlags_Optimization)
        compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
    if (_flags & eShaderCompileFlags_AvoidFlowControl)
        compileFlags |= D3DCOMPILE_AVOID_FLOW_CONTROL;
    if (_flags & eShaderCompileFlags_WarningsAsErrors)
        compileFlags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;

    return compileFlags;
}

constexpr std::array<const char*, EnumCount<eShader>()> k_ShaderModelTable {
    "vs_5_0",
    "ps_5_0",
    "gs_5_0",
    "hs_5_0",
    "ds_5_0",
    "cs_5_0",
};

ConstMemView ShaderCompiler::GetBytecode() const
{
    JAM_ASSERT(IsValid(), "Shader bytecode is not available.");
    return { m_pBlob->GetBufferPointer(), m_pBlob->GetBufferSize() };
}

bool ShaderCompiler::IsValid() const
{
    return m_pBlob;
}

bool ShaderCompiler::CompileFromFile(
    const fs::path&                    _path,
    const eShader                      _stage,
    const std::string_view             _entryPoint,
    const eShaderCompileFlags          _flags,
    const std::span<const ShaderMacro> _macros)
{
    JAM_ASSERT(_stage != eShader::Unknown, "Invalid shader stage.");
    JAM_ASSERT(_macros.empty() || _macros.back().IsNullTerminator(), "Shader macros must be null-terminated.");
    JAM_ASSERT(!_path.empty(), "File path must not be empty.");
    JAM_ASSERT(!_entryPoint.empty(), "Entry point must not be empty.");

    const D3D_SHADER_MACRO* pMacros = _macros.empty() ? nullptr : reinterpret_cast<const D3D_SHADER_MACRO*>(_macros.data());
    const std::wstring      path    = _path.wstring();

    ComPtr<ID3DBlob> pErrorBlob;
    const HRESULT    hr = D3DCompileFromFile(
        path.c_str(),
        pMacros,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        _entryPoint.data(),
        k_ShaderModelTable[EnumToInt(_stage)],
        CreateCompileFlags(_flags),
        0,
        m_pBlob.ReleaseAndGetAddressOf(),
        pErrorBlob.ReleaseAndGetAddressOf());

    if (FAILED(hr) || !m_pBlob)
    {
        if (pErrorBlob)
        {
            std::string_view msg { static_cast<const char*>(pErrorBlob->GetBufferPointer()), pErrorBlob->GetBufferSize() };
            SetErrorString("Shader compilation failed: {}", msg);
            return false;
        }
        else
        {
            std::string msg = CreateComErrorString(hr);
            SetErrorString("Shader compilation failed: {}", msg);
            return false;
        }
    }

    return true;
}

bool ShaderCompiler::CompileFromSource(
    const std::string_view             _hlslSource,
    const eShader                      _stage,
    const std::string_view             _entryPoint,
    const eShaderCompileFlags          _flags,
    const std::span<const ShaderMacro> _macros)
{
    JAM_ASSERT(_stage != eShader::Unknown, "Invalid shader stage.");
    JAM_ASSERT(!_hlslSource.empty(), "File path must not be empty.");
    JAM_ASSERT(!_entryPoint.empty(), "Entry point must not be empty.");
    JAM_ASSERT(_macros.empty() || _macros.back().IsNullTerminator(), "Shader macros must end with NullTerminator.");

    const D3D_SHADER_MACRO* pMacros = _macros.empty() ? nullptr : reinterpret_cast<const D3D_SHADER_MACRO*>(_macros.data());

    ComPtr<ID3DBlob> pErrorBlob;
    const HRESULT    hr = D3DCompile(
        _hlslSource.data(),
        _hlslSource.size(),
        nullptr,
        pMacros,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        _entryPoint.data(),
        k_ShaderModelTable[EnumToInt(_stage)],
        CreateCompileFlags(_flags),
        0,
        m_pBlob.ReleaseAndGetAddressOf(),
        pErrorBlob.ReleaseAndGetAddressOf());

    if (FAILED(hr) || !m_pBlob)
    {
        if (pErrorBlob)
        {
            std::string_view msg { static_cast<const char*>(pErrorBlob->GetBufferPointer()), pErrorBlob->GetBufferSize() };
            SetErrorString("Shader compilation failed: {}", msg);
            return false;
        }
        else
        {
            std::string msg = CreateComErrorString(hr);
            SetErrorString("Shader compilation failed: {}", msg);
            return false;
        }
    }

    return true;
}

bool ShaderCompiler::LoadFromFile(const fs::path& _filePath)
{
    if (const auto optBin = LoadBinaryFromFile(_filePath))
    {
        return LoadFromMemory(optBin.value());
    }
    else
    {
        return false;
    }
}

bool ShaderCompiler::LoadFromMemory(const ConstMemView _byteCode)
{
    JAM_CHECK_COM(D3DCreateBlob(_byteCode.GetByteWidth(), m_pBlob.ReleaseAndGetAddressOf()));
    std::memcpy(m_pBlob->GetBufferPointer(), _byteCode.GetDataPtr(), _byteCode.GetByteWidth());
    return true;
}

}   // namespace jam