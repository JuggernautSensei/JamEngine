#pragma once
#include "Camera.h"
#include "ComponentDB.h"

namespace jam
{

struct CameraComponent :
    SerializableComponent<CameraComponent>,
    EditableComponent<CameraComponent>
{
    JAM_COMPONENT(CameraComponent);

    void Serialize(JsonWriter& _writer, const Ref<Scene>& _pScene, Entity _owner) const;
    void Deserialize(JsonReader& _reader, const Ref<Scene>& _pScene, Entity _owner);
    void DrawEditor(const Ref<Scene>& _pScene, Entity _entity);

    Camera camera   = {};
    bool   bEnabled = true;
};

}   // namespace jam