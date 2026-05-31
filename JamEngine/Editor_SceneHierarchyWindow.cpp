#include "pch.h"

#include "Editor_SceneHierarchyWindow.h"

#include "Editor.h"
#include "EditorUtilities.h"

#include "Scene.h"
#include "SceneManager.h"
#include "TagComponent.h"
#include "SceneNode.h"
#include "EditorWidgets.h"
#include "Event.h"
#include "Model.h"
#include "Events.h"
#include "StaticString.h"

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

    const bool bHasScene = GetSceneManager().HasActiveScene();

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

        ImGui::Text("scene type: %s", m_pScene->GetTypeName().GetPtr());
        ImGui::Text("entity count: %d", m_pScene->GetNumEntities());
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

            Widget_EntityItem_Recursive_(m_pScene->GetRootEntity());

            // window popup
            if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
            {
                Menu_EntityListPanel_();
                ImGui::EndPopup();
            }

            // window drag drop target for accept model files
            const ImGuiWindow* pWin = ImGui::GetCurrentWindowRead();
            if (ImGui::BeginDragDropTargetCustom(pWin->Rect(), pWin->ID))
            {
                OnBeginDragDropTarget_EntityListPanel_();
                ImGui::EndDragDropTarget();
            }

            // if click window empty space, clear focus entity
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
            {
                GetEditor().SetFocusingEntity(Entity::kNull);
            }
        }
        ImGui::EndChild();
    }

    ImGui::End();
}

void Editor_SceneHierarchyWindow::OnEvent(Event& _event)
{
    EventDispatcher dispatcher(_event);
    dispatcher.Dispatch<SceneChangedEvent>(this, &Editor_SceneHierarchyWindow::OnSceneChangedEvent_);
}

void Editor_SceneHierarchyWindow::OnSceneChangedEvent_(SceneChangedEvent& _event)
{
    const SceneManager& mgr = GetSceneManager();
    if (!mgr.HasActiveScene())
    {
        m_pScene = nullptr;
        return;
    }

    m_pScene = mgr.GetActiveScene();
}

void Editor_SceneHierarchyWindow::Widget_SaveSceneButton_(
    const ImVec2 _btnSize) const
{
    ImGui::BeginDisabled(!GetSceneManager().HasActiveScene());

    const ImGuiKeyChord shortcut = GetEditor().GetStyleShortCut(eEditorShortCut::SaveScene);
    ImGui::SetNextItemShortcut(shortcut);
    if (ImGui::Button("save scene", _btnSize))
    {
        GetEditor().SaveSceneByFileDialog(m_pScene);
    }

    Widget_ItemDescriptionTooltip(
        "save scene",
        "save the current active scene to a file.",
        shortcut);

    ImGui::EndDisabled();
}

void Editor_SceneHierarchyWindow::Widget_LoadSceneButton_(
    const ImVec2 _btnSize) const
{
    const ImGuiKeyChord shortcut = GetEditor().GetStyleShortCut(eEditorShortCut::LoadScene);
    ImGui::SetNextItemShortcut(shortcut);
    if (ImGui::Button("load scene", _btnSize))
    {
        GetEditor().LoadScenByFileDialog();
    }

    Widget_ItemDescriptionTooltip(
        "load scene",
        "load a scene from a file and set it as the active scene.",
        shortcut);
}

