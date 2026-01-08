#include "pch.h"

#include "Editor_VertexLayoutEditor.h"

#include "Editor.h"

#include "EditorUtilities.h"
#include "VertexLayoutEditorDialog.h"
#include "VertexLayoutEntry.h"

namespace jam
{

void Editor_VertexLayoutEditor::OnImGuiRender(
    bool& _pShow)
{
    if (!ImGui::Begin("vertex layout editor", &_pShow))
    {
        ImGui::End();
        return;
    }

    // vertex layout list
    ImGui::BeginGroup();
    {
        ImGui::TextUnformatted("vertex layout list");

        ImGui::SetNextWindowSizeConstraints({ 300.f, 0.f }, { FLT_MAX, FLT_MAX });
        if (ImGui::BeginChild("VertexLayoutListPanel", GetContentRegionAvailScaled(0.7f, 1.f), ImGuiChildFlags_Border))
        {
            // render vertex layout entries
            for (const auto& [name, pVle]: Editor::GetVertexLayoutMap())
            {
                const char* label      = name.c_str();
                const bool  bIsFocused = m_pFocusedVle == pVle;
                if (ImGui::Selectable(label, bIsFocused))
                {
                    m_pFocusedVle = pVle;
                }

                if (bIsFocused)
                {
                    ImGui::SetItemDefaultFocus();
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
        ImGui::NewLine();   // align with the vertex layout list

        const ImVec2 btnSize = CalcFitButtonSize();
        if (ImGui::Button("create", btnSize))
        {
            OnClickCreateButton_();
        }

        ImGui::BeginDisabled(!m_pFocusedVle);
        if (ImGui::Button("edit", btnSize))
        {
            OnClickEditButton_();
        }

        if (ImGui::Button("remove", btnSize))
        {
            OnClickRemoveButton_();
        }
        ImGui::EndDisabled();

        if (ImGui::Button("close", btnSize))
        {
            _pShow = false;
        }
    }
    ImGui::EndGroup();

    ImGui::End();
}

void Editor_VertexLayoutEditor::OnClickRemoveButton_()
{
    JAM_ASSERT(m_pFocusedVle, "No vertex vertexLayout is selected for removal.");
    VertexLayoutMap& map  = Editor::GetVertexLayoutMap();
    const StringView name = m_pFocusedVle->GetName();
    map.erase(name.GetPtr());
    m_pFocusedVle = nullptr;
}

void Editor_VertexLayoutEditor::OnClickCreateButton_()
{
    constexpr Int32 k_attemptLimit = 1000;

    VertexLayoutMap& map = Editor::GetVertexLayoutMap();
    for (Int32 i = 0; i < k_attemptLimit; ++i)
    {
        StringView name = {};
        if (i == 0)
        {
            name = "new vertex layout";
        }
        else
        {
            name = GlobalBuffer::FormatTmp("new vertex layout {}", i);
        }

        if (!map.contains(name.GetPtr()))
        {
            const Ref<VertexLayoutEntry> pVle = MakeRef<VertexLayoutEntry>(name);
            map.emplace(name.ToString(), pVle);
            m_pFocusedVle = pVle;
            return;
        }
        // else: retry
    }

    Editor::PushMessageDialog("error", "failed to create a new vertex layout. too many vertex layouts with similar names exist.");
}

void Editor_VertexLayoutEditor::OnClickEditButton_() const
{
    JAM_ASSERT(m_pFocusedVle, "No vertex vertexLayout is selected for editing.");
    Editor::PushDialog(MakeScoped<VertexLayoutEditorDialog>(m_pFocusedVle));
}

}   // namespace jam