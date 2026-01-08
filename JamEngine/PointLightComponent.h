#pragma once
#include "ComponentDB.h"

namespace jam
{

struct PointLightComponent :
    SerializableComponent<PointLightComponent>,
    EditableComponent<PointLightComponent>
{
    JAM_COMPONENT(PointLightComponent);

    void Serialize(JsonWriter _writer, const Ref<Scene>& _pScene, Entity _owner) const;
    void Deserialize(JsonReader _reader, const Ref<Scene>& _pScene, Entity _owner);
    void Panel_EditComponent(Ref<Scene>& _pScene, Entity _entity);

    Vec3  radiance     = Vec3::Zero;
    float intensity    = 1.0f;
    float fallOffStart = 1.0f;
    float fallOffEnd   = 10.0f;
};

}   // namespace jam