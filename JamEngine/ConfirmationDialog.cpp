#include "pch.h"

#include "ConfirmationDialog.h"

#include "EditorWidgets.h"
#include "ImGuiUtilities.h"

namespace jam
{

ConfirmationDialog::ConfirmationDialog(
    const StringView             _title,
    const StringView             _message,
    const std::function<void()>& _onOk,
    const std::function<void()>& _onCancel,
    const ImVec2                 _size,
    const eDialogFlags           _flags)
    : Dialog(_title, _size, _flags)
    , m_onOk(_onOk)
    , m_onCancel(_onCancel)
    , m_msg(_message.ToString())
{
}

void ConfirmationDialog::OnShow()
{
    Widget_WrappedText(m_msg.c_str(), k_textWrapWidth);
    ImGui::Spacing();

    // buttons
    const ImVec2 btnSize = CalcFitButtonSize(2);
    if (ImGui::Button("ok", btnSize))
    {
        if (m_onOk)
        {
            m_onOk();
        }
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("cancel", btnSize))
    {
        if (m_onCancel)
        {
            m_onCancel();
        }
        ImGui::CloseCurrentPopup();
    }
}

}   // namespace jam