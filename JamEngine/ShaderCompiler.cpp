#include "pch.h"

#include "ShaderCompiler.h"

#include "FileIOUtilities.h"

#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

namespace jam
{

UINT ShaderCompiler::CompileFlagsFrom_(
    const eShaderCompileFlags _flags) const
{
    UINT flags = 0;

    if (_flags & eShaderCompileFlags_Debug)
        flags |= D3DCOMPILE_DEBUG;

    if (_flags & eShaderCompileFlags_SkipOptimization)
        flags |= D3DCOMPILE_SKIP_OPTIMIZATION;

    if (_flags & eShaderCompileFlags_PackMatrixRowMajor)
        flags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

    if (_flags & eShaderCompileFlags_PackMatrixColumnMajor)
        flags |= D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;

    if (_flags & eShaderCompileFlags_Optimization)
        flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;

    if (_flags & eShaderCompileFlags_AvoidFlowControl)
        flags |= D3DCOMPILE_AVOID_FLOW_CONTROL;

    if (_flags & eShaderCompileFlags_WarningsAsErrors)
        flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;

    return flags;
}

StringView ShaderCompiler::ShaderModelNameFrom_(
    const eShader _stage) const
{
    switch (_stage)
    {
        case eShader::VertexShader:
            return "vs_5_0";
        case eShader::PixelShader:
            return "ps_5_0";
        case eShader::GeometryShader:
            return "gs_5_0";
        case eShader::HullShader:
            return "hs_5_0";
        case eShader::DomainShader:
            return "ds_5_0";
        case eShader::ComputeShader:
            return "cs_5_0";
        default:
            JAM_ASSERT(0, "Invalid shader stage.");
            return nullptr;
    }
}

bool ShaderCompiler::CompileHLSLFile(
    const fs::path&               _path,
    const eShader                 _stage,
    const StringView              _entryPoint,
    const Span<const ShaderMacro> _macrosOrEmpty,
    const eShaderCompileFlags     _flags) const
{
    JAM_ASSERT(!_path.empty(), "File path must not be empty.");
    JAM_ASSERT(!_entryPoint.IsEmpty(), "Entry point must not be empty.");

    const D3D_SHADER_MACRO* pMacros = nullptr;
    if (!_macrosOrEmpty.empty())
    {
        JAM_ASSERT(_macrosOrEmpty.back().IsNull(), "Shader macros must be null-terminated.");
        pMacros = reinterpret_cast<const D3D_SHADER_MACRO*>(_macrosOrEmpty.data());
    }

    ComPtr<ID3DBlob>   pBlob      = nullptr;
    ComPtr<ID3DBlob>   pErrorBlob = nullptr;
    const std::wstring path       = _path.wstring();
    const HRESULT      hr         = D3DCompileFromFile(
        path.c_str(),
        pMacros,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        _entryPoint.GetPtr(),
        ShaderModelNameFrom_(_stage).GetPtr(),
        CompileFlagsFrom_(_flags),
        0,
        pBlob.ReleaseAndGetAddressOf(),
        pErrorBlob.ReleaseAndGetAddressOf());

    if (FAILED(hr) || !pBlob)
    {
        if (pErrorBlob)
        {
            const char* pErrorMsg = static_cast<const char*>(pErrorBlob->GetBufferPointer());
            GlobalBuffer::SetLastError("shader compilation failed: {}", pErrorMsg);
            return false;
        }
        else
        {
            GlobalBuffer::SetLastError("shader compilation failed: {}", CreateErrorMessageFromHRESULT(hr));
            return false;
        }
    }

    return true;
}

bool ShaderCompiler::CompileHLSLSource(
    const StringView              _source,
    const eShader                 _stage,
    const StringView              _entryPoint,
    const Span<const ShaderMacro> _macrosOrEmpty,
    const eShaderCompileFlags     _flags) const
{
    JAM_ASSERT(!_source.IsEmpty(), "File path must not be empty.");
    JAM_ASSERT(!_entryPoint.IsEmpty(), "Entry point must not be empty.");

    const D3D_SHADER_MACRO* pMacros = nullptr;
    if (!_macrosOrEmpty.empty())
    {
        JAM_ASSERT(_macrosOrEmpty.back().IsNull(), "Shader macros must be null-terminated.");
        pMacros = reinterpret_cast<const D3D_SHADER_MACRO*>(_macrosOrEmpty.data());
    }

    ComPtr<ID3DBlob> pBlob      = nullptr;
    ComPtr<ID3DBlob> pErrorBlob = nullptr;
    const HRESULT    hr         = D3DCompile(
        _source.GetPtr(),
        _source.GetLength(),
        nullptr,
        pMacros,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        _entryPoint.GetPtr(),
        ShaderModelNameFrom_(_stage).GetPtr(),
        CompileFlagsFrom_(_flags),
        0,
        pBlob.ReleaseAndGetAddressOf(),
        pErrorBlob.ReleaseAndGetAddressOf());

    if (FAILED(hr) || !pBlob)
    {
        if (pErrorBlob)
        {
            const char* pErrorMsg = static_cast<const char*>(pErrorBlob->GetBufferPointer());
            GlobalBuffer::SetLastError("shader compilation failed: {}", pErrorMsg);
        }
        else
        {
            GlobalBuffer::SetLastError("shader compilation failed: {}", CreateErrorMessageFromHRESULT(hr));
            return false;
        }
    }

    return true;
}

bool ShaderCompiler::LoadCSOFile(
    const fs::path& _path)
{
    Vector<Byte> data = {};
    if (!MoveIfHasValue(LoadBinaryFile(_path), data))
    {
        return false;
    }

    return LoadCSOMemory(data);
}

bool ShaderCompiler::LoadCSOMemory(
    const CMemView _bytecode)
{
    JAM_ASSERT(!_bytecode.IsEmpty(), "Bytecode must not be empty.");
    const HRESULT hr = D3DCreateBlob(_bytecode.GetByteWidth(), m_pBytecode.ReleaseAndGetAddressOf());
    JAM_ASSERT(SUCCEEDED(hr) && m_pBytecode, "Failed to create D3D blob for bytecode.");
    return true;
}

CMemView ShaderCompiler::GetBytecode() const
{
    JAM_ASSERT(m_pBytecode, "Shader bytecode is not loaded.");
    return { m_pBytecode->GetBufferPointer(), static_cast<Int32>(m_pBytecode->GetBufferSize()) };
}

}   // namespace jam