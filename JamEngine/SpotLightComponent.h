#pragma once
#include "ReflectionDB.h"

namespace Jam
{

struct SpotLightComponent
{
    JAM_REFLECT_COMPONENT(SpotLightComponent);
    JAM_COMPONENT_DISPLAY_NAME_IN_EDITOR("spot light component");

    void Serialize(ArchiveWriter& _writer, const Ref<Scene>& _pScene, Entity _owner) const;
    void Deserialize(ArchiveReader& _reader, const Ref<Scene>& _pScene, Entity _owner);
    void DrawEditPanel(const Ref<Scene>& _pScene, Entity _entity);

    Vec3  radiance         = vec3::k_zero;
    float intensity        = 1.f;
    float fallOffStart     = 1.f;
    float fallOffEnd       = 10.f;
    float innerConeHalfRad = ToRad(15.f);
    float outerConeHalfRad = ToRad(30.f);
};

JAM_STATIC_ASSERT_IS_COMPONENT_SERIALIZABLE(SpotLightComponent);
JAM_STATIC_ASSERT_IS_COMPONENT_EDITABLE(SpotLightComponent);

}   // namespace Jam