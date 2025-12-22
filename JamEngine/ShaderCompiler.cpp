#include "pch.h"

#include "ShaderCompiler.h"

#include "FileUtilities.h"

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

void ShaderCompiler::PreProcess()
{
    if (!m_macros.empty())
    {
        const Macro& lastMacro = m_macros.back();
        if (!lastMacro.IsNull())
        {
            m_macros.emplace_back(nullptr, nullptr);   // Null-terminate the array
        }
    }
}

bool ShaderCompiler::CompileFile(
    const fs::path&           _path,
    const eShader             _stage,
    const StringView          _entryPoint,
    const eShaderCompileFlags _flags)
{
    JAM_ASSERT(!_path.empty(), "File path must not be empty.");
    JAM_ASSERT(!_entryPoint.IsEmpty(), "Entry point must not be empty.");

    PreProcess();
    const D3D_SHADER_MACRO* pMacros = m_macros.empty() ? nullptr : reinterpret_cast<const D3D_SHADER_MACRO*>(m_macros.data());

    ComPtr<ID3DBlob>   pErrorBlob;
    const std::wstring path = _path.wstring();
    const HRESULT      hr   = D3DCompileFromFile(
        path.c_str(),
        pMacros,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        _entryPoint.GetPtr(),
        ShaderModelNameFrom_(_stage).GetPtr(),
        CompileFlagsFrom_(_flags),
        0,
        m_pBlob.ReleaseAndGetAddressOf(),
        pErrorBlob.ReleaseAndGetAddressOf());

    if (FAILED(hr) || !m_pBlob)
    {
        if (pErrorBlob)
        {
            const char*      pErrorMsg = static_cast<const char*>(pErrorBlob->GetBufferPointer());
            const Int32      len       = NarrowCast<Int32>(pErrorBlob->GetBufferSize());
            const StringView msg       = { pErrorMsg, len };
            GlobalBuffer::SetLastError("shader compilation failed: {}", msg);
            return false;
        }
        else
        {
            GlobalBuffer::SetLastError("shader compilation failed: {}", StringFrom(hr));
            return false;
        }
    }

    return true;
}

bool ShaderCompiler::CompileSource(
    const StringView          _hlslSource,
    const eShader             _stage,
    const StringView          _entryPoint,
    const eShaderCompileFlags _flags)
{
    JAM_ASSERT(!_hlslSource.IsEmpty(), "File path must not be empty.");
    JAM_ASSERT(!_entryPoint.IsEmpty(), "Entry point must not be empty.");

    PreProcess();
    const D3D_SHADER_MACRO* pMacros = m_macros.empty() ? nullptr : reinterpret_cast<const D3D_SHADER_MACRO*>(m_macros.data());

    ComPtr<ID3DBlob> pErrorBlob;
    const HRESULT    hr = D3DCompile(
        _hlslSource.GetPtr(),
        _hlslSource.GetLength(),
        nullptr,
        pMacros,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        _entryPoint.GetPtr(),
        ShaderModelNameFrom_(_stage).GetPtr(),
        CompileFlagsFrom_(_flags),
        0,
        m_pBlob.ReleaseAndGetAddressOf(),
        pErrorBlob.ReleaseAndGetAddressOf());

    if (FAILED(hr) || !m_pBlob)
    {
        if (pErrorBlob)
        {
            const char*      pErrorMsg = static_cast<const char*>(pErrorBlob->GetBufferPointer());
            const Int32      len       = NarrowCast<Int32>(pErrorBlob->GetBufferSize());
            const StringView msg       = { pErrorMsg, len };
            GlobalBuffer::SetLastError("shader compilation failed: {}", msg);
        }
        else
        {
            GlobalBuffer::SetLastError("shader compilation failed: {}", StringFrom(hr));
            return false;
        }
    }

    return true;
}

bool ShaderCompiler::LoadFile(
    const fs::path& _path)
{
    if (const Optional<Vector<Byte>> opt = LoadBinaryFile(_path))
    {
        return LoadSource(*opt);
    }
    else
    {
        return false;
    }
}

bool ShaderCompiler::LoadSource(
    const CMemView _bytecode)
{
    JAM_CHECK_COM(D3DCreateBlob(_bytecode.GetByteWidth(), m_pBlob.ReleaseAndGetAddressOf()));
    std::memcpy(m_pBlob->GetBufferPointer(), _bytecode.GetPtr(), _bytecode.GetByteWidth());
    return true;
}

void ShaderCompiler::AddMacro(
    const StringView _name,
    const StringView _value)
{
    m_macros.emplace_back(_name.GetPtr(), _value.GetPtr());
}

void ShaderCompiler::ResetMacros()
{
    m_macros.clear();
}

CMemView ShaderCompiler::GetBytecode() const
{
    JAM_ASSERT(IsValid(), "Shader bytecode is not available.");
    return { m_pBlob->GetBufferPointer(), m_pBlob->GetBufferSize() };
}

bool ShaderCompiler::IsValid() const
{
    return m_pBlob;
}

}   // namespace jam