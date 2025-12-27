#pragma once
#include "Dialog.h"

namespace jam
{

class MessageDialog : public Dialog
{
public:
    explicit MessageDialog(
        const StringView _title,
        const StringView _message)
        : Dialog(_title, ImVec2 { 0.f, 0.f }, eDialogFlags_Modal)
        , m_message(_message)
    {
    }

    void OnShow() override
    {
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 400.f);   // wrap at 400px
        ImGui::TextUnformatted(m_message.GetPtr());
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        if (ImGui::Button("ok"))
        {
            ImGui::CloseCurrentPopup();
        }
    }

private:
    StringView m_message;
};

}   // namespace jam
