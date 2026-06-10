#pragma once
#include "Entity.h"

namespace Jam
{
class SceneChangedEvent;

class Editor_SceneHierarchyWindow
{
public:
    void OnImGuiRender(bool& _pShow);
    void OnEvent(Event& _event);

private:
    void OnSceneChangedEvent_(SceneChangedEvent& _event);

    void Widget_SaveSceneButton_(ImVec2 _btnSize) const;
    void Widget_LoadSceneButton_(ImVec2 _btnSize) const;
    void Widget_EntityItem_Recursive_(Entity _entity);
    void Widget_CreateEntityButton_(ImVec2 _btnSize) const;
    void Widget_DeleteEntityButton_(ImVec2 _btnSize) const;
    void Widget_CloneEntityButton_(ImVec2 _btnSize) const;

    // entity list window
    void OnBeginDragDropTarget_EntityListPanel_() const;
    void Menu_EntityListPanel_() const;

    Scene* m_pScene = nullptr;   // cache;
};

}   // namespace Jam