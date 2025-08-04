#pragma once
#include "Entity.h"

namespace jam
{

struct DeserializeParameter
{
    const Json*   pJson;          // not null!
    const Scene*  pScene;         // not null!
    const Entity* pOnwerEntity;   // not null!
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