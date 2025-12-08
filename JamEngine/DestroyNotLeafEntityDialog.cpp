#include "pch.h"

#include "DestroyNotLeafEntityDialog.h"

#include "Editor.h"
#include "ImGuiUtilities.h"

namespace jam
{

DestroyNotLeafEntityDialog::DestroyNotLeafEntityDialog(const Entity _entity)
    : m_entity(_entity)
{
}

void DestroyNotLeafEntityDialog::operator()() const
{
    const ImVec2 btnSize = CalcFitButtonSize(2);

    ImGui::TextUnformatted("entity has child entities.\n"
                           "are you sure you want to destroy it and all its children?");

    ImGui::Spacing();
    if (ImGui::Button("yes", btnSize))
    {
        Editor::DestroyEntityToActiveScene(m_entity, true);
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("no", btnSize))
    {
        ImGui::CloseCurrentPopup();
    }
}

}   // namespace jam