#pragma once
#include "GfxBase.h"
#include "Memory.h"

namespace jam
{

// 셰이더 매크로 정의
struct ShaderMacro
{
    JAM_NODISCARD constexpr static ShaderMacro NullTerminator() { return { nullptr, nullptr }; }
    JAM_NODISCARD bool                         IsNullTerminator() const { return name == nullptr && value == nullptr; }

    const char* name;    // 매크로 이름
    const char* value;   // 매크로 값
};

// 셰이더 컴파일러
class ShaderCompiler
{
public:
    ShaderCompiler()  = default;
    ~ShaderCompiler() = default;

    ShaderCompiler(const ShaderCompiler&)            = delete;
    ShaderCompiler& operator=(const ShaderCompiler&) = delete;
    ShaderCompiler(ShaderCompiler&&)                 = default;
    ShaderCompiler& operator=(ShaderCompiler&&)      = default;

    /// @brief HLSL 파일로부터 셰이더 컴파일
    eErrorCode CompileFromFile(
        const fs::path&              _path,
        eShader                      _stage,
        std::string_view             _entryPoint,
        eShaderCompileFlags          _flags  = eShaderCompileFlags_Default,
        std::span<const ShaderMacro> _macros = {});

    /// @brief HLSL 소스 코드로부터 셰이더 컴파일
    eErrorCode CompileFromSource(
        std::string_view             _hlslSource,
        eShader                      _stage,
        std::string_view             _entryPoint,
        eShaderCompileFlags          _flags  = eShaderCompileFlags_Default,
        std::span<const ShaderMacro> _macros = {});

    /// @brief 컴파일된 셰이더 객체(.cso) 파일로부터 로드
    eErrorCode LoadFromFile(const fs::path& _filePath);

    /// @brief 메모리로부터 컴파일된 셰이더 객체 로드
    eErrorCode LoadFromMemory(MemoryView _byteCode);

    /// @brief 컴파일된 셰이더 바이트코드 반환
    JAM_NODISCARD MemoryView GetBytecode() const
    {
        JAM_ASSERT(IsValid(), "Shader bytecode is not available.");
        return MemoryView { m_pBlob->GetBufferPointer(), m_pBlob->GetBufferSize() };
    }

    /// @brief 셰이더 유효성 확인
    JAM_NODISCARD bool IsValid() const
    {
        return m_pBlob != nullptr;
    }

    /// @brief 셰이더 초기화
    void Clear()
    {
        m_pBlob.Reset();
    }

private:
    void HandleCompileError_(const ComPtr<ID3DBlob>& _pErrorBlob, HRESULT _hr) const;

    ComPtr<ID3DBlob> m_pBlob = nullptr;
};

}   // namespace jam
