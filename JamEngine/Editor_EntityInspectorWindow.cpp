#include "pch.h"

#include "Editor_EntityInspectorWindow.h"

#include "ComponentDatabase.h"
#include "Editor.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "Entity.h"
#include "Scene.h"
#include "SceneManager.h"

namespace Jam
{

void Editor_EntityInspectorWindow::OnImGuiRender(
    bool& _pShow) const
{
    if (!ImGui::Begin("entity inspector", &_pShow))
    {
        ImGui::End();
        return;
    }

    // scene check
    if (!JAM_SCENE_MGR.HasActiveScene())
    {
        Widget_TextFrame("scene not loaded", ImGui::GetContentRegionAvail());
        ImGui::End();
        return;
    }

    // entity check
    const Entity entity = JAM_EDITOR.GetFocusingEntity();
    if (!entity)
    {
        Widget_TextFrame("entity not selected", ImGui::GetContentRegionAvail());
        ImGui::End();
        return;
    }

    // render uid
    {
        ImGui::Text("uid: %d", entity.GetUID());
        ImGui::Separator();
    }

    // render add / remove component buttons
    {
        const ImVec2 btnSize = CalcFitButtonSize(2);

        // add component button
        ImGui::Button("add component", btnSize);
        if (ImGui::BeginPopupContextItem(0, ImGuiPopupFlags_MouseButtonLeft))
        {
            Menu_AddComponent_();
            ImGui::EndPopup();
        }

        // remove component button
        ImGui::SameLine();
        ImGui::Button("remove component", btnSize);
        if (ImGui::BeginPopupContextItem(0, ImGuiPopupFlags_MouseButtonLeft))
        {
            Menu_RemoveComponent_();
            ImGui::EndPopup();
        }
    }

    // component editor panels
    const Ref<Scene>& pScene = JAM_SCENE_MGR.GetActiveScene();
    for (const ComponentReflectionData& data: JAM_COMPONENT_DB.GetComponentReflectionContainer() | std::views::values)
    {
        if (data.Has(entity) && data.DrawEditPanel)
        {
            // render collapsing header
            const char* label = data.editorDisplayName.GetPtr();
            bool        bOpen = false;

            if (data.bNonRemoveable)
            {
                bOpen = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen);
            }
            else
            {
                // draw 'x' button to remove component
                bool bX = true;
                bOpen   = ImGui::CollapsingHeader(label, &bX, ImGuiTreeNodeFlags_DefaultOpen);

                // render collapsing header right popup
                if (ImGui::BeginPopupContextItem())
                {
                    if (Widget_MenuItem("remove"))
                    {
                        data.RemoveDefered(entity);
                    }
                    ImGui::EndPopup();
                }

                // if click 'x' button, remove component
                if (!bX)
                {
                    data.RemoveDefered(entity);
                }
            }

            // render editors if open collapsing header
            if (bOpen)
            {
                void* pCmp = data.Get(entity);
                data.DrawEditPanel(pCmp, pScene, entity);
                ImGui::Spacing();
            }
        }
    }

    // popup on void space
    if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
    {
        if (ImGui::BeginMenu("add component"))
        {
            Menu_AddComponent_();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("remove component"))
        {
            Menu_RemoveComponent_();
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

void Editor_EntityInspectorWindow::Menu_RemoveComponent_() const
{
    const Entity entity = JAM_EDITOR.GetFocusingEntity();
    for (const ComponentReflectionData& data: JAM_COMPONENT_DB.GetComponentReflectionContainer() | std::views::values)
    {
        if (data.Has(entity) && !data.bNonRemoveable)
        {
            if (Widget_MenuItem(data.editorDisplayName.GetPtr()))
            {
                data.RemoveDefered(entity);
            }
        }
    }
}

void Editor_EntityInspectorWindow::Menu_AddComponent_() const
{
    const Entity entity = JAM_EDITOR.GetFocusingEntity();
    for (const ComponentReflectionData& data: JAM_COMPONENT_DB.GetComponentReflectionContainer() | std::views::values)
    {
        if (!data.Has(entity) && !data.bNonCreatableByUser)
        {
            if (Widget_MenuItem(data.editorDisplayName.GetPtr()))
            {
                data.CreateDefered(entity);
            }
        }
    }
}

}   // namespace Jam