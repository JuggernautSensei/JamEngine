#include "pch.h"
#include "RenameDialog.h"

#include "GUIFileSystem.h"
#include "ShaderApp.h"
#include "../JamEngine/EditorUtilities.h"
#include "../JamEngine/EditorWidgets.h"

RenameDialog::RenameDialog(const StringView _path)
    : Dialog("rename")
    , m_oldPath(_path)
    , m_path(_path)
    , m_basename(_path.GetBaseName())
{
}

void RenameDialog::OnShow()
{
    if (ImGui::BeginTable("RenameDialogTable", 2))
    {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // path preview
        ImGui::TableNextColumn();
        Widget_Text("path ");
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        Widget_ReadOnlyInputText("##Path", m_path);

        // basename input
        ImGui::TableNextColumn();
        Widget_Text("name ");
        ImGui::TableNextColumn();
        SetNextItemToAvailWidth();
        if (Widget_InputText("##Name", m_basename))
        {
            bool bIsDir = m_oldPath.IsDirectory();
            m_path      = m_oldPath.GetDirectory();
            m_path.Join(m_basename, bIsDir ? eJoin::Directory : eJoin::File);
            if (!bIsDir)
            {
                m_path.ReplaceExtension(m_oldPath.GetExtension());
            }
        }

        ImGui::EndTable();
    }

    ImGui::Spacing();
    SetCursorScreenPosFromWindowBottom(ImGui::GetFrameHeight());
    const ImVec2 btnSize = CalcFitButtonSize(2);
    if (ImGui::Button("rename", btnSize))
    {
        if (OnClickRenameButton_())
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

bool RenameDialog::OnClickRenameButton_() const
{
    if (m_path == m_oldPath)
    {
        return true;
    }

    if (m_basename.IsEmpty())
    {
        GetShaderApp().PushMessageDialog("invalid name", "name cannot be empty.");
        return false;
    }

    if (FileSystem::Exists(m_path))
    {
        GetShaderApp().PushMessageDialog("invalid name", "a file or directory with the same name already exists.");
        return false;
    }

    return GUI::Rename(m_oldPath, m_path, FileSystem::eFileCollisionPolicy::Fail);
}
