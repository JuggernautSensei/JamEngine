#include "pch.h"

#include "ShaderCompiler.h"

#include "FileUtilities.h"
#include "WindowsUtilities.h"

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

using namespace jam;

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

namespace jam
{

eErrorCode ShaderCompiler::CompileFromFile(
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

    Clear();

    const D3D_SHADER_MACRO* pMacros = _macros.empty() ? nullptr : reinterpret_cast<const D3D_SHADER_MACRO*>(_macros.data());
    std::wstring            path    = _path.wstring();

    ComPtr<ID3DBlob> pErrorBlob;
    HRESULT          hr = D3DCompileFromFile(
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
        HandleCompileError_(pErrorBlob, hr);
        return eErrorCode::ShaderCompileFailed;
    }

    return eErrorCode::Ok;
}

eErrorCode ShaderCompiler::CompileFromSource(
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

    Clear();

    const D3D_SHADER_MACRO* pMacros = _macros.empty() ? nullptr : reinterpret_cast<const D3D_SHADER_MACRO*>(_macros.data());

    ComPtr<ID3DBlob> pErrorBlob;
    HRESULT          hr = D3DCompile(
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
        HandleCompileError_(pErrorBlob, hr);
        return eErrorCode::ShaderCompileFailed;
    }

    return eErrorCode::Ok;
}

eErrorCode ShaderCompiler::LoadFromFile(const fs::path& _filePath)
{
    auto [ec, buffer] = ReadBinaryFromFile(_filePath);

    if (IsError(ec))
        return ec;

    return LoadFromMemory(MemoryView { buffer });
}

eErrorCode ShaderCompiler::LoadFromMemory(const MemoryView _byteCode)
{
    Clear();

    JAM_CHECK_COM(D3DCreateBlob(_byteCode.GetByteWidth(), m_pBlob.ReleaseAndGetAddressOf()));
    std::memcpy(m_pBlob->GetBufferPointer(), _byteCode.GetDataPtr(), _byteCode.GetByteWidth());

    return eErrorCode::Ok;
}

void ShaderCompiler::HandleCompileError_(const ComPtr<ID3DBlob>& _pErrorBlob, const HRESULT _hr) const
{
    JAM_LOGGER_DEBUG("Shader compilation failed with error: {}", CreateComErrorMessage(_hr));

    if (_pErrorBlob)
    {
        std::string_view errorStr {
            static_cast<const char*>(_pErrorBlob->GetBufferPointer()),
            _pErrorBlob->GetBufferSize()
        };
        SetErrorMessage("Shader compilation failed: {}", errorStr);
    }
    else
    {
        SetErrorMessage("Shader compilation failed with unknown error.");
    }
}

}   // namespace jam