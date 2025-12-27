#pragma once
#include "ComponentDB.h"

namespace jam
{

struct SpotLightComponent :
    SerializableComponent<SpotLightComponent>,
    EditableComponent<SpotLightComponent>
{
    JAM_COMPONENT(SpotLightComponent);

    void Serialize(JsonWriter _writer, const Ref<Scene>& _pScene, Entity _owner) const;
    void Deserialize(JsonReader _reader, const Ref<Scene>& _pScene, Entity _owner);
    void DrawEditor(Ref<Scene>& _pScene, Entity _entity);

    Vec3  radiance         = Vec3::Zero;
    float intensity        = 1.f;
    float fallOffStart     = 1.f;
    float fallOffEnd       = 10.f;
    float innerConeHalfRad = ToRad(15.f);
    float outerConeHalfRad = ToRad(30.f);
};

}   // namespace jam