void Editor_SceneHierarchyWindow::Widget_EntityItem_Recursive_(
    Entity _entity)
{
    SceneNode& node         = _entity.GetSceneNode();
    const bool bHasChildren = node.HasChildren();
    const bool bIsFocused   = _entity == GetEditor().GetFocusingEntityOrNull();
    const UID  uid          = _entity.GetUID();
    const bool bHasTag      = _entity.HaveComponents<TagComponent>();

    // push id
    ImGui::PushID(static_cast<Int32>(uid.GetValue()));

    // set label
    StaticString<256> label;
    if (bHasTag)
    {
        const TagComponent& tag = _entity.GetComponents<TagComponent>();
        label.Set(tag.name);
    }
    else
    {
        label.Format("entity {}", uid);
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
    bool       bOpen          = ImGui::TreeNodeEx(label.GetPtr(), flags);
    bool       bRenderChilren = bOpen && bHasChildren;
    const bool bIsClicked     = ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen();   // for focus on click (not toggle)

    // right click to open context menu
    ImGui::OpenPopupOnItemClick("Menu", ImGuiPopupFlags_MouseButtonRight);

    // drag and drop source
    if (ImGui::BeginDragDropSource())
    {
        // render label
        ImGui::TextUnformatted(label.GetPtr());
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
                GetEditor().PushMessageDialog(
                    "invalid parent",
                    "cannot set parent. the target entity is a descendant of the accepted entity.");
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
        GetEditor().SetFocusingEntity(_entity);
    }

    // context menu
    if (ImGui::BeginPopup("Menu"))
    {
        JAM_ASSERT(GetSceneManager().HasActiveScene(), "no active scene to create entity into.");

        // render label
        ImGui::TextUnformatted(label.GetPtr());
        if (!bHasTag)
        {
            ImGui::TextDisabled("uid: %d", uid);
        }
        ImGui::Separator();

        // create child
        if (Widget_MenuItem("create child"))
        {
            GetEditor().CreateEntity(m_pScene, _entity);
        }

        // clone entity
        ImGui::BeginDisabled(!GetEditor().CanEntityClone(_entity));
        if (Widget_MenuItem("clone"))
        {
            GetEditor().CloneEntity(m_pScene, _entity);
        }
        ImGui::EndDisabled();

        // destroy entity
        ImGui::BeginDisabled(!GetEditor().CanEntityDestroy(_entity));
        if (Widget_MenuItem("destroy"))
        {
            GetEditor().DestroyEntity(m_pScene, _entity);
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
    const ImGuiKeyChord shortcut = GetEditor().GetStyleShortCut(eEditorShortCut::CreateEntity);
    ImGui::SetNextItemShortcut(shortcut);
    if (ImGui::Button("create", _btnSize))
    {

        GetEditor().CreateEntity(m_pScene, Entity::kNull);
    }

    Widget_ItemDescriptionTooltip(
        "create entity",
        "create a new entity in the active scene.",
        shortcut);
}

void Editor_SceneHierarchyWindow::Widget_DeleteEntityButton_(
    const ImVec2 _btnSize) const
{
    const ImGuiKeyChord shortcut = GetEditor().GetStyleShortCut(eEditorShortCut::DeleteEntity);
    ImGui::SetNextItemShortcut(shortcut);
    if (ImGui::Button("delete", _btnSize))
    {

        Entity entity = GetEditor().GetFocusingEntityOrNull();
        GetEditor().DestroyEntity(m_pScene, entity);
    }

    Widget_ItemDescriptionTooltip(
        "delete entity",
        "delete the focused entity from the active scene.",
        shortcut);
}

void Editor_SceneHierarchyWindow::Widget_CloneEntityButton_(
    const ImVec2 _btnSize) const
{
    const ImGuiKeyChord shortcut = GetEditor().GetStyleShortCut(eEditorShortCut::CloneEntity);
    ImGui::SetNextItemShortcut(shortcut);
    if (ImGui::Button("clone", _btnSize))
    {

        Entity entity = GetEditor().GetFocusingEntityOrNull();
        GetEditor().CloneEntity(m_pScene, entity);
    }

    Widget_ItemDescriptionTooltip(
        "clone entity",
        "create a copy of the focused entity in the active scene.",
        shortcut);
}

void Editor_SceneHierarchyWindow::OnBeginDragDropTarget_EntityListPanel_() const
{
    if (const ImGuiPayload* pPayload = AcceptPathDragDropPayload())
    {
        const String path = ResolvePathDragDropPayload(pPayload);
        if (path.MatchExtension(Model::kExtension))
        {
            if (Expect<Ref<Model>> pModel = GetEditor().LoadModel(path))
            {

                GetEditor().CreateEntitiesFrom(m_pScene, pModel.GetValue());
            }
        }
    }
}

void Editor_SceneHierarchyWindow::Menu_EntityListPanel_() const
{
    if (Widget_MenuItem("create"))
    {
        JAM_ASSERT(GetSceneManager().HasActiveScene(), "no active scene to create entity into.");
        GetEditor().CreateEntity(m_pScene, Entity::kNull);
    }
}

}   // namespace Jam