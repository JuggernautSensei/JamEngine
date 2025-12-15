#include "pch.h"

#include "ShaderCompiler.h"

#include "FileUtilities.h"

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

namespace jam
{

UINT ShaderCompiler::CompileFlagsFrom_(const eShaderCompileFlags _flags) const
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

const char* ShaderCompiler::ShaderModelNameFrom_(const eShader _stage) const
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

bool ShaderCompiler::CompileFile(
    const fs::path&                    _path,
    const eShader                      _stage,
    const std::string_view             _entryPoint,
    const eShaderCompileFlags          _flags,
    const std::span<const ShaderMacro> _macros)
{
    JAM_ASSERT(!_path.empty(), "File path must not be empty.");
    JAM_ASSERT(!_entryPoint.empty(), "Entry point must not be empty.");

    const D3D_SHADER_MACRO* pMacros = nullptr;
    if (_macros.empty())
    {
        JAM_ASSERT(_macros.back().IsNullTerminator(), "Shader macros must end with NullTerminator.");
        pMacros = reinterpret_cast<const D3D_SHADER_MACRO*>(_macros.data());
    }

    ComPtr<ID3DBlob>   pErrorBlob;
    const std::wstring path = _path.wstring();

    const HRESULT hr = D3DCompileFromFile(
        path.c_str(),
        pMacros,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        _entryPoint.data(),
        ShaderModelNameFrom_(_stage),
        CompileFlagsFrom_(_flags),
        0,
        m_pBlob.ReleaseAndGetAddressOf(),
        pErrorBlob.ReleaseAndGetAddressOf());

    if (FAILED(hr) || !m_pBlob)
    {
        if (pErrorBlob)
        {
            std::string_view msg = { static_cast<const char*>(pErrorBlob->GetBufferPointer()), pErrorBlob->GetBufferSize() };
            ErrorBuffer::Set("shader compilation failed: {}", msg);
            return false;
        }
        else
        {
            ErrorBuffer::Set("shader compilation failed: {}", ErrorMessageFrom(hr));
            return false;
        }
    }

    return true;
}

bool ShaderCompiler::CompileSource(
    const std::string_view             _hlslSource,
    const eShader                      _stage,
    const std::string_view             _entryPoint,
    const eShaderCompileFlags          _flags,
    const std::span<const ShaderMacro> _macros)
{
    JAM_ASSERT(!_hlslSource.empty(), "File path must not be empty.");
    JAM_ASSERT(!_entryPoint.empty(), "Entry point must not be empty.");

    const D3D_SHADER_MACRO* pMacros = nullptr;
    if (_macros.empty())
    {
        JAM_ASSERT(_macros.back().IsNullTerminator(), "Shader macros must end with NullTerminator.");
        pMacros = reinterpret_cast<const D3D_SHADER_MACRO*>(_macros.data());
    }

    ComPtr<ID3DBlob> pErrorBlob;
    const HRESULT    hr = D3DCompile(
        _hlslSource.data(),
        _hlslSource.size(),
        nullptr,
        pMacros,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        _entryPoint.data(),
        ShaderModelNameFrom_(_stage),
        CompileFlagsFrom_(_flags),
        0,
        m_pBlob.ReleaseAndGetAddressOf(),
        pErrorBlob.ReleaseAndGetAddressOf());

    if (FAILED(hr) || !m_pBlob)
    {
        if (pErrorBlob)
        {
            std::string_view msg = { static_cast<const char*>(pErrorBlob->GetBufferPointer()), pErrorBlob->GetBufferSize() };
            ErrorBuffer::Set("shader compilation failed: {}", msg);
            return false;
        }
        else
        {
            ErrorBuffer::Set("shader compilation failed: {}", ErrorMessageFrom(hr));
            return false;
        }
    }

    return true;
}

bool ShaderCompiler::LoadFile(const fs::path& _path)
{
    if (const std::optional<Vector<Byte>> opt = LoadBinaryFile(_path))
    {
        return LoadSource(*opt);
    }
    else
    {
        return false;
    }
}

bool ShaderCompiler::LoadSource(const ConstMemView _bytecode)
{
    JAM_CHECK_COM(D3DCreateBlob(_bytecode.GetByteWidth(), m_pBlob.ReleaseAndGetAddressOf()));
    std::memcpy(m_pBlob->GetBufferPointer(), _bytecode.GetPointer(), _bytecode.GetByteWidth());
    return true;
}

ConstMemView ShaderCompiler::GetBytecode() const
{
    JAM_ASSERT(IsValid(), "Shader bytecode is not available.");
    return { m_pBlob->GetBufferPointer(), m_pBlob->GetBufferSize() };
}

bool ShaderCompiler::IsValid() const
{
    return m_pBlob;
}

}   // namespace jam