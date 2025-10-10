#pragma once

// clang-format off
namespace jam { class Scene; class Entity; }
// clang-format on

namespace jam
{

template<typename T>
struct SerializableComponent
{
    Json Serialize_(const Scene* _pScene, Entity _owner) const
    {
        return static_cast<const T*>(this)->Serialize(_pScene, _owner);
    }

    eErrorCode Deserialize_(const Json& _json, const Scene* _pScene, Entity _owner)
    {
        return static_cast<T*>(this)->Deserialize(_json, _pScene, _owner);
    }
};

}   // namespace jam