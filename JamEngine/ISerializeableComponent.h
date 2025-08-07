#pragma once

namespace jam
{

class Entity;
class Scene;

struct DeserializeParameter
{
    const Json*   pJson;    // not null!
    const Entity* pOwner;   // not null!
    Scene*        pScene;   // not null!
};

template<typename T>
struct ISerializableComponent
{
    NODISCARD Json Serialize_Super() const
    {
        return static_cast<const T*>(this)->Serialize();
    }

    void Deserialize_Super(const DeserializeParameter& _param)
    {
        static_cast<T*>(this)->Deserialize(_param);
    }
};

}   // namespace jam