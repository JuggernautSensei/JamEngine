#include "pch.h"

#include "Editor_EntityInspector.h"

#include "ComponentDB.h"
#include "Editor.h"
#include "Entity.h"
#include "Scene.h"
#include "SceneManager.h"

namespace jam
{

void Editor_EntityInspector::OnImGuiRender(
    bool& _pShow) const
{
    if (!ImGui::Begin("entity inspector", &_pShow))
    {
        ImGui::End();
        return;
    }

    // scene check
    if (!SceneManager::HasActiveScene())
    {
        Widget_CenteredText("scene not loaded");
        ImGui::End();
        return;
    }

    // entity check
    const Entity entity = Editor::GetFocusedEntity();
    if (!entity)
    {
        Widget_CenteredText("entity not selected");
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

    // component editor
    const ComponentDB& db     = ComponentDB::GetInstance();
    Ref<Scene>&        pScene = SceneManager::GetActiveScene();
    for (const ComponentInfo& info: db.GetComponentInfoMap() | std::views::values)
    {
        if (info.hasFn(entity) && info.drawEditorFn)
        {
            // render collapsing header
            const char* label = info.displayName.c_str();
            bool        bOpen = false;
            if (info.bCannotRemovable)
            {
                bOpen = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen);
            }
            else
            {
                bool bVisible = true;
                bOpen         = ImGui::CollapsingHeader(label, &bVisible, ImGuiTreeNodeFlags_DefaultOpen);

                // render collapsing header right popup
                if (ImGui::BeginPopupContextItem(0))
                {
                    if (Widget_MenuItem("remove"))
                    {
                        info.removeDeferedFn(entity);
                    }
                    ImGui::EndPopup();
                }

                // if not visible anymore, remove component
                if (!bVisible)
                {
                    info.removeDeferedFn(entity);
                }
            }

            // render editors if open collapsing header
            if (bOpen)
            {
                if (void* pCmp = info.getFn(entity))
                {
                    info.drawEditorFn(pCmp, pScene, entity);
                }
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

void Editor_EntityInspector::Menu_RemoveComponent_() const
{
    const ComponentDB& db     = ComponentDB::GetInstance();
    const Entity       entity = Editor::GetFocusedEntity();
    for (const ComponentInfo& info: db.GetComponentInfoMap() | std::views::values)
    {
        if (info.hasFn(entity) && !info.bCannotRemovable)
        {
            if (Widget_MenuItem(info.displayName.c_str()))
            {
                info.removeDeferedFn(entity);
            }
        }
    }
}

void Editor_EntityInspector::Menu_AddComponent_() const
{
    const ComponentDB& db     = ComponentDB::GetInstance();
    const Entity       entity = Editor::GetFocusedEntity();
    for (const ComponentInfo& info: db.GetComponentInfoMap() | std::views::values)
    {
        if (!info.hasFn(entity) && !info.bCannotCreatable)
        {
            if (Widget_MenuItem(info.displayName.c_str()))
            {
                info.createDeferedFn(entity);
            }
        }
    }
}
}   // namespace jam