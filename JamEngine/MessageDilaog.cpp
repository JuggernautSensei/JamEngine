#include "pch.h"

#include "EditorUtilities.h"
#include "MessageDialog.h"

namespace Jam
{

MessageDialog::MessageDialog(
    const MessageDialogInfo& _info)
    : Dialog(_info.title, _info.size)
    , m_message(_info.message)
{
    m_buttons.assign(_info.buttons.begin(), _info.buttons.end());
}
void MessageDialog::OnShow()
{
    ImGui::TextWrapped(m_message.GetPtr());
    ImGui::Spacing();

    SetCursorScreenPosFromWindowBottom(ImGui::GetFrameHeight());
    const ImVec2 btnSize = CalcFitButtonSize(NumericCast<Int32>(m_buttons.size()));
    bool         bFirst  = true;
    for (const MessageDialogButtonInfo& buttonInfo: m_buttons)
    {
        if (!bFirst)
        {
            ImGui::SameLine();
        }
        bFirst = false;

        if (ImGui::Button(buttonInfo.label.GetPtr(), btnSize))
        {
            if (buttonInfo.onClick)
            {
                buttonInfo.onClick();
            }
            ImGui::CloseCurrentPopup();
        }
    }
}

}   // namespace Jam