#include "pch.h"

#include "ConfirmationDialog.h"

#include "EditorUtilities.h"
#include "EditorWidgets.h"

namespace Jam
{

ConfirmationDialog::ConfirmationDialog(
    const ConfirmationDialogInfo& _desc)
    : Dialog(_desc.baseInfo)
    , m_msg(_desc.msg.ToString())
    , m_OnClickOk(_desc.OnConfirm)
    , m_OnClickCancel(_desc.OnCancel)
{
}

void ConfirmationDialog::OnShow()
{
    Widget_TextWrapped(m_msg.GetPtr(), k_textWrapWidth);
    ImGui::Spacing();

    // buttons
    const ImVec2 btnSize = CalcFitButtonSize(2);
    if (ImGui::Button("ok", btnSize))
    {
        if (m_OnClickOk)
        {
            m_OnClickOk.Call();
        }
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("cancel", btnSize))
    {
        if (m_OnClickCancel)
        {
            m_OnClickCancel.Call();
        }
        ImGui::CloseCurrentPopup();
    }
}

}   // namespace Jam