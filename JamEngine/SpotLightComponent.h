#pragma once
#include "ComponentDatabase.h"

namespace Jam
{

struct SpotLightComponent
{
    JAM_COMPONENT_BODY(SpotLightComponent);
    JAM_COMPONENT_METADATA_EDITOR_DISPLAY_NAME("spot light component");
    JAM_COMPONENT_METADATA_PROPERTY_FLAGS(eComponentPropertyFlags::Serializable, eComponentPropertyFlags::Editable);

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

}   // namespace Jam