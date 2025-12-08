#pragma once
#include "ComponentDB.h"

namespace jam
{

struct SpotLightComponent :
    SerializableComponent<SpotLightComponent>,
    EditableComponent<SpotLightComponent>
{
    JAM_COMPONENT(SpotLightComponent);
    [[nodiscard]] Json Serialize(const Ref<Scene>& _pScene, Entity _entity) const;
    bool Deserialize(const Json& _json, Ref<Scene>& _pScene, Entity _entity);
    void               DrawEditor(Ref<Scene>& _pScene, Entity _entity);

    Vec3  radiance         = Vec3::Zero;
    float intensity        = 1.f;
    float fallOffStart     = 1.f;
    float fallOffEnd       = 10.f;
    float innerConeHalfRad = ToRad(15.f);
    float outerConeHalfRad = ToRad(30.f);
};

}