#include "pch.h"

#include "Editor_SceneHierarchy.h"

#include "AssetManager.h"
#include "Editor.h"
#include "EditorUtilities.h"
#include "Magics.h"
#include "Model.h"

#include "Scene.h"
#include "SceneManager.h"
#include "SceneSerializer.h"
#include "TagComponent.h"
#include "SceneNode.h"

namespace jam
{

void Editor_SceneHierarchy::Widget_SaveSceneButton_(const ImVec2 btnSize) const
{
    const bool bHasScene = SceneManager::HasActiveScene();

    ImGui::BeginDisabled(!bHasScene);

    ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_S);
    if (ImGui::Button("save scene", btnSize))
    {
        Editor::SaveActiveSceneByDialog();
    }

    if (ImGui::BeginItemTooltip())
    {
        if (bHasScene)
        {
            ImGui::Text("save active scene to file");
            ImGui::TextDisabled("short cut: ctrl + s");
        }
        else
        {
            ImGui::Text("no active scene to save");
        }
        ImGui::EndTooltip();
    }

    ImGui::EndDisabled();
}

void Editor_SceneHierarchy::Widget_LoadSceneButton_(const ImVec2 btnSize) const
{
    ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_O);
    if (ImGui::Button("load scene", btnSize))
        Editor::LoadActiveSceneByDialog();

    if (ImGui::BeginItemTooltip())
    {
        ImGui::Text("load scene from file");
        ImGui::TextDisabled("short cut: ctrl + o");
        ImGui::EndTooltip();
    }
}

void Editor_SceneHierarchy::Widget_CloneEntityButton_(const ImVec2 btnSize) const
{
    const Entity entity   = Editor::GetFocusEntity();
    const bool   bIsValid = !!entity;
    const bool   bIsRoot  = bIsValid && entity.GetSceneNode().IsRoot();

    ImGui::BeginDisabled(!bIsValid || bIsRoot);

    ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_D);
    if (ImGui::Button("clone", btnSize))
    {
        Editor::CloneEntityToActiveScene(entity);
    }
    if (ImGui::BeginItemTooltip())
    {
        if (bIsValid)
        {
            if (bIsRoot)
            {
                ImGui::Text("cannot clone root entity");
            }
            else
            {
                ImGui::Text("clone focused entity");
                ImGui::TextDisabled("short cut: ctrl + d");
            }
        }
        else
        {
            ImGui::Text("no focused entity to clone");
        }

        ImGui::EndTooltip();
    }

    ImGui::EndDisabled();
}

void Editor_SceneHierarchy::Widget_DestroyEntity_(const ImVec2 btnSize) const
{
    const Entity entity   = Editor::GetFocusEntity();
    const bool   bIsValid = !!entity;
    const bool   bIsRoot  = bIsValid && entity.GetSceneNode().IsRoot();

    ImGui::BeginDisabled(!bIsValid || bIsRoot);

    ImGui::SetNextItemShortcut(ImGuiKey_Delete);
    if (ImGui::Button("destroy", btnSize))
    {
        Editor::DestroyEntityToActiveScene(entity);
    }

    if (ImGui::BeginItemTooltip())
    {
        if (bIsValid)
        {
            if (bIsRoot)
            {
                ImGui::Text("cannot destroy root entity");
            }
            else
            {
                ImGui::Text("destroy focused entity");
                ImGui::TextDisabled("short cut: delete");
            }
        }
        else
        {
            ImGui::Text("no focused entity to destroy");
        }
        ImGui::EndTooltip();
    }

    ImGui::EndDisabled();
}

void Editor_SceneHierarchy::Widget_CreateEntityButton_(const ImVec2 btnSize) const
{
    ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_N);
    if (ImGui::Button("create", btnSize))
    {
        Editor::CreateEntityToActiveScene();
    }

    if (ImGui::BeginItemTooltip())
    {
        ImGui::Text("create new entity");
        ImGui::TextDisabled("short cut: ctrl + n");
        ImGui::EndTooltip();
    }
}

