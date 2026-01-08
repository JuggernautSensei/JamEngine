#include "pch.h"

#include "MessageDialog.h"

#include "EditorWidgets.h"
#include "ImGuiUtilities.h"

namespace jam
{

MessageDialog::MessageDialog(
    const StringView   _title,
    const StringView   _msg,
    const ImVec2       _size,
    const eDialogFlags _flags)
    : Dialog(_title, _size, _flags)
    , m_msg(_msg.ToString())
{
}

void MessageDialog::OnShow()
{
    Widget_WrappedText(m_msg.c_str(), k_textWrapWidth);
    ImGui::Spacing();

    if (ImGui::Button("ok", CalcFitButtonSize(1)))
    {
        ImGui::CloseCurrentPopup();
    }
}

void MessageDialog::SetMessage(
    const StringView _msg)
{
    m_msg = _msg.ToString();
}

}   // namespace jam