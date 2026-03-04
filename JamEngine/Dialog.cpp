#include "pch.h"

#include "Dialog.h"
#include "EditorUtilities.h"
#include "StackString.h"

namespace Jam
{

Dialog::Dialog(
    const StringView _title,
    const ImVec2     _size)
    : m_title(_title)
    , m_size(_size)
{
}

void Dialog::Open()
{
    m_bOpenRequest = true;
}

void Dialog::Show()
{
    if (m_bOpenRequest)
    {
        ImGui::OpenPopup(m_title.GetPtr());
        m_bOpen        = true;
        m_bOpenRequest = false;
    }

    if (m_bOpen && BeginPopup_())
    {
        OnShow();
        ImGui::EndPopup();
    }
    else
    {
        // prevent m_bOpen == true && BeginPopup_() == false
        // m_bOpen must track the actual open state of the dialog
        m_bOpen = false;
    }
}

bool Dialog::IsOpen() const
{
    return m_bOpen || m_bOpenRequest;   // consider dialog open if there is an open request, even if the popup hasn't actually opened yet (BeginPopup_() will return false until the popup is actually open)
}

bool Dialog::BeginPopup_() const
{
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, { 0.5f, 0.5f });
    bool bOpen = true;

    ImGuiPopupFlags flags = 0;
    if (IsZeroApprox(m_size))
    {
        flags = ImGuiWindowFlags_AlwaysAutoResize;
    }
    else
    {
        ImGui::SetNextWindowSize(m_size, ImGuiCond_Appearing);
    }

    return ImGui::BeginPopupModal(m_title.GetPtr(), &bOpen, flags);
}
}   // namespace Jam
