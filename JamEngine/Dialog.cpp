#include "pch.h"

#include "Dialog.h"
#include "EditorUtilities.h"

namespace Jam
{

Dialog::Dialog(
    const StringView _title,
    const ImVec2     _size)
    : m_title(_title)
    , m_initSize(_size)
{
}

void Dialog::Open()
{
    m_bOpenRequest = true;
}

bool Dialog::BeginShow()
{
    if (m_bOpenRequest)
    {
        ImGui::OpenPopup(m_title.GetPtr());
        m_bOpenRequest = false;
    }

    if (const bool bOpened = BeginPopup_())
    {
        OnShow();
        return bOpened;
    }

    return false;
}

void Dialog::EndShow() const
{
    ImGui::EndPopup();
}

StringView Dialog::GetTitle() const
{
    return m_title;
}

bool Dialog::BeginPopup_() const
{
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, { 0.5f, 0.5f });
    bool bOpen = true;

    ImGuiPopupFlags flags = 0;
    if (IsZeroApprox(m_initSize))
    {
        flags = ImGuiWindowFlags_AlwaysAutoResize;
    }
    else
    {
        ImGui::SetNextWindowSize(m_initSize, ImGuiCond_Once);
    }

    return ImGui::BeginPopupModal(m_title.GetPtr(), &bOpen, flags);
}
}   // namespace Jam
