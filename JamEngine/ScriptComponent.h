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

    [[nodiscard]] Json Serialize(const Ref<Scene>& _pScene, Entity _entity) const;
    void               Deserialize(const Json& _json, Ref<Scene>& _pScene, Entity _entity);
    void               DrawEditor(Ref<Scene>& _pScene, Entity _entity);

    Ref<Script> pScript = nullptr;
};

}   // namespace jam
