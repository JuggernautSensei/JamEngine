#pragma once
#include "ProjectExporter.h"
#include "../JamEngine/Dialog.h"
#include "../JamEngine/FileDialog.h"

struct ExportProjectDialogSetting
{
    void  WriteToArchive(ArchiveWriter& _archive) const;
    Error ReadFromArchive(ArchiveReader& _archive);

    String             exportDir            = "";
    String             baseName             = "";
    eProjectExportType type                 = eProjectExportType::CompiledBinary;
    bool               bIncludeDebugBuild   = false;
    bool               bIncludeReleaseBuild = false;
    String             debugMacro           = "DEBUG_OUTPUT";
    String             rootNamespace        = "";
    eNameFormat        nameFormat           = eNameFormat::NoSpace;
    String             namePrefix           = "";
};
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(ExportProjectDialogSetting);

class ExportProjectDialog : public Dialog
{
public:
    explicit ExportProjectDialog(const Project* _pProject);
    ~ExportProjectDialog() override;

    ExportProjectDialog(const ExportProjectDialog&)            = default;
    ExportProjectDialog& operator=(const ExportProjectDialog&) = default;
    ExportProjectDialog(ExportProjectDialog&&)                 = default;
    ExportProjectDialog& operator=(ExportProjectDialog&&)      = default;

    void OnShow() override;

private:
    bool TryLoadSetting_();
    void SaveSetting_() const;

    [[nodiscard]] FileDialog CreateFileDialog_() const;
    bool                     OnClickExportButton_() const;

    const Project*             m_pProject = nullptr;
    ExportProjectDialogSetting m_setting  = {};
    FileDialog                 m_exportDirFd;

    static constexpr StringView kExportProjectDialogSettingFileName = "ExportProjectDialogSetting.json";
};