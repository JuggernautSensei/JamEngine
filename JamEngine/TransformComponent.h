#pragma once
#include "ComponentDB.h"
#include "TransformHierarchy.h"

namespace jam
{

class TransformComponent :
    SerializableComponent<TransformComponent>,
    EditableComponent<TransformComponent>,
    CannotRemovableComponent<TransformComponent>
{
    JAM_COMPONENT(TransformComponent);

    TransformComponent() = default;
    explicit TransformComponent(const TransformHierarchy& _transformHierarchy)
        : transformHierarchy(_transformHierarchy)
    {
    }

    [[nodiscard]] Json Serialize(const Ref<Scene>& _pScene, Entity _entity) const;
    bool               Deserialize(const Json& _json, const Ref<Scene>& _pScene, Entity _entity);
    void               DrawEditor(Ref<Scene>& _pScene, Entity _entity);

    TransformHierarchy transformHierarchy = {};
};

}   // namespace jam