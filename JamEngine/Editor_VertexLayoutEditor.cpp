#include "pch.h"

#include "Editor_VertexLayoutEditor.h"

#include "Editor.h"

#include "EditorUtilities.h"
#include "MessageDialog.h"
#include "VertexLayoutEditorDialog.h"
#include "VertexLayoutEntry.h"

namespace Jam
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

        ImGui::SetNextWindowSizeConstraints({ 300.f, 0.f }, { Limits<float>::k_max, Limits<float>::k_max });
        if (ImGui::BeginChild("VertexLayoutListPanel", GetContentRegionAvailScaled(0.7f, 1.f), ImGuiChildFlags_Borders))
        {
            // render vertex layout entries
            const VertexLayoutMap& map = JAM_EDITOR.GetVertexLayoutMap();
            for (const auto& [name, pVle]: map.GetContainer())
            {
                const char* label      = name.GetPtr();
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

void Editor_VertexLayoutEditor::OnClickCreateButton_()
{
    constexpr Int32 k_attemptLimit = 1000;

    VertexLayoutMap& map = JAM_EDITOR.GetVertexLayoutMap();
    for (Int32 i = 0; i < k_attemptLimit; ++i)
    {
        ScratchBuffer tmp;
        tmp.Set("new vertex layout");

        if (i > 0)
        {
            tmp.AppendFormat(" ({})", i);
        }

        // check duplicate
        const StringView name = tmp.ToStringView();
        if (!map.Has(name))
        {
            const Ref<VertexLayoutEntry> pVle = MakeRef<VertexLayoutEntry>(name, VertexLayout {});   // empty layout
            map.Insert(pVle);
            m_pFocusedVle = pVle;
            return;
        }

        // else: retry
    }

    MessageDialogDesc desc = {};
    desc.baseDesc.title    = "error";
    desc.msg               = "failed to create new vertex layout: too many vertex layouts named 'new vertex layout' exist.";
    JAM_EDITOR.PushDialog(MakeRef<MessageDialog>(desc));
}

void Editor_VertexLayoutEditor::OnClickEditButton_() const
{
    JAM_ASSERT(m_pFocusedVle, "No vertex vertexLayout is selected for editing.");

    VertexLayoutEditorDialogDesc desc = {};
    desc.baseDesc.title               = "vertex layout editor";
    desc.pVle                         = m_pFocusedVle;

    JAM_EDITOR.PushDialog(MakeRef<VertexLayoutEditorDialog>(desc));
}

void Editor_VertexLayoutEditor::OnClickRemoveButton_()
{
    JAM_ASSERT(m_pFocusedVle, "No vertex vertexLayout is selected for removal.");
    VertexLayoutMap& map = JAM_EDITOR.GetVertexLayoutMap();
    map.Remove(m_pFocusedVle->GetName());
    m_pFocusedVle = nullptr;
}

}   // namespace Jam