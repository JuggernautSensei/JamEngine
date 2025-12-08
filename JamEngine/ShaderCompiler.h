#pragma once
#include "GraphicsBase.h"
#include "MemView.h"

namespace jam
{

// 셰이더 매크로 정의
struct ShaderMacro
{
    [[nodiscard]] constexpr static ShaderMacro NullTerminator() { return { nullptr, nullptr }; }
    [[nodiscard]] bool                         IsNullTerminator() const { return name == nullptr && value == nullptr; }

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
    bool CompileFromFile(
        const fs::path&              _path,
        eShader                      _stage,
        std::string_view             _entryPoint,
        eShaderCompileFlags          _flags  = eShaderCompileFlags_Default,
        std::span<const ShaderMacro> _macros = {});

    /// @brief HLSL 소스 코드로부터 셰이더 컴파일
    bool CompileFromSource(
        std::string_view             _hlslSource,
        eShader                      _stage,
        std::string_view             _entryPoint,
        eShaderCompileFlags          _flags  = eShaderCompileFlags_Default,
        std::span<const ShaderMacro> _macros = {});

    /// @brief 컴파일된 셰이더 객체(.cso) 파일로부터 로드
    bool LoadFromFile(const fs::path& _filePath);

    /// @brief 메모리로부터 컴파일된 셰이더 객체 로드
    bool LoadFromMemory(ConstMemView _byteCode);

    /// @brief 컴파일된 셰이더 바이트코드 반환
    [[nodiscard]] ConstMemView GetBytecode() const;

    /// @brief 셰이더 유효성 확인
    [[nodiscard]] bool IsValid() const;

private:
    ComPtr<ID3DBlob> m_pBlob = nullptr;
};

}   // namespace jam
