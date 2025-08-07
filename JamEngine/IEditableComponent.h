#pragma once

namespace jam
{

class EditorLayer;
class Entity;

struct DrawEditorParameter
{
    EditorLayer* pEditorLayer;   // not null!
    Entity*      pOwnerEntity;   // not null!
};

template<typename T>
class IEditableComponent
{
public:
    void DrawEditor_Super(const DrawEditorParameter& _param)
    {
        static_cast<T*>(this)->DrawEditor(_param);
    }
};

}   // namespace jam
