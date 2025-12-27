#pragma once
#include "Entity.h"
#include "JsonUtilities.h"

namespace jam
{

class Scene;

template<typename T>
struct SerializableComponent
{
    void Serialize_(JsonWriter _writer, const Ref<Scene>& _pScene, Entity _owner) const
    {
        return static_cast<const T*>(this)->Serialize(_writer, _pScene, _owner);
    }

    void Deserialize_(JsonReader _reader, const Ref<Scene>& _pScene, Entity _owner)
    {
        static_cast<T*>(this)->Deserialize(_reader, _pScene, _owner);
    }
};

}   // namespace jam