void Editor_SceneHierarchy::OnImGuiRender(bool& _pShow)
{
    if (!ImGui::Begin("scene hierarchy", &_pShow))
    {
        ImGui::End();
        return;
    }

    // scene save load
    {
        const ImVec2 btnSize = CalcFitButtonSize(2);

        // save scene button
        Widget_SaveSceneButton_(btnSize);

        // load scene button
        ImGui::SameLine();
        Widget_LoadSceneButton_(btnSize);
    }

    // scene inspector
    ImGui::Separator();
    if (!SceneManager::HasActiveScene())
    {
        Widget_CenterAlignedText("no active scene.\nplease load scene.");
        ImGui::End();
        return;
    }

    const Ref<Scene>& pScene = SceneManager::GetActiveScene();

    // scene info
    {
        ImGui::Text("scene type: %s", pScene->GetTypeName().data());
        ImGui::Text("entity count: %d", pScene->GetNumEntities());
    }

    // entities
    ImGui::Separator();
    {
        const ImVec2 btnSize = CalcFitButtonSize(3);

        // create
        Widget_CreateEntityButton_(btnSize);

        // clone
        ImGui::SameLine();
        Widget_CloneEntityButton_(btnSize);

        // destroy
        ImGui::SameLine();
        Widget_DestroyEntity_(btnSize);
    }

    // entity list
    {
        const ImVec2 winSize = ImGui::GetContentRegionAvail();
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);   // root entity는 열려있도록
        if (ImGui::BeginChild("EntityListChild", winSize, ImGuiChildFlags_Border))
        {
            // draw entityItems
            Widget_EntityItem_Recursive_(pScene->GetRootEntity());

            // window popup
            if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
            {
                DrawEntityItemChildWindowPopup_();
                ImGui::EndPopup();
            }

            // window drag drop target
            const ImGuiWindow* pCurWin = ImGui::GetCurrentWindow();
            if (ImGui::BeginDragDropTargetCustom(pCurWin->Rect(), pCurWin->ID))
            {
                OnEntityListChildWindowDragDropTarget_();
                ImGui::EndDragDropTarget();
            }

            // if click window empty space, clear focus entity
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
            {
                Editor::SetFocusEntity(Entity::s_null);
            }
        }
        ImGui::EndChild();
    }

    ImGui::End();
}

void Editor_SceneHierarchy::OnEntityListChildWindowDragDropTarget_()
{
    if (const ImGuiPayload* pPayload = AcceptFileDragDropPayload())
    {
        const fs::path path = ResolvePathDragDropPayload(pPayload);
        const fs::path ext  = path.extension();
        if (ext == k_jamModelExt)
        {
            if (const Optional<Ref<Model>> opt = AssetManager::GetOrLoadModel(path))
            {
                const Ref<Model>& pModel = opt.value();
                CreateEntityTreeFromModel_(pModel);
            }
            else
            {
                const std::string msg = std::format("failed to load model from path: {}\nerror: {}", path.native(), GlobalBuffer::GetLastError());
                Log::Error(msg);
                Editor::OpenCorePopupDialog("error", msg);
            }
        }
    }
}

void Editor_SceneHierarchy::CreateEntityTreeFromModel_(const Ref<Model>& _model)
{
    const ModelNode&             root  = _model->GetRootNode();
    const Vector<fs::path>& meshs = _model->GetMeshs();
    const Vector<fs::path>& mats  = _model->GetMaterials();
    CreateEntityTreeFromModelNode_Recursive_(root, Entity::s_null, meshs, mats);
}

void Editor_SceneHierarchy::CreateEntityTreeFromModelNode_Recursive_(const ModelNode& _node, const Entity _parentOrNull, const Vector<fs::path>& _meshs, const Vector<fs::path>& _mats)
{
    Ref<Scene>&         pScene = SceneManager::GetActiveScene();
    Entity              entity = pScene->CreateEntity();
    SceneNode& th     = entity.GetSceneNode();

    // _parentOrNull 이 유효하면 부모 설정
    if (_parentOrNull.IsValid())
    {
        th.SetParent(_parentOrNull);
    }

    // set local transformHierarchy
    Transform local;
    local.SetMatrix(_node.mtx);
    th.SetLocalTransform(local);

    // create leaf
    const Int32 numLeafs = NarrowCast<Int32>(_node.meshIndices.size());
    for (Int32 i = 0; i < numLeafs; ++i)
    {
        // create leaf entity
        Entity              leaf   = pScene->CreateEntity();
        SceneNode& leafTh = leaf.GetSceneNode();
        leafTh.SetParent(entity);

        // attach renderable component
        const fs::path       meshPath = _meshs[_node.meshIndices[i]];
        const fs::path       matPath  = _mats[_node.materialIndices[i]];
        RenderableComponent& rc       = leaf.CreateComponent<RenderableComponent>();

        if (Optional<Ref<Mesh>> opt = AssetManager::GetOrLoadMesh(meshPath))
        {
            rc.renderable.pMesh = opt.value();
        }
        else
        {
            std::string msg = std::format("failed to load mesh from path: {}\nerror: {}", meshPath.native(), GlobalBuffer::GetLastError());
            Log::Error(msg);
            Editor::OpenCorePopupDialog("error", msg);
        }

        if (Optional<Ref<Material>> opt = AssetManager::GetOrLoadMaterial(matPath))
        {
            rc.renderable.pMaterial = opt.value();
        }
        else
        {
            std::string msg = std::format("failed to load material from path: {}\nerror: {}", matPath.native(), GlobalBuffer::GetLastError());
            Log::Error(msg);
            Editor::OpenCorePopupDialog("error", msg);
        }
    }

    // create child
    for (const ModelNode& childNode: _node.children)
    {
        CreateEntityTreeFromModelNode_Recursive_(childNode, entity, _meshs, _mats);
    }
}

