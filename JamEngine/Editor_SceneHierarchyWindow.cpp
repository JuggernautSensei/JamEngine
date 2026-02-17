#include "pch.h"

#include "Editor_SceneHierarchyWindow.h"

#include "Editor.h"
#include "EditorUtilities.h"

#include "Scene.h"
#include "SceneManager.h"
#include "TagComponent.h"
#include "SceneNode.h"
#include "EditorWidgets.h"
#include "MessageDialog.h"
#include "Model.h"

namespace Jam
{

void Editor_SceneHierarchyWindow::OnImGuiRender(
    bool& _pShow)
{
    if (!ImGui::Begin("scene hierarchy", &_pShow))
    {
        ImGui::End();
        return;
    }

    const bool bHasScene = JAM_SCENE_MGR.HasActiveScene();

    // scene save load
    {
        const ImVec2 btnSize = CalcFitButtonSize(2);

        Widget_SaveSceneButton_(btnSize);

        ImGui::SameLine();
        Widget_LoadSceneButton_(btnSize);
    }

    // scene inspector
    ImGui::Separator();
    if (!bHasScene)
    {
        Widget_TextFrame("no active scene.\nplease load scene.", ImGui::GetContentRegionAvail());
        ImGui::End();
        return;
    }

    // scene info
    {
        const Ref<Scene>& pScene = JAM_SCENE_MGR.GetActiveScene();
        ImGui::Text("scene type: %s", pScene->GetTypeName().GetPtr());
        ImGui::Text("entity count: %d", pScene->GetNumEntities());
    }

    // entities
    ImGui::Separator();
    {
        const ImVec2 btnSize = CalcFitButtonSize(3);

        // create new entity
        Widget_CreateEntityButton_(btnSize);

        // clone focused entity
        ImGui::SameLine();

        // destroy focused entity
        ImGui::SameLine();
    }

    // entity list
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);   // root entity는 열려있도록
        if (ImGui::BeginChild("EntityListPanel", ImGui::GetContentRegionAvail(), ImGuiChildFlags_Borders))
        {
            // draw entityItems
            const Ref<Scene>& pScene = JAM_SCENE_MGR.GetActiveScene();
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
                JAM_EDITOR.ReleaseFocusingEntity();
            }
        }
        ImGui::EndChild();
    }

    ImGui::End();
}

void Editor_SceneHierarchyWindow::Widget_SaveSceneButton_(
    const ImVec2 _btnSize) const
{
    ImGui::BeginDisabled(!JAM_SCENE_MGR.HasActiveScene());

    ImGui::SetNextItemShortcut(JAM_EDITOR.GetStyleShortCut(eEditorShortCut::SaveScene));
    if (ImGui::Button("save scene", _btnSize))
    {
        const Ref<Scene>& pScene = JAM_SCENE_MGR.GetActiveScene();
        JAM_EDITOR.SaveSceneByFileDialog(pScene);
    }

    Widget_ItemDescriptionTooltip(
        "save scene",
        "save the current active scene to a file.");

    ImGui::EndDisabled();
}

void Editor_SceneHierarchyWindow::Widget_LoadSceneButton_(
    const ImVec2 _btnSize) const
{
    ImGui::SetNextItemShortcut(JAM_EDITOR.GetStyleShortCut(eEditorShortCut::LoadScene));
    if (ImGui::Button("load scene", _btnSize))
    {
        JAM_EDITOR.LoadScenByFileDialog();
    }

    Widget_ItemDescriptionTooltip(
        "load scene",
        "load a scene from a file and set it as the active scene.");
}

