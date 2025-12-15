#pragma once
#include "Entity.h"

namespace jam
{

class Scene;

template<typename T>
struct SerializableComponent
{
    Json Serialize_(const Ref<Scene>& _pScene, Entity _owner) const
    {
        return static_cast<const T*>(this)->Serialize(_pScene, _owner);
    }

    void Deserialize_(const Json& _json, Ref<Scene>& _pScene, Entity _owner)
    {
        static_cast<T*>(this)->Deserialize(_json, _pScene, _owner);
    }
};

}   // namespace jam