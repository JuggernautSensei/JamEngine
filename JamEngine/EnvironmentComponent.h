#pragma once
#include "ComponentDatabase.h"
#include "Environment.h"

namespace Jam
{

struct EnvironmentComponent
{
    JAM_COMPONENT_BODY(EnvironmentComponent);
    JAM_COMPONENT_DISPLAY_NAME("environment component");
    JAM_COMPONENT_PROPERTY_FLAGS(eComponentPropertyFlags::Serializable, eComponentPropertyFlags::Editable, eComponentPropertyFlags::NonRemoveable);

    void Serialize(ArchiveWriter& _writer, Scene* _pScene, Entity _owner) const;
    void Deserialize(ArchiveReader& _reader, Scene* _pScene, Entity _owner);
    void DrawEditPanel(Scene* _pScene, Entity _owner);

    Environment environment = {};
};

}   // namespace Jam