void Editor_SceneHierarchy::Widget_EntityItem_Recursive_(Entity _entity)
{
    const Entity fChild     = _entity.GetSceneNode().GetFirstChild();
    const bool   bHasChild  = fChild.IsValid();
    const bool   bIsFocused = (_entity == Editor::GetFocusEntity());
    const UID    uid        = _entity.GetUID();
    const bool   bHasTag    = _entity.HasComponent<TagComponent>();

    // push id
    ImGui::PushID(NarrowCast<Int32>(uid));

    // set label
    const char* label;
    if (bHasTag)
    {
        const TagComponent& tag = _entity.GetComponent<TagComponent>();
        label                   = tag.name.c_str();
    }
    else
    {
        label = GlobalBuffer::FormatTmp("entity %d", uid);
    }

    // render node
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DrawLinesFull;
    if (bIsFocused)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (!bHasChild)
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    // render node
    bool bNodeOpen = (bNodeOpen = ImGui::TreeNodeEx(label, flags));
    bNodeOpen &= bHasChild;

    // right click to open context menu
    const bool bIsClicked = ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen();
    ImGui::OpenPopupOnItemClick("Menu", ImGuiPopupFlags_MouseButtonRight);

    // drag and drop source
    if (ImGui::BeginDragDropSource())
    {
        OnEntityItemDragDropSource_(_entity);
        ImGui::EndDragDropSource();
    }

    // drag and drop target
    if (ImGui::BeginDragDropTarget())
    {
        OnEntityItemDragDropTarget_(_entity);
        ImGui::EndDragDropTarget();
    }

    // click to focus
    if (bIsClicked)
    {
        Editor::SetFocusEntity(_entity);
    }

    // context menu
    if (ImGui::BeginPopup("Menu"))
    {
        if (bHasTag)
        {
            ImGui::TextUnformatted(label);
            ImGui::TextDisabled("uid: %d", uid);
        }
        else
        {
            ImGui::Text("uid: %d", uid);
        }
        ImGui::Separator();

        if (ImGui::MenuItem("create child"))
        {
            Editor::CreateEntityToActiveScene(_entity);
        }

        // disable clone/destroy for root entity
        ImGui::BeginDisabled(_entity.GetSceneNode().IsRoot());
        if (ImGui::MenuItem("clone"))
        {
            Editor::CloneEntityToActiveScene(_entity);
        }
        if (ImGui::MenuItem("clone as child"))
        {
            Editor::CloneEntityToActiveScene(_entity, _entity);
        }
        if (ImGui::MenuItem("destroy"))
        {
            Editor::DestroyEntityToActiveScene(_entity);
        }
        ImGui::EndDisabled();

        ImGui::EndPopup();
    }

    // if node is open, draw children (recursion)
    if (bNodeOpen)
    {
        Entity child = fChild;
        while (child.IsValid())
        {
            Widget_EntityItem_Recursive_(child);
            child = child.GetSceneNode().GetNextSibling();
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void Editor_SceneHierarchy::OnEntityItemDragDropSource_(const Entity _entity) const
{
    const UID uid = _entity.GetUID();
    if (_entity.HasComponent<TagComponent>())
    {
        const TagComponent& tag = _entity.GetComponent<TagComponent>();
        ImGui::TextUnformatted(tag.name.c_str());
        ImGui::TextDisabled("uid: %d", uid);
    }
    else
    {
        ImGui::Text("uid: %d", uid);
    }

    SetEntityDragDropPayload(_entity);
}

void Editor_SceneHierarchy::OnEntityItemDragDropTarget_(const Entity _parent) const
{
    if (const ImGuiPayload* pPayload = AcceptEntityDragDropPayload())
    {
        Entity              entity = ResolveEntityDragDropPayload(pPayload);
        SceneNode& th     = entity.GetSceneNode();
        if (th.IsAncestorOf(_parent))
        {
            Editor::OpenCorePopupDialog("error", "cannot set parent: would create circular reference.");
        }
        else
        {
            th.SetParent(_parent);
        }
    }
}

void Editor_SceneHierarchy::DrawEntityItemChildWindowPopup_() const
{
    if (ImGui::MenuItem("create"))
    {
        Editor::CreateEntityToActiveScene();
    }
}

}   // namespace jam