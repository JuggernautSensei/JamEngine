#pragma once
#include "ComponentDB.h"
#include "Script.h"

namespace jam
{

struct ScriptComponent :
    SerializableComponent<ScriptComponent>,
    EditableComponent<ScriptComponent>
{
    JAM_COMPONENT(ScriptComponent);

    void Serialize(JsonWriter& _writer, const Ref<Scene>& _pScene, Entity _owner) const;
    void Deserialize(JsonReader& _reader, const Ref<Scene>& _pScene, Entity _owner);
    void DrawEditor(Ref<Scene>& _pScene, Entity _entity);

    Ref<Script> pScript = nullptr;
};

}   // namespace jam