void Editor_SceneHierarchyWindow::Widget_EntityItem_Recursive_(
    Entity _entity)
{
    SceneNode& node         = _entity.GetSceneNode();
    const bool bHasChildren = node.HasChildren();
    const bool bIsFocused   = _entity == JAM_EDITOR.GetFocusingEntity();
    const UID  uid          = _entity.GetUID();
    const bool bHasTag      = _entity.HasComponent<TagComponent>();

    // push id
    ImGui::PushID(NumericCast<Int32>(uid.GetValue()));

    // set label
    ScratchBuffer buf;
    if (bHasTag)
    {
        const TagComponent& tag = _entity.GetComponent<TagComponent>();
        buf.Set(tag.name);
    }
    else
    {
        buf.Format("entity {}", uid);
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
    bool       bOpen          = ImGui::TreeNodeEx(buf.GetPtr(), flags);
    bool       bRenderChilren = bOpen && bHasChildren;
    const bool bIsClicked     = ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen();   // for focus on click (not toggle)

    // right click to open context menu
    ImGui::OpenPopupOnItemClick("Menu", ImGuiPopupFlags_MouseButtonRight);

    // drag and drop source
    if (ImGui::BeginDragDropSource())
    {
        // render label
        ImGui::TextUnformatted(buf.GetPtr());
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
                MessageDialogInfo desc = {};
                desc.baseInfo.title    = "error";
                desc.msg               = "cannot set parent.s would create circular reference.";
                JAM_EDITOR.PushDialog(MakeRef<MessageDialog>(desc));
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
        JAM_EDITOR.SetFocusingEntity(_entity);
    }

    // context menu
    if (ImGui::BeginPopup("Menu"))
    {
        JAM_ASSERT(JAM_SCENE_MGR.HasActiveScene(), "no active scene to create entity into.");
        const Ref<Scene>& _pScene = JAM_SCENE_MGR.GetActiveScene();

        // render label
        ImGui::TextUnformatted(buf.GetPtr());
        if (!bHasTag)
        {
            ImGui::TextDisabled("uid: %d", uid);
        }
        ImGui::Separator();

        // create child
        if (Widget_MenuItem("create child"))
        {
            JAM_EDITOR.CreateEntity(_pScene, _entity);
        }

        // clone entity
        ImGui::BeginDisabled(!JAM_EDITOR.CanEntityClone(_entity));
        if (Widget_MenuItem("clone"))
        {
            JAM_EDITOR.CloneEntity(_pScene, _entity);
        }
        ImGui::EndDisabled();

        // destroy entity
        ImGui::BeginDisabled(!JAM_EDITOR.CanEntityDestroy(_entity));
        if (Widget_MenuItem("destroy"))
        {
            JAM_EDITOR.DestroyEntity(_pScene, _entity);
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

void Editor_SceneHierarchyWindow::Widget_CreateEntityButton_(
    const ImVec2 _btnSize) const
{
    ImGui::SetNextItemShortcut(JAM_EDITOR.GetStyleShortCut(eEditorShortCut::CreateEntity));
    if (ImGui::Button("create", _btnSize))
    {
        const Ref<Scene>& pScene = JAM_SCENE_MGR.GetActiveScene();
        JAM_EDITOR.CreateEntity(pScene, Entity::k_invalid);
    }

    Widget_ItemDescriptionTooltip(
        "create entity",
        "create a new entity in the active scene.");
}

void Editor_SceneHierarchyWindow::Widget_DeleteEntityButton_(
    const ImVec2 _btnSize) const
{
    ImGui::SetNextItemShortcut(JAM_EDITOR.GetStyleShortCut(eEditorShortCut::DeleteEntity));
    if (ImGui::Button("delete", _btnSize))
    {
        const Ref<Scene>& pScene = JAM_SCENE_MGR.GetActiveScene();
        Entity            entity = JAM_EDITOR.GetFocusingEntity();
        JAM_EDITOR.DestroyEntity(pScene, entity);
    }

    Widget_ItemDescriptionTooltip(
        "delete entity",
        "delete the focused entity from the active scene.");
}

void Editor_SceneHierarchyWindow::Widget_CloneEntityButton_(
    const ImVec2 _btnSize) const
{
    ImGui::SetNextItemShortcut(JAM_EDITOR.GetStyleShortCut(eEditorShortCut::CloneEntity));
    if (ImGui::Button("clone", _btnSize))
    {
        const Ref<Scene>& pScene = JAM_SCENE_MGR.GetActiveScene();
        Entity            entity = JAM_EDITOR.GetFocusingEntity();
        JAM_EDITOR.CloneEntity(pScene, entity);
    }

    Widget_ItemDescriptionTooltip(
        "clone entity",
        "create a copy of the focused entity in the active scene.");
}

void Editor_SceneHierarchyWindow::OnBeginDragDropTarget_EntityListPanel_() const
{
    if (const ImGuiPayload* pPayload = AcceptFileDragDropPayload())
    {
        const Path path = ResolvePathDragDropPayload(pPayload);
        if (MatchesExtension(path, Model::k_extension))
        {
            if (Optional<Ref<Model>> pModel = JAM_EDITOR.LoadModel(path))
            {
                const Ref<Scene>& pScene = JAM_SCENE_MGR.GetActiveScene();
                JAM_EDITOR.CreateEntitiesFrom(pScene, pModel.GetValue());
            }
        }
    }
}

void Editor_SceneHierarchyWindow::Menu_EntityListPanel_() const
{
    if (Widget_MenuItem("create"))
    {
        JAM_ASSERT(JAM_SCENE_MGR.HasActiveScene(), "no active scene to create entity into.");
        const Ref<Scene>& pScene = JAM_SCENE_MGR.GetActiveScene();
        JAM_EDITOR.CreateEntity(pScene, Entity::k_invalid);
    }
}

}   // namespace Jam