#pragma once
#include "GfxBase.h"
#include "Memory.h"

namespace jam
{

struct ShaderMacro
{
    JAM_NODISCARD constexpr static ShaderMacro NullTerminator() { return { nullptr, nullptr }; }
    JAM_NODISCARD bool                         IsNullTerminator() const { return name == nullptr && value == nullptr; }

    const char* name;    // 매크로 이름
    const char* value;   // 매크로 값
};

class ShaderCompiler
{
public:
    /// @brief HLSL 파일로부터 셰이더 컴파일
    /// @param _filePath HLSL 파일 경로
    /// @param _stage 셰이더 스테이지
    /// @param _entryPoint 셰이더 진입점 함수 이름
    /// @param _flags 컴파일 플래그
    /// @param _macrosOrEmpty 셰이더 매크로 정의 (선택 사항, 마지막은 반드시 null terminator로 끝나야 함)
    /// @return 컴파일 성공 여부 (true: 성공, false: 실패)
    eErrorCode LoadHLSLFromFile(
        const fs::path&              _filePath,
        eShader                      _stage,
        std::string_view             _entryPoint,
        eShaderCompileFlags          _flags         = eShaderCompileFlags_Default,
        std::span<const ShaderMacro> _macrosOrEmpty = {});

    /// @brief HLSL 소스 코드로부터 셰이더 컴파일
    /// @param _hlslSource HLSL 소스 코드
    /// @param _stage 셰이더 스테이지
    /// @param _entryPoint 셰이더 진입점 함수 이름
    /// @param _flags 컴파일 플래그
    /// @param _macrosOrEmpty 셰이더 매크로 정의 (선택 사항, 마지막은 반드시 null terminator로 끝나야 함)
    /// @return 컴파일 성공 여부 (true: 성공, false: 실패)
    eErrorCode LoadHLSL(
        std::string_view             _hlslSource,
        eShader                      _stage,
        std::string_view             _entryPoint,
        eShaderCompileFlags          _flags         = eShaderCompileFlags_Default,
        std::span<const ShaderMacro> _macrosOrEmpty = {});

    /// @brief 컴파일된 셰이더 객체(.cso) 파일로부터 셰이더 로드
    /// @param _filePath .cso 파일 경로
    /// @return 로드 성공 여부 (true: 성공, false: 실패)
    eErrorCode LoadCSOFromFile(
        const fs::path& _filePath);

    /// @brief 메모리 뷰로부터 컴파일된 셰이더 객체(.cso) 로드
    /// @param _byteCode 셰이더 코드를 담은 메모리 뷰
    /// @return 로드 성공 여부 (true: 성공, false: 실패)
    eErrorCode LoadCSO(
        MemoryView _byteCode);

    /// @brief 컴파일된 셰이더 바이트코드 반환
    /// @return 셰이더 바이트코드
    JAM_NODISCARD MemoryView GetBytecode() const
    {
        JAM_ASSERT(IsValid(), "Shader is not compiled.");
        return MemoryView { m_pBlob->GetBufferPointer(), m_pBlob->GetBufferSize() };
    }

    JAM_NODISCARD bool IsValid() const
    {
        return m_pBlob != nullptr;
    }

    void Reset()
    {
        m_pBlob.Reset();
    }

private:
    ComPtr<ID3DBlob> m_pBlob = nullptr;
};

}   // namespace jam