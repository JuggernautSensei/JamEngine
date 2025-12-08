#pragma once
#include "Entity.h"

namespace jam
{

class Scene;

template<typename T>
class EditableComponent
{
    void DrawEditor_(Ref<Scene>& _pScene, Entity _entity)
    {
        static_cast<T*>(this)->DrawEditor(_pScene, _entity);
    }
};

}   // namespace jam