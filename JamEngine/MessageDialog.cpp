#include "pch.h"

#include "MessageDialog.h"

#include "EditorUtilities.h"
#include "EditorWidgets.h"

namespace Jam
{

MessageDialog::MessageDialog(
    const MessageDialogDesc& _desc)
    : Dialog(_desc.baseDesc)
    , m_msg(_desc.msg.ToString())
{
}

void MessageDialog::OnShow()
{
    Widget_TextWrapped(m_msg.GetPtr(), k_textWrapWidth);
    ImGui::Spacing();

    if (ImGui::Button("ok", CalcFitButtonSize()))
    {
        ImGui::CloseCurrentPopup();
    }
}

}   // namespace Jam