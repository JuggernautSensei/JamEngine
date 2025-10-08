#include "pch.h"

#include "ShaderImporter.h"

#include "ShaderCompiler.h"
#include "StringUtilities.h"

namespace jam
{

eErrorCode ShaderImporter::Import()
{
    JAM_ASSERT(!m_filePath.empty(), "File path is empty.");
    JAM_ASSERT(IsImportablePath(m_filePath), "File path is not importable. File path: '{}'", m_filePath.native());
    JAM_ASSERT(m_type != eType::Unknown, "Shader type is unknown. File path: '{}'", m_filepath.native());
    if (m_type == eType::HLSL)
    {
        JAM_ASSERT(m_stage != eShader::Unknown, "Shader stage is not set for HLSL shader. File path: '{}'", m_filepath.native());
        JAM_ASSERT(!m_entryPoint.empty(), "Entry point is not set for HLSL shader. File path: '{}'", m_filepath.native());
    }

    // 임포트
    eErrorCode result = eErrorCode::UnknownError;
    switch (m_type)
    {
        case eType::HLSL:
        {
            // 매크로 설정
            std::vector<ShaderMacro> macros;
            if (!m_macros.empty())
            {
                bool bNeedNullTerminator = !m_macros.empty() && !m_macros.back().IsNullTerminator();

                Int32 size = m_macros.size();
                if (bNeedNullTerminator)
                    size += 1;

                macros.reserve(size);
                for (const Macro& macro: m_macros)
                    macros.push_back({ macro.name.c_str(), macro.value.c_str() });

                if (bNeedNullTerminator)
                    macros.push_back(ShaderMacro::NullTerminator());
            }

            result = m_compiler.LoadHLSLFromFile(m_filePath, m_stage, m_entryPoint, m_flags, macros);
            break;
        }

        case eType::CSO:
        {
            result = m_compiler.LoadCSOFromFile(m_filePath);
            break;
        }
    }

    return result;
}

MemoryView ShaderImporter::GetCompiledByteCode() const
{
    JAM_ASSERT(m_compiler.IsValid(), "Shader is not compiled yet.");
    return m_compiler.GetBytecode();
}

void ShaderImporter::SetPath(const fs::path& _filePath)
{
    m_filePath = _filePath;
    m_type     = DetermineTypeFromExtension_(_filePath);
}

void ShaderImporter::AddMacro(const std::string_view _name, const std::string_view _value)
{
    m_macros.push_back({ std::string(_name), std::string(_value) });
}

void ShaderImporter::SetShaderStage(const eShader _stage)
{
    JAM_ASSERT(_stage != eShader::Unknown, "Shader stage cannot be Unknown.");
    m_stage = _stage;
}

void ShaderImporter::SetEntryPoint(const std::string_view _entryPoint)
{
    JAM_ASSERT(!_entryPoint.empty(), "Entry point cannot be empty.");
    m_entryPoint = _entryPoint;
}

void ShaderImporter::SetCompileFlags(const eShaderCompileFlags _flags)
{
    m_flags = _flags;
}

void ShaderImporter::Reset()
{
    m_filePath.clear();
    m_macros.clear();
    m_entryPoint.clear();
    m_compiler.Reset();
    m_type  = eType::Unknown;
    m_stage = eShader::Unknown;
    m_flags = eShaderCompileFlags_None;
}

bool ShaderImporter::IsImportablePath(const fs::path& _filePath) const
{
    eType type = DetermineTypeFromExtension_(_filePath);
    return type != eType::Unknown;
}

ShaderImporter::eType ShaderImporter::DetermineTypeFromExtension_(
    const fs::path& _filePath) const
{
    fs::path    ext      = _filePath.extension();
    std::string lowerExt = ToLower(ext.native());
    switch (HashOf(lowerExt))
    {
        case HashOf(".hlsl"):
            return eType::HLSL;
        case HashOf(".cso"):
            return eType::CSO;
        default:
            return eType::Unknown;
    }
}

}   // namespace jam