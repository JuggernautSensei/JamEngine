#include "pch.h"

#include "Editor_EntityInspector.h"

#include "Application.h"
#include "Editor.h"
#include "EditorUtilities.h"
#include "Entity.h"
#include "Scene.h"
#include "SceneManager.h"

#include <efsw/efsw.hpp>

namespace jam
{

void Editor_EntityInspector::OnImGuiRender(bool& _pShow) const
{
    if (!ImGui::Begin("entity inspector", &_pShow))
    {
        ImGui::End();
        return;
    }

    if (!SceneManager::HasActiveScene())
    {
        constexpr const char* k_msg = "no active scene.\nplease load scene.";
        Widget_CenterAlignedTextDisabled(k_msg);
        ImGui::End();
        return;
    }

    const Entity entity = Editor::GetFocusEntity();
    if (!entity)
    {
        constexpr const char* k_msg = "no focused entity.\nplease select an entity\nfrom scene hierarchy.";
        Widget_CenterAlignedTextDisabled(k_msg);
        ImGui::End();
        return;
    }

    // entity info
    {
        ImGui::Text("uid: %d", entity.GetUID());
        ImGui::Separator();
    }

    // create or delete component
    {
        const ImVec2 btnSize = CalcFitButtonSize(2);

        ImGui::Button("add component", btnSize);
        if (ImGui::BeginPopupContextItem(0, ImGuiPopupFlags_MouseButtonLeft))
        {
            DrawAddComponentPopup_();
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::Button("remove component", btnSize);
        if (ImGui::BeginPopupContextItem(0, ImGuiPopupFlags_MouseButtonLeft))
        {
            DrawRemoveComponentPopup_();
            ImGui::EndPopup();
        }
    }

    // component editor
    const ComponentDB::ContainerT& container = ComponentDB::GetInstance().GetContainer();
    for (const ComponentInfo& info: std::views::values(container))
    {
        if (info.hasFn(entity) && info.drawEditorFn)
        {
            const char* label = info.displayName.c_str();
            const bool  bOpen = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen);

            // remove button
            if (info.removeFn && ImGui::BeginPopupContextItem(0))
            {
                if (ImGui::MenuItem("remove"))
                    RemoveComponent_(info.hash, entity);
                ImGui::EndPopup();
            }

            // draw editor
            if (bOpen)
            {
                Ref<Scene>& pScene = SceneManager::GetActiveScene();
                void*       pComp  = info.getFn(entity);

                info.drawEditorFn(pComp, pScene, entity);
                ImGui::Spacing();
            }
        }
    }

    // popup
    if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
    {
        if (ImGui::BeginMenu("add component"))
        {
            DrawAddComponentPopup_();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("remove component"))
        {
            DrawRemoveComponentPopup_();
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

void Editor_EntityInspector::DrawRemoveComponentPopup_() const
{
    const ComponentDB::ContainerT& container = ComponentDB::GetInstance().GetContainer();
    const Entity                   entity    = Editor::GetFocusEntity();

    for (const ComponentInfo& info: std::views::values(container))
    {
        if (info.hasFn(entity) && info.removeFn && !info.bCannotRemovable)
        {
            if (ImGui::MenuItem(info.displayName.c_str()))
            {
                RemoveComponent_(info.hash, entity);
                ImGui::CloseCurrentPopup();
                break;
            }
        }
    }
}

void Editor_EntityInspector::DrawAddComponentPopup_() const
{
    const ComponentDB::ContainerT& container = ComponentDB::GetInstance().GetContainer();
    const Entity                   entity    = Editor::GetFocusEntity();

    for (const ComponentInfo& info: std::views::values(container))
    {
        if (!info.hasFn(entity) && info.createFn && !info.bCannotCreatable)
        {
            if (ImGui::MenuItem(info.displayName.c_str()))
            {
                AddComponent_(info.hash, entity);
                ImGui::CloseCurrentPopup();
                break;
            }
        }
    }
}

void Editor_EntityInspector::AddComponent_(const UInt32 _typeID, const Entity _entity) const
{
    struct CreateComponentCommand
    {
        void operator()() const
        {
            ComponentDB::GetInstance().Create(typeID, entity);
        }

        UInt32 typeID;
        Entity entity;
    };

    Application::GetInstance().SubmitCommand(CreateComponentCommand { _typeID, _entity });
}

void Editor_EntityInspector::RemoveComponent_(const UInt32 _typeID, const Entity _entity) const
{
    struct RemoveComponentCommand
    {
        void operator()() const
        {
            ComponentDB::GetInstance().Remove(typeID, entity);
        }

        UInt32 typeID;
        Entity entity;
    };

    Application::GetInstance().SubmitCommand(RemoveComponentCommand { _typeID, _entity });
}

}   // namespace jam