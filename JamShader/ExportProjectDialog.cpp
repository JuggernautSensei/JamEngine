#include "pch.h"
#include "ExportProjectDialog.h"

#include "ShaderApp.h"
#include "../JamEngine/EditorUtilities.h"
#include "../JamEngine/EditorWidgets.h"
#include "../JamEngine/JsonArchive.h"

void ExportProjectDialogSetting::WriteToArchive(ArchiveWriter& _archive) const
{
    _archive.WriteField("exportDir", exportDir);
    _archive.WriteField("baseName", baseName);
    _archive.WriteField("type", type);
    _archive.WriteField("flags", flags);
    _archive.WriteField("debugMacro", debugMacro);
    _archive.WriteField("releaseMacro", releaseMacro);
    _archive.WriteField("rootNamespace", rootNamespace);
    _archive.WriteField("nameFormat", nameFormat);
    _archive.WriteField("namePrefix", namePrefix);
}

Error ExportProjectDialogSetting::ReadFromArchive(ArchiveReader& _archive)
{
    if (const Error err = _archive.ReadField("exportDir", exportDir))
    {
        return err;
    }
    if (const Error err = _archive.ReadField("baseName", baseName))
    {
        return err;
    }
    if (const Error err = _archive.ReadField("type", type))
    {
        return err;
    }
    if (const Error err = _archive.ReadField("flags", flags))
    {
        return err;
    }
    if (const Error err = _archive.ReadField("debugMacro", debugMacro))
    {
        return err;
    }
    if (const Error err = _archive.ReadField("releaseMacro", releaseMacro))
    {
        return err;
    }
    if (const Error err = _archive.ReadField("rootNamespace", rootNamespace))
    {
        return err;
    }
    if (const Error err = _archive.ReadField("nameFormat", nameFormat))
    {
        return err;
    }
    if (const Error err = _archive.ReadField("namePrefix", namePrefix))
    {
        return err;
    }
    return Error::None;
}

////////////////////////////////////////////////////////////////////////////////////////

ExportProjectDialog::ExportProjectDialog(
    const Project* _pProject)
    : Dialog("Export Project", { 500.f, 0.f })
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
        Widget_Text("name format:", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_EnumCombo("##NameFormat", m_setting.nameFormat);

        ImGui::TableNextColumn();
        Widget_Text("variable prefix:", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_InputText("##VariablePrefix", m_setting.namePrefix);

        ImGui::TableNextColumn();
        Widget_Text("export flags:", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_EnumFlags_();

        ImGui::BeginDisabled(m_setting.flags.Disabled(eProjectExportFlags::DebugExport));
        {
            ImGui::TableNextColumn();
            Widget_Text("debug macro:", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            Widget_InputText("##DebugMacro", m_setting.debugMacro);
        }
        ImGui::EndDisabled();

        ImGui::BeginDisabled(m_setting.flags.Disabled(eProjectExportFlags::ReleaseExport));
        {
            ImGui::TableNextColumn();
            Widget_Text("release macro:", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            Widget_InputText("##ReleaseMacro", m_setting.releaseMacro);
        }
        ImGui::EndDisabled();

        ImGui::TableNextColumn();
        Widget_Text("root namespace:", true);
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_InputText("##RootNamespace", m_setting.rootNamespace);

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
    const String path = String::Joined(m_pProject->GetWorkingDirectory(), k_exportProjectDialogSettingFileName);
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
    const String path = String::Joined(m_pProject->GetWorkingDirectory(), k_exportProjectDialogSettingFileName);

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
    ExportProjectInfo info = {};
    info.type              = m_setting.type;
    info.flags             = m_setting.flags;
    info.debugMacro        = m_setting.debugMacro;
    info.releaseMacro      = m_setting.releaseMacro;
    info.rootNamespace     = m_setting.rootNamespace;
    info.nameFormat        = m_setting.nameFormat;
    info.namePrefix        = m_setting.namePrefix;

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

void ExportProjectDialog::Widget_EnumFlags_()
{
    EnumWidgetItem k_flagItems[] = {
        { "Default export", NumericCast<Int32>(eProjectExportFlags::DefaultExport) },
        {   "Debug export",   NumericCast<Int32>(eProjectExportFlags::DebugExport) },
        { "Release export", NumericCast<Int32>(eProjectExportFlags::ReleaseExport) },
    };

    Int32 flags = NumericCast<Int32>(m_setting.flags.GetValue());
    if (Widget_EnumFlags("##WidgetEnumFlags", flags, k_flagItems, {}))
    {
        m_setting.flags = Flags<eProjectExportFlags> { NumericCast<UnderlyingT<eProjectExportFlags>>(flags) };
    }
}