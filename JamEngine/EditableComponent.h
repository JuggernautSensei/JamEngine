#pragma once

// clang-format off
namespace jam { class Scene; class Entity; }
// clang-format on

namespace jam
{

template<typename T>
class EditableComponent
{
    void DrawEditor_(const Scene* _pScene, const Entity& _entity)
    {
        static_cast<T*>(this)->DrawEditor(_pScene, _entity);
    }
};

}   // namespace jam