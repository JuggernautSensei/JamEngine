#include "pch.h"
#include "ExportProjectDialog.h"

#include "ShaderApp.h"
#include "../JamEngine/EditorUtilities.h"
#include "../JamEngine/EditorWidgets.h"
#include "../JamEngine/JsonArchive.h"

void ExportProjectDialogSetting::WriteToArchive(ArchiveWriter& _archive) const
{
    _archive.BeginObject();
    _archive.WriteField("exportDir", exportDir);
    _archive.WriteField("baseName", baseName);
    _archive.WriteField("type", type);
    _archive.WriteField("bIncludeDebugBuild", bIncludeDebugBuild);
    _archive.WriteField("bIncludeReleaseBuild", bIncludeReleaseBuild);
    _archive.WriteField("debugMacro", debugMacro);
    _archive.WriteField("rootNamespace", rootNamespace);
    _archive.WriteField("nameFormat", nameFormat);
    _archive.WriteField("namePrefix", namePrefix);
    _archive.EndObject();
}

Error ExportProjectDialogSetting::ReadFromArchive(ArchiveReader& _archive)
{
    if (const Error err = _archive.ReadField("exportDir", exportDir)) { return err; }
    if (const Error err = _archive.ReadField("baseName", baseName)) { return err; }
    if (const Error err = _archive.ReadField("type", type)) { return err; }
    if (const Error err = _archive.ReadField("bIncludeDebugBuild", bIncludeDebugBuild)) { return err; }
    if (const Error err = _archive.ReadField("bIncludeReleaseBuild", bIncludeReleaseBuild)) { return err; }
    if (const Error err = _archive.ReadField("debugMacro", debugMacro)) { return err; }
    if (const Error err = _archive.ReadField("rootNamespace", rootNamespace)) { return err; }
    if (const Error err = _archive.ReadField("nameFormat", nameFormat)) { return err; }
    if (const Error err = _archive.ReadField("namePrefix", namePrefix)) { return err; }
    return Error::None;
}

////////////////////////////////////////////////////////////////////////////////////////

ExportProjectDialog::ExportProjectDialog(
    const Project* _pProject)
    : Dialog("Export Project")
      , m_pProject(_pProject)
      , m_exportDirFd(CreateFileDialog_())
{
    JAM_ASSERT(m_pProject, "ExportProjectDialog: _pProject cannot be null.");

    // recover last setting if exists
    if (!TryLoadSetting_())
    {
        m_setting.exportDir = _pProject->GetWorkingDirectory();
        m_setting.baseName  = _pProject->GetName();
    }
}

ExportProjectDialog::~ExportProjectDialog()
{
    SaveSetting_();
}

void ExportProjectDialog::OnShow()
{
    if (ImGui::BeginTable("ExportProjectDialogTable", 2))
    {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextColumn();
        Widget_Text("export directory:", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_FileDialog("##ExportDir", m_setting.exportDir, m_exportDirFd);

        ImGui::TableNextColumn();
        Widget_Text("base name:", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_InputText("##BaseName", m_setting.baseName);

        ImGui::TableNextColumn();
        Widget_Text("export type:", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_EnumCombo("##ExportType", m_setting.type);

        ImGui::TableNextColumn();
        Widget_Text("root namespace:", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_InputText("##RootNamespace", m_setting.rootNamespace);

        ImGui::TableNextColumn();
        Widget_Text("name format:", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_EnumCombo("##NameFormat", m_setting.nameFormat);

        ImGui::TableNextColumn();
        Widget_Text("variable prefix:", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_InputText("##VariablePrefix", m_setting.namePrefix);

        // compiled binary options
        ImGui::BeginDisabled(m_setting.type != eProjectExportType::CompiledBinary);

        ImGui::TableNextColumn();
        Widget_Text("include debug build:", true);
        ImGui::TableNextColumn();
        ImGui::Checkbox("##IncludeDebugBuild", &m_setting.bIncludeDebugBuild);

        ImGui::BeginDisabled(!m_setting.bIncludeDebugBuild);
        ImGui::TableNextColumn();
        Widget_Text("debug macro:", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_InputText("##DebugMacro", m_setting.debugMacro);
        ImGui::EndDisabled();

        ImGui::TableNextColumn();
        Widget_Text("include release build:", true);
        ImGui::TableNextColumn();
        ImGui::Checkbox("##IncludeReleaseBuild", &m_setting.bIncludeReleaseBuild);
        ImGui::EndDisabled();

        ImGui::EndTable();
    }

    ImGui::Spacing();
    SetCursorScreenPosFromWindowBottom(ImGui::GetFrameHeight());
    const ImVec2 btnSize = CalcFitButtonSize(2);
    if (ImGui::Button("export", btnSize))
    {
        if (OnClickExportButton_())
        {
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("cancel", btnSize))
    {
        ImGui::CloseCurrentPopup();
    }
}

bool ExportProjectDialog::TryLoadSetting_()
{
    const String path = String::Joined(m_pProject->GetWorkingDirectory(), kExportProjectDialogSettingFileName);
    if (!FileSystem::Exists(path))
    {
        return false;
    }

    Expect<JsonReader> reader = JsonReader::ParseFromFile(path);
    if (!reader)
    {
        return false;
    }

    if (reader->Read(m_setting).IsError())
    {
        return false;
    }

    return true;
}

void ExportProjectDialog::SaveSetting_() const
{
    const String path = String::Joined(m_pProject->GetWorkingDirectory(), kExportProjectDialogSettingFileName);

    JsonWriter writer = {};
    writer.Write(m_setting);
    writer.SaveToFile(path, eJsonWriteFlags::Pretty);
}

FileDialog ExportProjectDialog::CreateFileDialog_() const
{
    OpenDirectoryDialogInfo info = {};
    info.title                   = "Select Export Directory";
    info.defaultPathOrEmpty      = "";
    info.bAllowMultiSelect       = false;
    return FileDialog { info };
}

bool ExportProjectDialog::OnClickExportButton_() const
{
    ExportProjectInfo info    = {};
    info.type                 = m_setting.type;
    info.bIncludeDebugBuild   = m_setting.bIncludeDebugBuild;
    info.bIncludeReleaseBuild = m_setting.bIncludeReleaseBuild;
    info.debugMacro           = m_setting.debugMacro;
    info.rootNamespace        = m_setting.rootNamespace;
    info.nameFormat           = m_setting.nameFormat;
    info.namePrefix           = m_setting.namePrefix;

    ProjectExporter exporter = ProjectExporter::Export(m_pProject, m_setting.exportDir, m_setting.baseName, info);
    if (exporter.IsFailed())
    {
        GetShaderApp().PushMessageDialog(
            "Export Failed",
            exporter.GetErrorMessage());

        return false;
    }

    return true;
}
