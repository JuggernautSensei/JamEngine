#pragma once

enum class eProjectExportType
{
    CompiledBinary,
    HLSLLiteral,
};

enum class eProjectExportFlags : UInt32
{
    None          = 0,        // user flags only
    DefaultExport = 1 << 0,   // user flags only
    DebugExport   = 1 << 1,   // user flags + debug defaults
    ReleaseExport = 1 << 2,   // user flags + release defaults
};

enum class eNameFormat
{
    NoSpace,     // Vertex Shader 0 -> VertexShader0
    CamelCase,   // Vertex Shader 0 -> vertexShader0
    SnakeCase,   // Vertex Shader 0 -> vertex_shader_0
    PascalCase   // Vertex Shader 0 -> VertexShader0
};

struct ExportProjectInfo
{
    // export settings
    eProjectExportType         type  = eProjectExportType::CompiledBinary;
    Flags<eProjectExportFlags> flags = eProjectExportFlags::DefaultExport;

    // variable
    eNameFormat nameFormat = eNameFormat::NoSpace;
    String      namePrefix = "";

    // preprocessor macros
    StringView debugMacro    = "DEBUG_OUTPUT";
    StringView releaseMacro  = "RELEASE_OUTPUT";
    StringView rootNamespace = "";
};

class ProjectExporter
{

    enum class eWriteCompiledBinarySection
    {
        Declaration,
        Definition,
    };

public:
    [[nodiscard]] static ProjectExporter Export(const Project* _pProject, StringView _exportDir, StringView _baseName, const ExportProjectInfo& _exportProjectInfo);

    [[nodiscard]] bool       IsSucceeded() const;
    [[nodiscard]] bool       IsFailed() const;
    [[nodiscard]] Error      GetError() const;
    [[nodiscard]] StringView GetErrorMessage() const;

private:
    ProjectExporter() = default;
    void                 Export_(const Project* _pProject, StringView _exportDir, StringView _baseName, const ExportProjectInfo& _exportProjectInfo);
    void                 WriteDeclarations_(const Project* _pProject, const FileWriter& _writer, const ExportProjectInfo& _info) const;
    void                 WriteLiteralDefinitions_(const Project* _pProject, const FileWriter& _writer, const ExportProjectInfo& _info);
    void                 WriteCompiledBinaryDefinitions_(const Project* _pProject, const FileWriter& _writer, const ExportProjectInfo& _info);
    void                 WriteCompiledBinaryOutput_(const Project* _pProject, const FileWriter& _writer, const ExportProjectInfo& _info, eWriteCompiledBinarySection _section);
    void                 AppendErrorState_(Error _error, StringView _msg);
    [[nodiscard]] String MakeVariableName_(StringView _name, StringView _prefix, eNameFormat _format) const;

    String m_errorMsg = {};
    Error  m_error    = Error::None;
};
