#include "pch.h"

#include "ShaderCompiler.h"

#include "WindowsUtilities.h"

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

namespace jam
{

UINT ShaderCompiler::k_debugCompileFlags   = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
UINT ShaderCompiler::k_releaseCompileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;

bool ShaderCompiler::CompileHLSLFromFile(const fs::path& _filename, const std::string_view _entryPoint, const std::string_view _target, const D3D_SHADER_MACRO* _macros, const bool _bDebug)
{
    ComPtr<ID3DBlob> errorBlob;

    const HRESULT hr = D3DCompileFromFile(
        _filename.c_str(),
        _macros,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        _entryPoint.data(),
        _target.data(),
        _bDebug ? k_debugCompileFlags : k_releaseCompileFlags,
        0,
        m_pCompiled.GetAddressOf(),
        errorBlob.GetAddressOf());

    if (FAILED(hr))
    {
        std::string errorMsg = "Failed to compile HLSL shader file.";
        if (errorBlob)
        {
            errorMsg = std::format("{}: {}\nCompile Error: {}\n{}", _filename.string(), errorMsg, errorBlob->GetBufferPointer(), GetSystemLastErrorMessage());
        }
        JAM_ERROR("{}", errorMsg);
        return false;
    }

    return true;
}

bool ShaderCompiler::CompileHLSL(std::string_view _pSource, const std::string_view _entryPoint, const std::string_view _target, const D3D_SHADER_MACRO* _macros, const bool _bDebug)
{
    ComPtr<ID3DBlob> errorBlob;

    const HRESULT hr = D3DCompile(
        _pSource.data(),
        _pSource.size(),
        nullptr,   // source name
        _macros,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        _entryPoint.data(),
        _target.data(),
        _bDebug ? k_debugCompileFlags : k_releaseCompileFlags,
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

bool ShaderCompiler::CompileCSOFromFile(const fs::path& _filename)
{
    const HRESULT hr = D3DReadFileToBlob(_filename.c_str(), m_pCompiled.GetAddressOf());
    if (FAILED(hr))
    {
        JAM_ERROR("Failed to load compiled shader from '{}'. HRESULT: {}", _filename.string(), GetSystemErrorMessage(hr));
        return false;
    }
    return true;
}

bool ShaderCompiler::CompileCSO(const std::string_view _pSource)
{
    if (_pSource.empty())
    {
        JAM_ERROR("Empty shader source provided.");
        return false;
    }

    const HRESULT hr = D3DCreateBlob(_pSource.size(), m_pCompiled.GetAddressOf());
    if (FAILED(hr))
    {
        JAM_ERROR("Failed to create blob for compiled shader. HRESULT: {}", GetSystemErrorMessage(hr));
        return false;
    }

    memcpy(m_pCompiled->GetBufferPointer(), _pSource.data(), _pSource.size());
    return true;
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