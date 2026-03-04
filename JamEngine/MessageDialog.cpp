#include "pch.h"

#include "MessageDialog.h"

#include "EditorUtilities.h"
#include "EditorWidgets.h"

namespace Jam
{

MessageDialog::MessageDialog(
    const MessageDialogInfo& _info)
    : Dialog(_info.title, _info.size)
    , m_msg(_info.msg)
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