#pragma once
#include "GfxBase.h"
#include "IImporter.h"
#include "ShaderCompiler.h"
namespace jam
{

class ShaderImporter : public IImporter
{
    struct Macro
    {
        JAM_NODISCARD bool IsNullTerminator() const
        {
            return name.empty() && value.empty();
        }

        JAM_NODISCARD static Macro NullTerminator()
        {
            return Macro {};
        }

        std::string name;
        std::string value;
    };

    enum class eType : Int8
    {
        Unknown = -1,

        HLSL = 0,
        CSO
    };

public:
    ShaderImporter()           = default;
    ~ShaderImporter() override = default;

    ShaderImporter(const ShaderImporter&)                = default;
    ShaderImporter& operator=(const ShaderImporter&)     = default;
    ShaderImporter(ShaderImporter&&) noexcept            = default;
    ShaderImporter& operator=(ShaderImporter&&) noexcept = default;

    eErrorCode               Import() override;
    JAM_NODISCARD MemoryView GetCompiledByteCode() const;

    void Reset() override;
    void SetPath(const fs::path& _filePath) override;
    void AddMacro(std::string_view _name, std::string_view _value);
    void SetShaderStage(eShader _stage);
    void SetEntryPoint(std::string_view _entryPoint);
    void SetCompileFlags(eShaderCompileFlags _flags);

private:
    JAM_NODISCARD eType DetermineTypeFromExtension_(const fs::path& _filePath) const;

    fs::path            m_filePath   = {};
    eType               m_type       = eType::Unknown;
    std::vector<Macro>  m_macros     = {};
    eShader             m_stage      = eShader::Unknown;
    std::string         m_entryPoint = "main";
    eShaderCompileFlags m_flags      = eShaderCompileFlags_None;
    ShaderCompiler      m_compiler   = {};
};

}   // namespace jam