#pragma once
#include "Entity.h"

namespace jam
{

class Editor_SceneHierarchy
{
public:
    void OnImGuiRender(bool& _pShow);

private:
    // entity item
    void Widget_EntityItem_Recursive_(Entity _entity);

    // entity list window
    void OnBeginDragDropTarget_EntityListPanel_() const;
    void Menu_EntityListPanel_() const;
};

}   // namespace jam