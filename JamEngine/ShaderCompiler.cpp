#include "pch.h"

#include "ShaderCompiler.h"

#include <d3dcompiler.h>
#include "WindowsUtilities.h"
#pragma comment(lib, "d3dcompiler.lib")

namespace Jam
{

namespace
{

    using D3DShaderMacroBuffer = Array<D3D_SHADER_MACRO, 128>;

    void BuildD3D11ShaderMacroBuffer_(
        D3DShaderMacroBuffer&         _outBuffer,
        const Span<const ShaderMacro> _macrosOrEmpty)   // return number of macros written
    {
        JAM_ASSERT(_macrosOrEmpty.size() < 127, "Too many shader macros specified.");

        // fill d3d macro
        const SizeT numMacros = _macrosOrEmpty.size();
        for (SizeT i = 0; i < numMacros; ++i)
        {
            const ShaderMacro& srcMacro = _macrosOrEmpty[i];
            _outBuffer[i]               = { srcMacro.name.GetPtr(), srcMacro.value.GetPtr() };
        }

        // Ensure null-termination
        if (_macrosOrEmpty.empty() || _macrosOrEmpty.back().IsNull() == false)
        {
            _outBuffer[numMacros] = { nullptr, nullptr };
        }
    }

    [[nodiscard]] UINT CompileFlagsFrom_(
        const Flags<eShaderCompileFlags> _flags)
    {
        UINT flags = 0;

        if (_flags & eShaderCompileFlags::Debug)
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

Expect<ShaderCompiler> ShaderCompiler::CompileFromHLSLFile(
    const Path&               _path,
    const ShaderCompilerInfo& _info)
{
    ShaderCompiler compiler = {};
    if (const Error err = compiler.CompileFromHLSLFile_(_path, _info))
    {
        return err;
    }

    return compiler;
}

Expect<ShaderCompiler> ShaderCompiler::CompileFromHLSLSource(
    const StringView          _source,
    const ShaderCompilerInfo& _info)
{
    ShaderCompiler compiler = {};
    if (const Error err = compiler.CompileHLSLSource_(_source, _info))
    {
        return err;
    }

    return compiler;
}

CMemoryBlock ShaderCompiler::GetBytecode() const
{
    JAM_ASSERT(m_pBlob, "Shader blob is null.");
    return { m_pBlob->GetBufferPointer(), NumericCast<Int32>(m_pBlob->GetBufferSize()) };
}

Error ShaderCompiler::CompileFromHLSLFile_(
    const Path&               _path,
    const ShaderCompilerInfo& _desc)
{
    JAM_ASSERT(!_path.empty(), "File path must not be empty.");

    D3DShaderMacroBuffer macroBuf;
    BuildD3D11ShaderMacroBuffer_(macroBuf, _desc.macros);

    ScratchWBuffer pathBuf;
    pathBuf.SetUtf8(_path.native());

    ComPtr<ID3DBlob> pErrorBlob = nullptr;
    const HRESULT    hr         = D3DCompileFromFile(
        pathBuf.GetPtr(),
        macroBuf.data(),
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
            JAM_LOG.Error("shader compilation failed: {}", static_cast<const char*>(pErrorBlob->GetBufferPointer()));
        }
        else
        {
            ScratchBuffer buf;
            FormatHRESULTMessage(buf, hr);
            JAM_LOG.Error("shader compilation failed: {}", buf.ToStringView());
        }

        return Error::ShaderCompileFailed;
    }

    return Error::None;
}

Error ShaderCompiler::CompileHLSLSource_(
    const StringView          _source,
    const ShaderCompilerInfo& _desc)
{
    JAM_ASSERT(!_source.IsEmpty(), "Shader source must not be empty.");

    D3DShaderMacroBuffer macroBuf;
    BuildD3D11ShaderMacroBuffer_(macroBuf, _desc.macros);

    ComPtr<ID3DBlob> pErrorBlob = nullptr;
    const HRESULT    hr         = D3DCompile(
        _source.GetPtr(),
        _source.GetLength(),
        nullptr,
        macroBuf.data(),
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
            JAM_LOG.Error("shader compilation failed: {}", static_cast<const char*>(pErrorBlob->GetBufferPointer()));
        }
        else
        {
            ScratchBuffer buf;
            FormatHRESULTMessage(buf, hr);
            JAM_LOG.Error("shader compilation failed: {}", buf.ToStringView());
        }

        return Error::ShaderCompileFailed;
    }

    return Error::None;
}

}   // namespace Jam