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

    [[nodiscard]] Vector<D3D_SHADER_MACRO> CreateD3D11ShaderMacroBuffer_(
        const Span<const ShaderMacro> _macrosOrEmpty)   // return number of macros written
    {
        const bool bIsTerminated = !_macrosOrEmpty.empty() && _macrosOrEmpty.back().IsNull();

        Vector<D3D_SHADER_MACRO> macros = {};
        macros.reserve(_macrosOrEmpty.size() + (bIsTerminated ? 0 : 1));   // reserve one extra slot for null-termination if not already terminated

        for (const ShaderMacro& macro: _macrosOrEmpty)
        {
            macros.emplace_back(macro.name.GetPtr(), macro.value.GetPtr());
        }

        if (!bIsTerminated)
        {
            macros.emplace_back(nullptr, nullptr);   // null-terminate the buffer
        }

        return macros;
    }

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
    return { m_pBlob->GetBufferPointer(), NarrowCast<Int32>(m_pBlob->GetBufferSize()) };
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

    const WString path = WString::Encoded(_path);
    if (!FileSystem::Utf16::Exists(path))
    {
        m_errorMsg.Format("shader file not found: {}", _path);
        JAM_LOG_ERROR(m_errorMsg);
        m_error = Error::NotFound;
        return;
    }

    const Vector<D3D_SHADER_MACRO> macros     = CreateD3D11ShaderMacroBuffer_(_desc.macros);
    ComPtr<ID3DBlob>               pErrorBlob = nullptr;
    const HRESULT                  hr         = D3DCompileFromFile(
        path.GetPtr(),
        macros.data(),
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        _desc.entryPoint.GetPtr(),
        ShaderModelNameFrom(_desc.stage).GetPtr(),
        CompileFlagsFrom_(_desc.flags),
        0,
        m_pBlob.ReleaseAndGetAddressOf(),
        pErrorBlob.ReleaseAndGetAddressOf());

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

    const Vector<D3D_SHADER_MACRO> macros     = CreateD3D11ShaderMacroBuffer_(_desc.macros);
    ComPtr<ID3DBlob>               pErrorBlob = nullptr;
    const HRESULT                  hr         = D3DCompile(
        _source.GetPtr(),
        _source.GetLength(),
        nullptr,
        macros.data(),
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        _desc.entryPoint.GetPtr(),
        ShaderModelNameFrom(_desc.stage).GetPtr(),
        CompileFlagsFrom_(_desc.flags),
        0,
        m_pBlob.ReleaseAndGetAddressOf(),
        pErrorBlob.ReleaseAndGetAddressOf());

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