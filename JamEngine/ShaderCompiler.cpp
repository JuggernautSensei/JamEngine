#include "pch.h"

#include "ShaderCompiler.h"

#include "WindowsUtilities.h"

#include <d3dcompiler.h>
#include <fstream>
#pragma comment(lib, "d3dcompiler.lib")
using namespace jam;

namespace
{

constexpr const char* k_shaderModelTable[EnumCount<eShader>()] = {
    "vs_5_0",
    "ps_5_0",
    "gs_5_0",
    "hs_5_0",
    "ds_5_0",
};

}

namespace jam
{

eErrorCode ShaderCompiler::LoadHLSLFromFile(
    const fs::path&                    _filePath,
    const eShader                      _stage,
    const std::string_view             _entryPoint,
    const eShaderCompileFlags          _flags,
    const std::span<const ShaderMacro> _macrosOrEmpty)
{
    // file open
    std::ifstream file { _filePath, std::ios::in | std::ios::binary };
    if (!file.is_open())
    {
        SetErrorString(R"(Failed to open shader file: "{}")", _filePath.native());
        return eErrorCode::FileOpenFailed;
    }

    // file size
    std::istream& eof  = file.seekg(0, std::ios::end);
    size_t        size = eof.tellg();
    file.seekg(0, std::ios::beg);

    // file read
    std::string buffer;
    buffer.resize(size);
    file.read(buffer.data(), size);
    if (file.bad() || file.fail())
    {
        SetErrorString(R"(Failed to read shader file: "{}")", _filePath.native());
        return eErrorCode::FileReadFailed;
    }
    return LoadHLSL(buffer, _stage, _entryPoint, _flags, _macrosOrEmpty);
}

eErrorCode ShaderCompiler::LoadHLSL(
    const std::string_view             _hlslSource,
    const eShader                      _stage,
    const std::string_view             _entryPoint,
    const eShaderCompileFlags          _flags,
    const std::span<const ShaderMacro> _macrosOrEmpty)
{
    JAM_ASSERT(_stage != eShader::Unknown, "Invalid shader stage.");
    JAM_ASSERT(!_hlslSource.empty(), "HLSL source is empty.");
    JAM_ASSERT(!_entryPoint.empty(), "Entry point is empty.");
    JAM_ASSERT(_macrosOrEmpty.empty() || _macrosOrEmpty.back().IsNullTerminator(), "Shader macros must be null-terminated.");

    Reset();

    // 컴파일 플래그 변환
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

    // 매크로 변환
    const D3D_SHADER_MACRO* macros = nullptr;
    if (!_macrosOrEmpty.empty())
        macros = reinterpret_cast<const D3D_SHADER_MACRO*>(_macrosOrEmpty.data());

    // 컴파일
    ComPtr<ID3DBlob> pErrorBlob;
    HRESULT          hr = D3DCompile(
        _hlslSource.data(),
        _hlslSource.size(),
        nullptr,
        macros,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        _entryPoint.data(),
        k_shaderModelTable[EnumToInt(_stage)],
        compileFlags,
        0,
        m_pBlob.ReleaseAndGetAddressOf(),
        pErrorBlob.ReleaseAndGetAddressOf());

    // 오류 처리
    if (FAILED(hr) || m_pBlob == nullptr)
    {
        JAM_LOG_DEBUG("Shader compilation failed with error: {}", CreateComErrorMessage(hr));
        if (pErrorBlob)
        {
            std::string_view errorStr { static_cast<const char*>(pErrorBlob->GetBufferPointer()), pErrorBlob->GetBufferSize() };
            SetErrorString("Shader compilation failed: {}", errorStr);
        }
        else
        {
            SetErrorString("Shader compilation failed with unknown error.");
        }

        return eErrorCode::ShaderCompileFailed;
    }

    return eErrorCode::Ok;
}

eErrorCode ShaderCompiler::LoadCSO(const MemoryView& _byteCode)
{
    Reset();
    JAM_CHECK_COM(D3DCreateBlob(_byteCode.GetByteWidth(), m_pBlob.ReleaseAndGetAddressOf()));
    std::memcpy(m_pBlob->GetBufferPointer(), _byteCode.GetDataPtr(), _byteCode.GetByteWidth());
    return eErrorCode::Ok;
}

eErrorCode ShaderCompiler::LoadCSOFromFile(
    const fs::path& _filePath)
{
    // file open
    std::ifstream file { _filePath, std::ios::in | std::ios::binary };
    if (!file.is_open())
    {
        SetErrorString(R"(Failed to open shader file: "{}")", _filePath.native());
        return eErrorCode::FileOpenFailed;
    }

    // file size
    std::istream& eof  = file.seekg(0, std::ios::end);
    size_t        size = eof.tellg();
    file.seekg(0, std::ios::beg);

    // file read
    std::string buffer;
    buffer.resize(size);
    file.read(buffer.data(), size);
    if (file.bad() || file.fail())
    {
        SetErrorString(R"(Failed to read shader file: "{}")", _filePath.native());
        return eErrorCode::FileReadFailed;
    }
    return LoadCSO(MemoryView { buffer.data(), size });
}

}   // namespace jam