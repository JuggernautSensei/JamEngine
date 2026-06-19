#pragma once
#include "ComponentDatabase.h"

namespace Jam
{

struct SpotLightComponent
{
    JAM_COMPONENT_BODY(SpotLightComponent);
    JAM_COMPONENT_DISPLAY_NAME("spot light component");
    JAM_COMPONENT_PROPERTY_FLAGS(eComponentPropertyFlags::Serializable, eComponentPropertyFlags::Editable);

    void Serialize(ArchiveWriter& _writer, Scene* _pScene, Entity _owner) const;
    void Deserialize(ArchiveReader& _reader, Scene* _pScene, Entity _owner);
    void DrawEditPanel(Scene* _pScene, Entity _entity);

    Vec3  radiance         = Vec3::kZero;
    float intensity        = 1.f;
    float fallOffStart     = 1.f;
    float fallOffEnd       = 10.f;
    float innerConeHalfRad = ToRad(15.f);
    float outerConeHalfRad = ToRad(30.f);
};

}   // namespace Jam