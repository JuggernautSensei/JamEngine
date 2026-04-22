#include "pch.h"

#include "ShaderCompiler.h"

#include <d3dcompiler.h>

#include "FileSystem.h"
#include "WindowsUtilities.h"
#pragma comment(lib, "d3dcompiler.lib")

namespace Jam
{

namespace
{
    [[nodiscard]] UINT CompileFlagsFrom_(
        const Flags<eShaderCompileFlags> _flags)
    {
        UINT flags = 0;

        if (_flags & eShaderCompileFlags::DebugInfo)
        {
            flags |= D3DCOMPILE_DEBUG;
        }

        if (_flags & eShaderCompileFlags::SkipOptimization)
        {
            flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
        }

        if (_flags & eShaderCompileFlags::PackMatrixRowMajor)
        {
            flags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
        }

        if (_flags & eShaderCompileFlags::PackMatrixColumnMajor)
        {
            flags |= D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;
        }

        if (_flags & eShaderCompileFlags::Optimization)
        {
            flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
        }

        if (_flags & eShaderCompileFlags::AvoidFlowControl)
        {
            flags |= D3DCOMPILE_AVOID_FLOW_CONTROL;
        }

        if (_flags & eShaderCompileFlags::WarningsAsErrors)
        {
            flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
        }

        return flags;
    }

    [[nodiscard]] StringView ShaderModelNameFrom(
        const eShader _stage)
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
                JAM_UNREACHABLE;
        }
    }
}   // namespace

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ShaderCompiler ShaderCompiler::CompileHLSLFile(
    const StringView          _path,
    const ShaderCompilerInfo& _desc)
{
    ShaderCompiler compiler = {};
    compiler.CompileFromHLSLFile_(_path, _desc);
    return compiler;
}

ShaderCompiler ShaderCompiler::CompileHLSLSource(
    const StringView          _source,
    const ShaderCompilerInfo& _desc)
{
    ShaderCompiler compiler = {};
    compiler.CompileHLSLSource_(_source, _desc);
    return compiler;
}

MemoryView ShaderCompiler::GetBytecode() const
{
    JAM_ASSERT(m_pBlob, "Shader blob is null.");
    return { m_pBlob->GetBufferPointer(), static_cast<Int32>(m_pBlob->GetBufferSize()) };
}

StringView ShaderCompiler::GetErrorMessage() const
{
    return m_errorMsg;
}

Error ShaderCompiler::GetError() const
{
    return m_error;
}

bool ShaderCompiler::IsSucceeded() const
{
    return m_pBlob != nullptr;
}

bool ShaderCompiler::IsFailed() const
{
    return !IsSucceeded();
}

void ShaderCompiler::CompileFromHLSLFile_(
    const StringView          _path,
    const ShaderCompilerInfo& _desc)
{
    JAM_ASSERT(!_path.IsEmpty(), "File path must not be empty.");
    JAM_ASSERT(m_pBlob == nullptr, "ShaderCompiler instance is already used for compilation.");

    // macros
    D3D_SHADER_MACRO* pMacro = nullptr;
    if (!_desc.macros.empty())
    {
        Int32      numMacros    = static_cast<Int32>(_desc.macros.size());
        const bool bHasNullTerm = !_desc.macros.back().IsNull();

        if (bHasNullTerm)
        {
            ++numMacros;   // add null terminator
        }

        pMacro = static_cast<D3D_SHADER_MACRO*>(JAM_STACK_ALLOC(sizeof(D3D_SHADER_MACRO) * numMacros));
        for (SizeT i = 0; i < _desc.macros.size(); ++i)
        {
            pMacro[i].Name       = _desc.macros[i].name.GetPtr();
            pMacro[i].Definition = _desc.macros[i].value.GetPtr();
        }

        if (bHasNullTerm)
        {
            pMacro[numMacros - 1].Name       = nullptr;
            pMacro[numMacros - 1].Definition = nullptr;
        }
    }

    // compile
    const WString    path       = WString::Encoded(_path);
    ComPtr<ID3DBlob> pErrorBlob = nullptr;
    const HRESULT    hr         = D3DCompileFromFile(
        path.GetPtr(),
        pMacro,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        _desc.entryPoint.GetPtr(),
        ShaderModelNameFrom(_desc.stage).GetPtr(),
        CompileFlagsFrom_(_desc.flags),
        0,
        m_pBlob.ReleaseAndGetAddressOf(),
        pErrorBlob.ReleaseAndGetAddressOf());

    // check error
    if (FAILED(hr) || !m_pBlob)
    {
        if (pErrorBlob)
        {
            m_errorMsg.Format("shader compilation failed: {}", static_cast<const char*>(pErrorBlob->GetBufferPointer()));
        }
        else
        {
            m_errorMsg.Format("shader compilation failed: {}", CreateWindowsErrorMessage(hr, eWindowsErrorCode::HRESULT));
        }

        JAM_LOG_ERROR(m_errorMsg);
        m_error = Error::ShaderCompileFailed;
    }

    m_error = Error::None;
}

void ShaderCompiler::CompileHLSLSource_(
    const StringView          _source,
    const ShaderCompilerInfo& _desc)
{
    JAM_ASSERT(!_source.IsEmpty(), "Shader source must not be empty.");

    // macros
    D3D_SHADER_MACRO* pMacro = nullptr;
    if (!_desc.macros.empty())
    {
        Int32      numMacros    = static_cast<Int32>(_desc.macros.size());
        const bool bHasNullTerm = !_desc.macros.back().IsNull();

        if (bHasNullTerm)
        {
            ++numMacros;   // add null terminator
        }

        pMacro = static_cast<D3D_SHADER_MACRO*>(JAM_STACK_ALLOC(sizeof(D3D_SHADER_MACRO) * numMacros));
        for (SizeT i = 0; i < _desc.macros.size(); ++i)
        {
            pMacro[i].Name       = _desc.macros[i].name.GetPtr();
            pMacro[i].Definition = _desc.macros[i].value.GetPtr();
        }

        if (bHasNullTerm)
        {
            pMacro[numMacros - 1].Name       = nullptr;
            pMacro[numMacros - 1].Definition = nullptr;
        }
    }

    // compile
    ComPtr<ID3DBlob> pErrorBlob = nullptr;
    const HRESULT    hr         = D3DCompile(
        _source.GetPtr(),
        _source.GetLength(),
        nullptr,
        pMacro,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        _desc.entryPoint.GetPtr(),
        ShaderModelNameFrom(_desc.stage).GetPtr(),
        CompileFlagsFrom_(_desc.flags),
        0,
        m_pBlob.ReleaseAndGetAddressOf(),
        pErrorBlob.ReleaseAndGetAddressOf());

    // check error
    if (FAILED(hr) || !m_pBlob)
    {
        if (pErrorBlob)
        {
            m_errorMsg.Format("shader compilation failed: {}", static_cast<const char*>(pErrorBlob->GetBufferPointer()));
        }
        else
        {
            m_errorMsg.Format("shader compilation failed: {}", CreateWindowsErrorMessage(hr, eWindowsErrorCode::HRESULT));
        }

        JAM_LOG_ERROR(m_errorMsg);
        m_error = Error::ShaderCompileFailed;
    }

    m_error = Error::None;
}

}   // namespace Jam