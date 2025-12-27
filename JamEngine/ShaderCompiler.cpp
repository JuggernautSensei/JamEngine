#include "pch.h"

#include "ShaderCompiler.h"

#include "FileIOUtilities.h"

#include <d3dcompiler.h>

#include "Graphics.h"
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
    const fs::path&                    _path,
    const eShader                      _stage,
    const StringView                   _entryPoint,
    const std::span<const ShaderMacro> _macrosOrEmpty,
    const eShaderCompileFlags          _flags)
{
    JAM_ASSERT(!_path.empty(), "File path must not be empty.");
    JAM_ASSERT(!_entryPoint.IsEmpty(), "Entry point must not be empty.");

    const D3D_SHADER_MACRO* pMacros = nullptr;
    if (!_macrosOrEmpty.empty())
    {
        JAM_ASSERT(_macrosOrEmpty.back().IsNull(), "Shader macros must be null-terminated.");
        pMacros = reinterpret_cast<const D3D_SHADER_MACRO*>(_macrosOrEmpty.data());
    }

    ComPtr<ID3DBlob>   pBlob, pErrorBlob;
    const std::wstring path = _path.wstring();
    const HRESULT      hr   = D3DCompileFromFile(
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
            GlobalBuffer::SetLastError("shader compilation failed: {}", StringFrom(hr));
            return false;
        }
    }

    m_sh = Graphics::CreateShader({ pBlob->GetBufferPointer(), NarrowCast<Int32>(pBlob->GetBufferSize()) });
    return true;
}

bool ShaderCompiler::CompileHLSLSource(
    const StringView                   _source,
    const eShader                      _stage,
    const StringView                   _entryPoint,
    const std::span<const ShaderMacro> _macrosOrEmpty,
    const eShaderCompileFlags          _flags)
{
    JAM_ASSERT(!_source.IsEmpty(), "File path must not be empty.");
    JAM_ASSERT(!_entryPoint.IsEmpty(), "Entry point must not be empty.");

    const D3D_SHADER_MACRO* pMacros = nullptr;
    if (!_macrosOrEmpty.empty())
    {
        JAM_ASSERT(_macrosOrEmpty.back().IsNull(), "Shader macros must be null-terminated.");
        pMacros = reinterpret_cast<const D3D_SHADER_MACRO*>(_macrosOrEmpty.data());
    }

    ComPtr<ID3DBlob> pBlob, pErrorBlob;
    const HRESULT    hr = D3DCompile(
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
            GlobalBuffer::SetLastError("shader compilation failed: {}", StringFrom(hr));
            return false;
        }
    }

    m_sh = Graphics::CreateShader({ pBlob->GetBufferPointer(), NarrowCast<Int32>(pBlob->GetBufferSize()) });
    return true;
}

bool ShaderCompiler::LoadCSOFile(
    const fs::path& _path)
{
    if (const Optional<Vector<Byte>> optData = LoadBinaryFile(_path))
    {
        return LoadCSOMemory(optData.value());
    }
    else
    {
        return false;
    }
}

bool ShaderCompiler::LoadCSOMemory(
    const CMemView _bytecode)
{
    m_sh = Graphics::CreateShader(_bytecode);
    return true;
}

ShaderHandle ShaderCompiler::GetShader() const
{
    JAM_ASSERT(JAM_IS_VALID_HANDLE(m_sh), "Shader is not compiled or loaded.");
    return m_sh;
}

}   // namespace jam