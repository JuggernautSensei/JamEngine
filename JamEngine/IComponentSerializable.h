#pragma once
#include "Entity.h"

namespace jam
{

struct ComponentDeserializeData
{
    Json   json;
    Scene* pScene;
    Entity ownerEntity;
};

template<typename T>
struct IComponentSerializable
{
    NODISCARD Json SerializeComponent()
    {
        return static_cast<T*>(this)->Serialize();
    }

    void DeserializeComponent(const ComponentDeserializeData& _data)
    {
        static_cast<T*>(this)->Deserialize(_data);
    }
};

}   // namespace jam