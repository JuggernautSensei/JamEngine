#include "pch.h"

#include "RenameFileDialog.h"

#include "Editor.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "MessageDialog.h"

namespace Jam
{

RenameFileDialog::RenameFileDialog(
    const RenameFileDialogDesc& _desc)
    : Dialog(_desc.baseDesc)
    , m_oldPath(_desc.srcPath)
    , m_filename(FileNameOf(_desc.srcPath))
{
}

void RenameFileDialog::OnShow()
{
    const ImVec2 btnSize = CalcFitButtonSize(2);

    // message
    Widget_TextWrapped("enter new name for file:", k_textWrapWidth);
    ImGui::Spacing();

    // input
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    Widget_InputText("##Input", m_filename);
    ImGui::Spacing();

    // buttons
    if (ImGui::Button("ok", btnSize))
    {
        if (JAM_EDITOR.ReplaceFileName(m_oldPath, m_filename))
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