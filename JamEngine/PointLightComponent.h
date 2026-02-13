#pragma once
#include "ReflectionDB.h"

namespace Jam
{

struct PointLightComponent
{
    JAM_REFLECT_COMPONENT(PointLightComponent);
    JAM_COMPONENT_DISPLAY_NAME_IN_EDITOR("point light component");

    void Serialize(ArchiveWriter& _writer, const Ref<Scene>& _pScene, Entity _owner) const;
    void Deserialize(ArchiveReader& _reader, const Ref<Scene>& _pScene, Entity _owner);
    void DrawEditPanel(const Ref<Scene>& _pScene, Entity _entity);

    Vec3  radiance     = vec3::k_zero;
    float intensity    = 1.0f;
    float fallOffStart = 1.0f;
    float fallOffEnd   = 10.0f;
};

JAM_STATIC_ASSERT_IS_COMPONENT_SERIALIZABLE(PointLightComponent);
JAM_STATIC_ASSERT_IS_COMPONENT_EDITABLE(PointLightComponent);

}   // namespace Jam