#pragma once
#include "ComponentDB.h"
#include "SceneNode.h"

namespace jam
{

class SceneComponent :
    SerializableComponent<SceneComponent>,
    EditableComponent<SceneComponent>,
    CannotRemovableComponent<SceneComponent>
{
    JAM_COMPONENT(SceneComponent);

    SceneComponent();
    explicit SceneComponent(const SceneNode& _node);

    [[nodiscard]] Json Serialize(const Ref<Scene>& _pScene, Entity _entity) const;
    void Deserialize(const Json& _json, const Ref<Scene>& _pScene, Entity _entity);
    void               DrawEditor(Ref<Scene>& _pScene, Entity _entity);

    SceneNode node = {};
};

}   // namespace jam