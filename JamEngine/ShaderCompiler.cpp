#include "pch.h"

#include "ShaderCompiler.h"

#include "WindowsUtilities.h"

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

namespace
{

NODISCARD UINT GetShaderCompileFlags(jam::eShaderCompileOption _option)
{
    constexpr UINT k_debugCompileFlags   = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    constexpr UINT k_releaseCompileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;

    if (_option == jam::eShaderCompileOption::Default)
    {
#ifdef _DEBUG
        _option = jam::eShaderCompileOption::Debug;   // Default to Debug in debug mode
#else
        _option = jam::eShaderCompileOption::Optimized;   // Default to Optimized in release mode
#endif
    }

    switch (_option)
    {
        case jam::eShaderCompileOption::Debug:
            return k_debugCompileFlags;
        case jam::eShaderCompileOption::Optimized:
            return k_releaseCompileFlags;
        default:
            JAM_ERROR("Unknown shader compile option: {}", static_cast<int>(_option));
            return 0;   // Return 0 or handle error appropriately
    }
}

NODISCARD std::vector<D3D_SHADER_MACRO> CreateShaderMacros(const std::span<const jam::ShaderMacro> _macros_orEmpty)
{
    std::vector<D3D_SHADER_MACRO> macros;
    macros.reserve(_macros_orEmpty.size() + 1);   // +1 for the null terminator
    for (const jam::ShaderMacro& macro: _macros_orEmpty)
    {
        macros.emplace_back(macro.name.data(), macro.value.data());
    }
    macros.emplace_back(nullptr, nullptr);   // Null terminator
    return macros;
}

}   // namespace

namespace jam
{

bool ShaderCompiler::CompileHLSLFromFile(const fs::path& _filename, const std::string_view _entryPoint, const std::string_view _target, const std::span<const ShaderMacro> _macros_orEmpty, const eShaderCompileOption _compileOption)
{
    // create macro
    std::vector<D3D_SHADER_MACRO> macros        = CreateShaderMacros(_macros_orEmpty);
    D3D_SHADER_MACRO*             pMacro_orNull = macros.empty() ? nullptr : macros.data();

    ComPtr<ID3DBlob> errorBlob;
    const HRESULT    hr = D3DCompileFromFile(
        _filename.c_str(),
        pMacro_orNull,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        _entryPoint.data(),
        _target.data(),
        GetShaderCompileFlags(_compileOption),
        0,
        m_pCompiled.GetAddressOf(),
        errorBlob.GetAddressOf());

    if (FAILED(hr))
    {
        std::string errorMsg = "Failed to compile HLSL shader file.";
        if (errorBlob)
        {
            errorMsg = std::format("{}: {}\nCompile Error: {}\n{}", _filename.string(), errorMsg, static_cast<const char*>(errorBlob->GetBufferPointer()), GetSystemErrorMessage(hr));
        }
        JAM_ERROR("{}", errorMsg);
        return false;
    }

    return true;
}

bool ShaderCompiler::CompileHLSL(std::string_view _pSource, const std::string_view _entryPoint, const std::string_view _target, const std::span<const ShaderMacro> _macros_orEmpty, const eShaderCompileOption _compileOption)
{
    // create macro
    std::vector<D3D_SHADER_MACRO> macros        = CreateShaderMacros(_macros_orEmpty);
    D3D_SHADER_MACRO*             pMacro_orNull = macros.empty() ? nullptr : macros.data();

    ComPtr<ID3DBlob> errorBlob;
    const HRESULT    hr = D3DCompile(
        _pSource.data(),
        _pSource.size(),
        nullptr,   // source name
        pMacro_orNull,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        _entryPoint.data(),
        _target.data(),
        GetShaderCompileFlags(_compileOption),
        0,
        m_pCompiled.GetAddressOf(),
        errorBlob.GetAddressOf());

    if (FAILED(hr))
    {
        std::string errorMsg = "Failed to compile HLSL shader source.";
        if (errorBlob)
        {
            errorMsg = std::format("{}: {}\nCompile Error: {}\n{}", _pSource, errorMsg, errorBlob->GetBufferPointer(), GetSystemLastErrorMessage());
        }
        JAM_ERROR("{}", errorMsg);
        return false;
    }

    return true;
}

bool ShaderCompiler::LoadCSOFromFile(const fs::path& _filename)
{
    const HRESULT hr = D3DReadFileToBlob(_filename.c_str(), m_pCompiled.GetAddressOf());
    if (FAILED(hr))
    {
        JAM_ERROR("Failed to load compiled shader from '{}'. HRESULT: {}", _filename.string(), GetSystemErrorMessage(hr));
        return false;
    }
    return true;
}

bool ShaderCompiler::LoadCSO(const void* _pSource, const size_t _sourceSize)
{
    if (!_pSource || _sourceSize == 0)
    {
        JAM_ERROR("Invalid shader source provided. Source pointer is null or size is zero.");
        return false;
    }

    const HRESULT hr = D3DCreateBlob(_sourceSize, m_pCompiled.GetAddressOf());
    if (FAILED(hr))
    {
        JAM_ERROR("Failed to create blob for compiled shader. HRESULT: {}", GetSystemErrorMessage(hr));
        return false;
    }

    memcpy(m_pCompiled->GetBufferPointer(), _pSource, _sourceSize);
    return true;
}

void ShaderCompiler::GetCompiledShader(ID3DBlob** _out_ppBlob) const
{
    JAM_ASSERT(_out_ppBlob, "Output pointer for compiled shader blob cannot be null.");
    JAM_ASSERT(m_pCompiled, "No compiled shader available. Ensure CompileHLSLFromFile, CompileHLSL, LoadCSOFromFile, or CompileCSO has been called successfully before calling GetCompiledShader.");
    HRESULT hr = m_pCompiled.CopyTo(_out_ppBlob);
    JAM_ASSERT(SUCCEEDED(hr), "Failed to get compiled shader blob. HRESULT: {}", GetSystemErrorMessage(hr));
}

bool ShaderCompiler::SaveCompiledShader(const fs::path& _filename) const
{
    if (!m_pCompiled)
    {
        JAM_ERROR("No compiled shader to save.");
        return false;
    }
    const HRESULT hr = D3DWriteBlobToFile(m_pCompiled.Get(), _filename.c_str(), TRUE);
    if (FAILED(hr))
    {
        JAM_ERROR("Failed to save compiled shader to '{}'. HRESULT: {}", _filename.string(), GetSystemErrorMessage(hr));
        return false;
    }
    return true;
}

}   // namespace jam