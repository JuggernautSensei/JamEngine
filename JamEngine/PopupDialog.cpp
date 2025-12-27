#include "pch.h"

#include "PopupDialog.h"

#include "EditorWidgets.h"
#include "ImGuiUtilities.h"

namespace jam
{

void PopupDialog::Show()
{
    if (m_bOpen)
    {
        ImGui::OpenPopup(m_title.c_str());
        m_bOpen = false;
    }

    constexpr ImVec2 k_winMin = { 300.f, k_floatMin };
    constexpr ImVec2 k_winMax = { 800.f, k_floatMax };
    bool             bDummy   = true;

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2 { 0.5f, 0.5f });
    ImGui::SetNextWindowSizeConstraints(k_winMin, k_winMax);
    if (ImGui::BeginPopupModal(m_title.c_str(), &bDummy, ImGuiWindowFlags_AlwaysAutoResize))
    {
        m_func();
        ImGui::EndPopup();
    }
}

void PopupDialog::Open(const StringView _title, const StringView _message)
{
    struct Func
    {
        void operator()() const
        {
            ImGui::Spacing();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.f);
            ImGui::TextUnformatted(message.c_str());
            ImGui::PopTextWrapPos();
            ImGui::Spacing();
            if (ImGui::Button("ok", CalcFitButtonSize()))
            {
                ImGui::CloseCurrentPopup();
            }
        }

        std::string message;
    };

    m_bOpen = true;
    m_title = _title.ToString();
    m_func  = Func { _message.ToString() };
}

void PopupDialog::Open(const StringView _title, const std::function<void()>& _fn)
{
    m_bOpen = true;
    m_title = _title.ToString();
    m_func  = _fn;
}

}   // namespace jam