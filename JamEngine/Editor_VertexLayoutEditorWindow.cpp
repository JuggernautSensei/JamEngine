#include "pch.h"

#include "Editor_VertexLayoutEditorWindow.h"

#include "Editor.h"

#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "VertexLayoutEditorDialog.h"
#include "VertexLayoutEntryManager.h"

namespace Jam
{

namespace
{

    [[nodiscard]] String MakeUniqueVertexLayoutEntryName(const StringView _baseName)
    {
        const VertexLayoutEntryManager& mgr = GetEditor().GetVertexLayoutManager();

        String name = _baseName;
        Int32  i    = 0;
        while (mgr.Contains(name))
        {
            name = String::Formatted("{} ({})", _baseName, ++i);
        }
        return name;
    }

}   // namespace

void Editor_VertexLayoutEditorWindow::OnImGuiRender(
    bool& _bShow)
{
    ImGui::SetNextWindowSizeConstraints({ 300.f, 0.f }, { Limits<float>::k_max, Limits<float>::k_max });
    if (!ImGui::Begin("vertex layout editor", &_bShow))
    {
        ImGui::End();
        return;
    }

    // vertex layout list
    ImGui::TextUnformatted("vertex layout list");

    ImVec2 listSize = ImGui::GetContentRegionAvail();
    listSize.y -= ImGui::GetFrameHeightWithSpacing();
    if (ImGui::BeginListBox("##VertexLayoutList", listSize))
    {
        const VertexLayoutEntryManager& mgr = GetEditor().GetVertexLayoutManager();
        for (const auto& [name, pEntry]: mgr.GetEntries())
        {
            Widget_VertexLayoutEntryListItem_(pEntry);
        }

        // new button
        if (ImGui::Button("new", CalcFitButtonSize()))
        {
            OnClickNewButton_();
        }

        ImGui::EndChild();
    }

    // action buttons
    const ImVec2 btnSize = CalcFitButtonSize(4);
    ImGui::BeginDisabled(!m_pFocusedVle);
    if (ImGui::Button("edit", btnSize))
    {
        OnClickEditButton_(m_pFocusedVle);
    }

    if (ImGui::Button("clone", btnSize))
    {
        OnClickCloneButton_(m_pFocusedVle);
    }

    if (ImGui::Button("remove", btnSize))
    {
        OnClickRemoveButton_(m_pFocusedVle);
    }
    ImGui::EndDisabled();

    if (ImGui::Button("close", btnSize))
    {
        _bShow = false;
    }
}

void Editor_VertexLayoutEditorWindow::OnClickNewButton_()
{
    VertexLayoutEntryManager& mgr    = GetEditor().GetVertexLayoutManager();
    const String              name   = MakeUniqueVertexLayoutEntryName("new vertex layout");
    Ref<VertexLayoutEntry>    pEntry = mgr.Emplace(name, VertexLayout {});
    m_pFocusedVle                    = pEntry;
}

void Editor_VertexLayoutEditorWindow::OnClickEditButton_(
    const Ref<VertexLayoutEntry>& _pEntry) const
{
    JAM_ASSERT(_pEntry, "No vertex vertexLayout is selected for editing.");

    VertexLayoutEditorDialogInfo info = {};
    info.title                        = "vertex layout editor";
    info.pVle                         = _pEntry;
    GetEditor().PushDialog(MakeRef<VertexLayoutEditorDialog>(info));
}

void Editor_VertexLayoutEditorWindow::OnClickRemoveButton_(
    const Ref<VertexLayoutEntry>& _pEntry)
{
    JAM_ASSERT(_pEntry, "No vertex vertexLayout is selected for removal.");
    VertexLayoutEntryManager& map = GetEditor().GetVertexLayoutManager();
    map.Remove(_pEntry->GetName());

    if (m_pFocusedVle == _pEntry)
    {
        m_pFocusedVle = nullptr;
    }
}

void Editor_VertexLayoutEditorWindow::OnClickCloneButton_(
    const Ref<VertexLayoutEntry>& _pEntry) const
{
    JAM_ASSERT(_pEntry, "No vertex vertexLayout is selected for cloning.");

    VertexLayoutEntryManager& mgr    = GetEditor().GetVertexLayoutManager();
    const String              name   = MakeUniqueVertexLayoutEntryName(_pEntry->GetName());
    Ref<VertexLayoutEntry>    pEntry = mgr.Emplace(name, _pEntry->GetVertexLayout());
}

void Editor_VertexLayoutEditorWindow::Widget_VertexLayoutEntryListItem_(
    const Ref<VertexLayoutEntry>& _pVle)
{
    const bool bIsFocused = (_pVle == m_pFocusedVle);
    if (ImGui::Selectable(_pVle->GetName().GetPtr(), bIsFocused))
    {
        m_pFocusedVle = _pVle;
    }

    if (bIsFocused)
    {
        ImGui::SetItemDefaultFocus();
    }

    if (ImGui::BeginPopupContextItem())
    {
        ImGui::TextUnformatted(_pVle->GetName().GetPtr());
        ImGui::Separator();

        if (ImGui::MenuItem("edit"))
        {
            OnClickEditButton_(_pVle);
        }

        if (ImGui::MenuItem("clone"))
        {
            OnClickCloneButton_(_pVle);
        }

        if (ImGui::MenuItem("remove"))
        {
            OnClickRemoveButton_(_pVle);
        }

        ImGui::EndPopup();
    }
}

}   // namespace Jam