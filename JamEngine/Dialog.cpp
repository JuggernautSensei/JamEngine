#include "pch.h"

#include "Dialog.h"
#include "EditorUtilities.h"

namespace Jam
{

Dialog::Dialog(
    const DialogDesc& _desc)
    : m_title(_desc.title.ToString())
      , m_size(_desc.size)
      , m_flags(_desc.flags)
{
}

void Dialog::Open()
{
    m_bOpenRequest = true;
}

void Dialog::Close()
{
    m_bOpen = false;
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
    return m_bOpen;
}

bool Dialog::BeginPopup_() const
{
    // check if popup is open
    ImGuiContext* pCtx = ImGui::GetCurrentContext();
    ImGuiWindow*  pWin = ImGui::GetCurrentWindow();
    const ImGuiID id   = pWin->GetID(m_title.GetPtr());
    if (!ImGui::IsPopupOpen(id, ImGuiPopupFlags_None))
    {
        pCtx->NextWindowData.ClearFlags();
        return false;
    }

    // flags
    ImGuiWindowFlags flags = ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;

    if (m_flags & eDialogFlags::AutoResize)
    {
        flags |= ImGuiWindowFlags_AlwaysAutoResize;
    }

    if (m_flags & eDialogFlags::Modal)
    {
        flags |= ImGuiWindowFlags_Modal;
    }

    if (m_flags & eDialogFlags::NoResize)
    {
        flags |= ImGuiWindowFlags_NoResize;
    }

    if (m_flags & eDialogFlags::NoMove)
    {
        flags |= ImGuiWindowFlags_NoMove;
    }

    // set next window pos to center of screen
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2 { 0.5f, 0.5f });

    // set next window size
    if (!(m_flags & eDialogFlags::AutoResize))
    {
        ImGui::SetNextWindowSize(m_size, ImGuiCond_Once);
    }

    // set size constraints
    ImGui::SetNextWindowSizeConstraints(ImVec2 { 100.f, 50.f }, ImVec2 { Limits<float>::k_max, Limits<float>::k_max });

    // begin popup
    bool       bOpen    = true;
    const bool bWinOpen = ImGui::Begin(m_title.GetPtr(), &bOpen, flags);
    if (!bWinOpen || !bOpen)
    {
        ImGui::EndPopup();
        if (bWinOpen)
        {
            ImGui::ClosePopupToLevel(pCtx->BeginPopupStack.Size, true);
        }

        return false;
    }

    return bWinOpen;
}

}   // namespace Jam