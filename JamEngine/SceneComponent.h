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

    void Serialize(JsonWriter& _writer, const Ref<Scene>& _pScene, Entity _owner) const;
    void Deserialize(JsonReader& _reader, const Ref<Scene>& _pScene, Entity _owner);
    void DrawEditor(Ref<Scene>& _pScene, Entity _entity);

    SceneNode node = {};
};

}   // namespace jam