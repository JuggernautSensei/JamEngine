#include "pch.h"

#include "Editor_EntityInspector.h"

#include "ReflectionDB.h"
#include "Editor.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "Entity.h"
#include "Scene.h"
#include "SceneManager.h"

namespace Jam
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

    // component editor
    const ReflectionDB& db     = ReflectionDB::GetInstance();
    const Ref<Scene>&   pScene = JAM_SCENE_MGR.GetActiveScene();
    for (const ComponentMetadata& meta: db.GetComponentMetadataContainer() | std::views::values)
    {
        if (meta.Has(entity) && meta.Panel_EditComponent)
        {
            // render collapsing header
            const char* label = meta.editorDisplayName.GetPtr();
            bool        bOpen = false;

            if (meta.bCannotRemove)
            {
                bOpen = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen);
            }
            else
            {
                bool bVisible = true;
                bOpen         = ImGui::CollapsingHeader(label, &bVisible, ImGuiTreeNodeFlags_DefaultOpen);

                // render collapsing header right popup
                if (ImGui::BeginPopupContextItem())
                {
                    if (Widget_MenuItem("remove"))
                    {
                        meta.RemoveDefered(entity);
                    }
                    ImGui::EndPopup();
                }

                // if not visible anymore, remove component
                if (!bVisible)
                {
                    meta.RemoveDefered(entity);
                }
            }

            // render editors if open collapsing header
            if (bOpen)
            {
                if (void* pCmp = meta.Get(entity))
                {
                    meta.Panel_EditComponent(pCmp, pScene, entity);
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
    const Entity        entity = JAM_EDITOR.GetFocusingEntity();
    const ReflectionDB& db     = ReflectionDB::GetInstance();
    for (const ComponentMetadata& meta: db.GetComponentMetadataContainer() | std::views::values)
    {
        if (meta.Has(entity) && !meta.bCannotRemove)
        {
            if (Widget_MenuItem(meta.editorDisplayName.GetPtr()))
            {
                meta.RemoveDefered(entity);
            }
        }
    }
}

void Editor_EntityInspector::Menu_AddComponent_() const
{
    const Entity        entity = JAM_EDITOR.GetFocusingEntity();
    const ReflectionDB& db     = ReflectionDB::GetInstance();
    for (const ComponentMetadata& meta: db.GetComponentMetadataContainer() | std::views::values)
    {
        if (!meta.Has(entity) && !meta.bCannotCreateInEditor)
        {
            if (Widget_MenuItem(meta.editorDisplayName.GetPtr()))
            {
                meta.CreateDefered(entity);
            }
        }
    }
}
}   // namespace Jam