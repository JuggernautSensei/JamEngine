#pragma once

namespace jam
{

class EditorLayer;
class Entity;

template<typename T>
class IEditableComponent
{
public:
    void DrawEditor_Super(EditorLayer* const _pEditorLayer, Scene* _pScene, const Entity& _owner)
    {
        static_cast<T*>(this)->DrawEditor(_pEditorLayer, _pScene, _owner);
    }
};

}   // namespace jam
