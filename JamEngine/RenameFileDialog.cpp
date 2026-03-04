#include "pch.h"

#include "RenameFileDialog.h"

#include "Editor.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "MessageDialog.h"

namespace Jam
{

RenameFileDialog::RenameFileDialog(
    const RenameFileDialogInfo& _info)
    : Dialog(_info.title, _info.size)
    , m_oldPath(_info.srcPath)
    , m_filename(_info.srcPath.GetFileName())
{
}

void RenameFileDialog::OnShow()
{
    const ImVec2 btnSize = CalcFitButtonSize(2);

    // msg
    Widget_TextWrapped("enter new name for file:", k_textWrapWidth);
    ImGui::Spacing();

    // input
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    Widget_InputText("##Input", m_filename);
    ImGui::Spacing();

    // buttons
    if (ImGui::Button("ok", btnSize))
    {
        const String dstPath = String::Joined(m_oldPath.GetDirectory(), m_filename, eJoinType::File);
        if (GetEditor().RenameFile(m_oldPath, dstPath))
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

}   // namespace Jam