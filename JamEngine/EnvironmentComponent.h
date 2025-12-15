#pragma once
#include "ComponentDB.h"
#include "EditableComponent.h"
#include "Environment.h"
#include "SerializableComponent.h"

namespace jam
{

struct EnvironmentComponent :
    public SerializableComponent<EnvironmentComponent>,
    public EditableComponent<EnvironmentComponent>,
    public CannotRemovableComponent<EnvironmentComponent>,
    public CannotCreatableInEditorComponent<EnvironmentComponent>
{
    JAM_COMPONENT(EnvironmentComponent);

    Json Serialize(const Ref<Scene>& _pScene, Entity _owner) const;
    void Deserialize(const Json& _json, Ref<Scene>& _pScene, Entity _owner);
    void DrawEditor(Ref<Scene>& _pScene, Entity _entity);

    Environment environment;
};

}   // namespace jam
