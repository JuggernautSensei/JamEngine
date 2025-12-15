#pragma once
#include "ComponentDB.h"

namespace jam
{

struct PointLightComponent :
    SerializableComponent<PointLightComponent>,
    EditableComponent<PointLightComponent>
{
    JAM_COMPONENT(PointLightComponent);

    [[nodiscard]] Json Serialize(const Ref<Scene>& _pScene, Entity _entity) const;
    void Deserialize(const Json& _json, Ref<Scene>& _pScene, Entity _entity);
    void               DrawEditor(Ref<Scene>& _pScene, Entity _entity);

    Vec3  radiance     = Vec3::Zero;
    float intensity    = 1.0f;
    float fallOffStart = 1.0f;
    float fallOffEnd   = 10.0f;
};

}   // namespace jam