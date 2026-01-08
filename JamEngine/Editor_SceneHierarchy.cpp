#include "pch.h"

#include "Editor_SceneHierarchy.h"

#include "AssetManager.h"
#include "Editor.h"
#include "EditorUtilities.h"
#include "Magics.h"
#include "Scene.h"
#include "SceneManager.h"
#include "SceneSerializer.h"
#include "TagComponent.h"
#include "SceneNode.h"
#include "EditorWidgets.h"

namespace jam
{

void Editor_SceneHierarchy::OnImGuiRender(
    bool& _pShow)
{
    if (!ImGui::Begin("scene hierarchy", &_pShow))
    {
        ImGui::End();
        return;
    }

    const bool        bHasScene     = SceneManager::HasActiveScene();
    const Ref<Scene>& pScene        = bHasScene ? SceneManager::GetActiveScene() : nullptr;
    const Entity      focusedEntity = Editor::GetFocusedEntity();

    // scene save load
    {
        const ImVec2 btnSize = CalcFitButtonSize(2);

        // save scene button
        ImGui::BeginDisabled(bHasScene);
        if (Widget_Button("save scene", btnSize, ImGuiMod_Ctrl | ImGuiKey_S))
        {
            Editor::SaveSceneBySaveDialog(pScene);
        }
        ImGui::EndDisabled();

        // load scene button
        ImGui::SameLine();
        if (Widget_Button("load scene", btnSize, ImGuiMod_Ctrl | ImGuiKey_O))
        {
            Editor::LoadSceneByOpenDialog();
        }
    }

    // scene inspector
    ImGui::Separator();
    if (!bHasScene)
    {
        Widget_CenteredText("no active scene.\nplease load scene.");
        ImGui::End();
        return;
    }

    // scene info
    {
        ImGui::Text("scene type: %s", pScene->GetTypeName().GetPtr());
        ImGui::Text("entity count: %d", pScene->GetNumEntities());
    }

    // entities
    ImGui::Separator();
    {
        const ImVec2 btnSize = CalcFitButtonSize(3);

        // create new entity
        if (Widget_Button("create", btnSize, ImGuiMod_Ctrl | ImGuiKey_N))
        {
            Editor::CreateEntity(pScene, Entity::s_null);
        }

        // clone focused entity
        ImGui::SameLine();
        ImGui::BeginDisabled(!Editor::IsClonableEntity(pScene, focusedEntity));
        if (Widget_Button("clone", btnSize, ImGuiMod_Ctrl | ImGuiKey_D))
        {
            Editor::CloneEntity(pScene, focusedEntity);
        }
        ImGui::EndDisabled();

        // destroy focused entity
        ImGui::SameLine();
        ImGui::BeginDisabled(!Editor::IsDestroyableEntity(pScene, focusedEntity));
        if (Widget_Button("destroy", btnSize, ImGuiKey_Delete))
        {
            Editor::DestroyEntity(pScene, focusedEntity);
        }
        ImGui::EndDisabled();
    }

    // entity list
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);   // root entity는 열려있도록
        if (ImGui::BeginChild("EntityListPanel", ImGui::GetContentRegionAvail(), ImGuiChildFlags_Border))
        {
            // draw entityItems
            Widget_EntityItem_Recursive_(pScene->GetRootEntity());

            // window popup
            if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
            {
                Menu_EntityListPanel_();
                ImGui::EndPopup();
            }

            // window drag drop target for accept model files
            const ImGuiWindow* pWin = ImGui::GetCurrentWindow();
            if (ImGui::BeginDragDropTargetCustom(pWin->Rect(), pWin->ID))
            {
                OnBeginDragDropTarget_EntityListPanel_();
                ImGui::EndDragDropTarget();
            }

            // if click window empty space, clear focus entity
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
            {
                Editor::SetFocusedEntity(Entity::s_null);
            }
        }
        ImGui::EndChild();
    }

    ImGui::End();
}

void Editor_SceneHierarchy::OnBeginDragDropTarget_EntityListPanel_() const
{
    if (const ImGuiPayload* pPayload = AcceptFileDragDropPayload())
    {
        const fs::path   path  = ResolvePathDragDropPayload(pPayload);
        const fs::path   ext   = path.extension();
        const StringView svExt = ext.native();

        if (svExt == k_jamModelExt)   // model file
        {
            Ref<Model> pModel = nullptr;
            if (!MoveIfHasValue(AssetManager::LoadModel(path), pModel))
            {
                const StringView msg = GlobalBuffer::FormatTmp("failed to load model asset from path: {}. {}", path.native(), GlobalBuffer::GetLastError());
                Editor::PushMessageDialog("error", msg);
                return;
            }

            JAM_ASSERT(SceneManager::HasActiveScene(), "no active scene to create entities into.");
            const Ref<Scene>& pScene = SceneManager::GetActiveScene();
            Editor::CreateEntitiesFrom(pScene, pModel);
        }
    }
}

