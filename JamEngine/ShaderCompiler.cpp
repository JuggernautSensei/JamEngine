#include "pch.h"

#include "ShaderCompiler.h"

#include <d3dcompiler.h>

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

    [[nodiscard]] Int32 CalcMacroArrayAllocSize_(
        const Span<const ShaderMacro> _macros)
    {
        Int32 numMacros = static_cast<Int32>(_macros.size());
        if (!_macros.empty() && !_macros.back().IsNull())
        {
            ++numMacros;   // add null terminator
        }
        return static_cast<Int32>(sizeof(D3D_SHADER_MACRO)) * numMacros;
    }

    void BuildMacroArray_(
        const ShaderCompilerInfo& _desc,
        D3D_SHADER_MACRO*         _pBuffer)
    {
        if (_desc.macros.empty())
        {
            return;
        }

        SizeT numMacros = 0;
        for (; numMacros < _desc.macros.size(); ++numMacros)
        {
            _pBuffer[numMacros].Name       = _desc.macros[numMacros].name.GetPtr();
            _pBuffer[numMacros].Definition = _desc.macros[numMacros].value.GetPtr();
        }

        if (!_desc.macros.back().IsNull())
        {
            _pBuffer[numMacros].Name       = nullptr;
            _pBuffer[numMacros].Definition = nullptr;
        }
    }

}   // namespace

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Expect<ShaderCompiler> ShaderCompiler::CompileHLSLFile(
    const StringView          _path,
    const ShaderCompilerInfo& _desc)
{
    ShaderCompiler compiler = {};
    if (const Error err = compiler.CompileFromHLSLFile_(_path, _desc))
    {
        return err;
    }
    return compiler;
}

Expect<ShaderCompiler> ShaderCompiler::CompileHLSLSource(
    const StringView          _source,
    const ShaderCompilerInfo& _desc)
{
    ShaderCompiler compiler = {};
    if (const Error err = compiler.CompileHLSLSource_(_source, _desc))
    {
        return err;
    }
    return compiler;
}

MemoryView ShaderCompiler::GetBytecode() const
{
    JAM_ASSERT(m_pBlob, "Shader blob is null.");
    return { m_pBlob->GetBufferPointer(), static_cast<SizeT>(m_pBlob->GetBufferSize()) };
}

Error ShaderCompiler::CompileFromHLSLFile_(
    const StringView          _path,
    const ShaderCompilerInfo& _desc)
{
    JAM_ASSERT(!_path.IsEmpty(), "File path must not be empty.");
    JAM_ASSERT(m_pBlob == nullptr, "ShaderCompiler instance is already used for compilation.");

    // macros
    D3D_SHADER_MACRO* pMacro = nullptr;
    if (!_desc.macros.empty())
    {
        const Int32 byteWidth = CalcMacroArrayAllocSize_(_desc.macros);
        JAM_ASSERT(byteWidth > 0, "Number of macros must be greater than zero.");
        pMacro = static_cast<D3D_SHADER_MACRO*>(JAM_STACK_ALLOC(byteWidth));
        BuildMacroArray_(_desc, pMacro);
    }

    // compile
    const WString    path       = WString::Encoded(_path);
    Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;
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
        String msg = String::Formatted("shader \"{}\" compilation failed: ", _path);

        if (pErrorBlob)
        {
            msg.Append(static_cast<const char*>(pErrorBlob->GetBufferPointer()));
        }
        else
        {
            msg.Append(MakeWindowsErrorMessage(hr, eWindowsErrorCode::HRESULT));
        }

        JAM_LOG_ERROR(msg);
        return Error::ShaderCompileFailed;
    }

    return Error::None;
}

Error ShaderCompiler::CompileHLSLSource_(
    const StringView          _source,
    const ShaderCompilerInfo& _desc)
{
    JAM_ASSERT(!_source.IsEmpty(), "Shader source must not be empty.");

    // macros
    D3D_SHADER_MACRO* pMacro = nullptr;
    if (!_desc.macros.empty())
    {
        const Int32 byteWidth = CalcMacroArrayAllocSize_(_desc.macros);
        JAM_ASSERT(byteWidth > 0, "Number of macros must be greater than zero.");
        pMacro = static_cast<D3D_SHADER_MACRO*>(JAM_STACK_ALLOC(byteWidth));
        BuildMacroArray_(_desc, pMacro);
    }

    // compile
    Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;
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
        String str = "shader source compilation failed: ";
        if (pErrorBlob)
        {
            str.Append(static_cast<const char*>(pErrorBlob->GetBufferPointer()));
        }
        else
        {
            str.Append(MakeWindowsErrorMessage(hr, eWindowsErrorCode::HRESULT));
        }

        JAM_LOG_ERROR(str);
        return Error::ShaderCompileFailed;
    }

    return Error::None;
}

}   // namespace Jam