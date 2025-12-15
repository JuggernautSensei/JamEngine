#pragma once
#include "Entity.h"
#include "Model.h"

namespace jam
{

class Editor_SceneHierarchy
{
public:
    void OnImGuiRender(bool& _pShow);

private:
    // listeners
    void OnEntityListChildWindowDragDropTarget_();
    void OnEntityItemDragDropSource_(Entity _entity) const;
    void OnEntityItemDragDropTarget_(Entity _parent) const;

    // build entity tree from model
    void CreateEntityTreeFromModel_(const Ref<Model>& _model);
    void CreateEntityTreeFromModelNode_Recursive_(const ModelNode& _node, Entity _parentOrNull, const Vector<fs::path>& _meshs, const Vector<fs::path>& _mats);

    // widgets
    void Widget_EntityItem_Recursive_(Entity _entity);
    void Widget_SaveSceneButton_(ImVec2 btnSize) const;
    void Widget_LoadSceneButton_(ImVec2 btnSize) const;
    void Widget_CloneEntityButton_(ImVec2 btnSize) const;
    void Widget_DestroyEntity_(ImVec2 btnSize) const;
    void Widget_CreateEntityButton_(ImVec2 btnSize) const;

    // popups
    void DrawEntityItemChildWindowPopup_() const;
};

}   // namespace jam