void Editor_SceneHierarchy::Widget_EntityItem_Recursive_(
    Entity _entity)
{
    SceneNode& node         = _entity.GetSceneNode();
    const bool bHasChildren = node.HasChildren();
    const bool bIsFocused   = _entity == Editor::GetFocusedEntity();
    const UID  uid          = _entity.GetUID();
    const bool bHasTag      = _entity.HasComponent<TagComponent>();

    // push id
    ImGui::PushID(NarrowCast<Int32>(uid));

    // set label
    const char* label = nullptr;
    if (bHasTag)
    {
        const TagComponent& tag = _entity.GetComponent<TagComponent>();
        label                   = tag.name.c_str();
    }
    else
    {
        const StringView tmp = GlobalBuffer::FormatTmp("entity %d", uid);
        label                = tmp.GetPtr();
    }

    // render node flags
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DrawLinesFull;
    if (bIsFocused)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (!bHasChildren)
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    // render node
    bool       bOpen          = ImGui::TreeNodeEx(label, flags);
    bool       bRenderChilren = bOpen && bHasChildren;
    const bool bIsClicked     = ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen();   // for focus on click (not toggle)

    // right click to open context menu
    ImGui::OpenPopupOnItemClick("Menu", ImGuiPopupFlags_MouseButtonRight);

    // drag and drop source
    if (ImGui::BeginDragDropSource())
    {
        // render label
        ImGui::TextUnformatted(label);
        if (!bHasTag)
        {
            ImGui::TextDisabled("uid: %d", uid);
        }

        // set payload
        SetEntityDragDropPayload(_entity);
        ImGui::EndDragDropSource();
    }

    // drag and drop target
    if (ImGui::BeginDragDropTarget())
    {
        // accept
        if (const ImGuiPayload* pPayload = AcceptEntityDragDropPayload())
        {
            Entity     accepted     = ResolveEntityDragDropPayload(pPayload);
            SceneNode& acceptedNode = accepted.GetSceneNode();

            // check circular reference
            if (acceptedNode.IsAncestorOf(_entity))
            {
                Editor::PushMessageDialog("error", "cannot set parent.s would create circular reference.");
            }
            else
            {
                acceptedNode.SetParent(_entity);
            }
        }

        ImGui::EndDragDropTarget();
    }

    // click to focus
    if (bIsClicked)
    {
        Editor::SetFocusedEntity(_entity);
    }

    // context menu
    if (ImGui::BeginPopup("Menu"))
    {
        JAM_ASSERT(SceneManager::HasActiveScene(), "no active scene to create entity into.");
        const Ref<Scene>& _pScene = SceneManager::GetActiveScene();

        // render label
        ImGui::TextUnformatted(label);
        if (!bHasTag)
        {
            ImGui::TextDisabled("uid: %d", uid);
        }
        ImGui::Separator();

        // create child
        if (Widget_MenuItem("create child"))
        {
            Editor::CreateEntity(_pScene, _entity);
        }

        // clone entity
        ImGui::BeginDisabled(!Editor::IsClonableEntity(_pScene, _entity));
        if (Widget_MenuItem("clone"))
        {
            Editor::CloneEntity(_pScene, _entity);
        }
        ImGui::EndDisabled();

        // destroy entity
        ImGui::BeginDisabled(!Editor::IsDestroyableEntity(_pScene, _entity));
        if (Widget_MenuItem("destroy"))
        {
            Editor::DestroyEntity(_pScene, _entity);
        }
        ImGui::EndDisabled();

        ImGui::EndPopup();
    }

    // if node is open, draw children (recursion)
    if (bRenderChilren)
    {
        Entity child = node.GetFirstChild();
        while (child)
        {
            Widget_EntityItem_Recursive_(child);
            child = child.GetSceneNode().GetNextSibling();
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void Editor_SceneHierarchy::Menu_EntityListPanel_() const
{
    if (Widget_MenuItem("create"))
    {
        JAM_ASSERT(SceneManager::HasActiveScene(), "no active scene to create entity into.");
        const Ref<Scene>& pScene = SceneManager::GetActiveScene();
        Editor::CreateEntity(pScene, Entity::s_null);
    }
}

}   // namespace jam