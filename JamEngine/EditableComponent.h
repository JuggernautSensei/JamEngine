#pragma once
#include "Entity.h"

namespace jam
{

class Scene;

template<typename T>
struct EditableComponent
{
    void Panel_EditComponent_(Ref<Scene>& _pScene, Entity _entity)
    {
        static_cast<T*>(this)->Panel_EditComponent(_pScene, _entity);
    }
};

}   // namespace jam