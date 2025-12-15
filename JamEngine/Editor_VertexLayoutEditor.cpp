#include "pch.h"

#include "Editor_VertexLayoutEditor.h"

#include "Editor.h"
#include "VertexLayout.h"
#include "VertexLayoutEditorDialog.h"

namespace jam
{

void Editor_VertexLayoutEditor::OnImGuiRender(bool& _pShow)
{
    if (!ImGui::Begin("vertex layout editor", &_pShow))
    {
        ImGui::End();
        return;
    }

    // vertex layout list
    ImGui::BeginGroup();
    {
        const ImVec2 avail   = ImGui::GetContentRegionAvail();
        const ImVec2 winSize = { avail.x * 0.7f, avail.y };

        ImGui::TextUnformatted("vertex layout list");
        if (ImGui::BeginChild("VertexLayoutListChild", winSize, ImGuiChildFlags_Border))
        {
            const VertexLayoutStorage& storage = Editor::GetVertexLayoutStorage();
            for (const auto& [name, pVle]: storage)
            {
                const char* label      = name.c_str();
                const bool  bIsFocused = m_pFocusedVle == pVle;
                if (ImGui::Selectable(label, bIsFocused))
                {
                    m_pFocusedVle = pVle;
                }
            }
        }
        ImGui::EndChild();
    }
    ImGui::EndGroup();

    // action buttons
    ImGui::SameLine();
    ImGui::BeginGroup();
    {
        ImGui::NewLine();

        const ImVec2 btnSize = CalcFitButtonSize();
        if (ImGui::Button("create", btnSize))
            OnClickCreateButton_();

        ImGui::BeginDisabled(!m_pFocusedVle);
        if (ImGui::Button("edit", btnSize))
            OnClickEditButton_();

        if (ImGui::Button("remove", btnSize))
            OnClickRemoveButton_();
        ImGui::EndDisabled();

        if (ImGui::Button("close", btnSize))
            _pShow = false;
    }
    ImGui::EndGroup();
    ImGui::End();
}

void Editor_VertexLayoutEditor::OnClickRemoveButton_()
{
    JAM_ASSERT(m_pFocusedVle, "No vertex vertexLayout is selected for removal.");
    Editor::GetVertexLayoutStorage().Remove(m_pFocusedVle->GetName());
    m_pFocusedVle = nullptr;
}

void Editor_VertexLayoutEditor::OnClickCreateButton_()
{
    VertexLayoutStorage& storage = Editor::GetVertexLayoutStorage();
    const char*          name    = "new vertex layout";
    Int32                suffix  = 0;

    while (storage.Has(name))
    {
        name = FormatStringToTmpBuffer("new vertex layout %d", ++suffix);
    }

    VertexLayout vertexLayout;   // dummy vertexLayout
    storage.Add(name, vertexLayout);
    m_pFocusedVle = storage.Get(name);
}

void Editor_VertexLayoutEditor::OnClickEditButton_() const
{
    JAM_ASSERT(m_pFocusedVle, "No vertex vertexLayout is selected for editing.");
    Editor::OpenCorePopupDialog("vertex vertexLayout editor", VertexLayoutEditorDialog { m_pFocusedVle });
}

}   // namespace jam