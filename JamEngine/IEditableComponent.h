#pragma once
#include "Entity.h"

namespace jam
{

class EditorLayer;

struct DrawComponentEditorParameter
{
    EditorLayer* pEditorLayer;   // not null!
    Entity*      pOwnerEntity;   // not null!
};

template<typename T>
class IEditableComponent
{
public:
    void DrawComponentEditor_Super(const DrawComponentEditorParameter& _param)
    {
        static_cast<T*>(this)->DrawComponentEditor(_param);
    }
};

}   // namespace jam
