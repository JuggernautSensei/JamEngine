#include "pch.h"

#include "Dialog.h"

namespace jam
{

Dialog::Dialog(
    const StringView   _title,
    const ImVec2       _size,
    const eDialogFlags _flags)
    : m_title(_title.ToString())
    , m_size(_size)
    , m_flags(_flags)
{
    m_bAutoResize = (IsZeroApprox(_size.x) && IsZeroApprox(_size.y));
    m_bSignal     = false;
}

void Dialog::Open()
{
    m_bSignal = true;
}

void Dialog::Show()
{
    if (m_bSignal)
    {
        ImGui::OpenPopup(m_title.c_str());
        m_bOpen = true;
    }

    if (OpenPopup_())
    {
        m_bSignal = false;
        m_bOpen   = true;

        OnShow();
        ImGui::EndPopup();
    }
    else
    {
        m_bOpen = false;
    }
}

bool Dialog::IsOpen() const
{
    return m_bOpen;
}

bool Dialog::OpenPopup_() const
{
    ImGuiContext& g    = *ImGui::GetCurrentContext();
    ImGuiWindow*  pWin = ImGui::GetCurrentWindow();
    const ImGuiID id   = pWin->GetID(m_title.c_str());
    if (!ImGui::IsPopupOpen(id, ImGuiPopupFlags_None))
    {
        g.NextWindowData.ClearFlags();
        return false;
    }

    // flags
    ImGuiWindowFlags flags = ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;

    if (m_bAutoResize)
        flags |= ImGuiWindowFlags_AlwaysAutoResize;

    if (m_flags & eDialogFlags_Modal)
        flags |= ImGuiWindowFlags_Modal;

    if (m_flags & eDialogFlags_NoResize)
        flags |= ImGuiWindowFlags_NoResize;

    // set next window pos to center of screen
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2 { 0.5f, 0.5f });

    // set next window size
    if (!m_bAutoResize)
    {
        ImGui::SetNextWindowSize(m_size, ImGuiCond_Once);
    }

    // set size constraints
    ImGui::SetNextWindowSizeConstraints(ImVec2 { 100.f, 50.f }, ImVec2 { FLT_MAX, FLT_MAX });

    // begin popup
    bool       bOpen    = true;
    const bool bWinOpen = ImGui::Begin(m_title.c_str(), &bOpen, flags);
    if (!bWinOpen || !bOpen)
    {
        ImGui::EndPopup();
        if (bWinOpen)
        {
            ImGui::ClosePopupToLevel(g.BeginPopupStack.Size, true);
        }

        return false;
    }

    return bWinOpen;
}

}   // namespace jam