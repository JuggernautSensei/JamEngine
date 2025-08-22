#pragma once

namespace jam
{

class Entity;
class Scene;

template<typename T>
struct ISerializableComponent
{
    NODISCARD Json Serialize_Super() const
    {
        return static_cast<const T*>(this)->Serialize();
    }

    void Deserialize_Super(const Json& _pComponentValueJson, Scene* _pScene, const Entity& _ownerEntity)
    {
        static_cast<T*>(this)->Deserialize(_pComponentValueJson, _pScene, _ownerEntity);
    }
};

}   // namespace jam