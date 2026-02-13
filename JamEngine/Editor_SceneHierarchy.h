#pragma once
#include "Entity.h"
#include "Scene.h"

namespace Jam
{

class Editor_SceneHierarchy
{
public:
    void OnImGuiRender(bool& _pShow);

private:
    void Widget_SaveSceneButton_(ImVec2 _btnSize) const;
    void Widget_LoadSceneButton_(ImVec2 _btnSize) const;
    void Widget_EntityItem_Recursive_(Entity _entity);
    void Widget_CreateEntityButton_(ImVec2 _btnSize) const;
    void Widget_DeleteEntityButton_(ImVec2 _btnSize) const;
    void Widget_CloneEntityButton_(ImVec2 _btnSize) const;

    // entity list window
    void OnBeginDragDropTarget_EntityListPanel_() const;
    void Menu_EntityListPanel_() const;
};

}   // namespace Jam