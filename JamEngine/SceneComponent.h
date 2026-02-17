#pragma once
#include "ComponentDatabase.h"
#include "SceneNode.h"

namespace Jam
{

struct SceneComponent
{
    JAM_COMPONENT_BODY(SceneComponent);
    JAM_COMPONENT_METADATA_EDITOR_DISPLAY_NAME("scene component");
    JAM_COMPONENT_METADATA_PROPERTY_FLAGS(eComponentPropertyFlags::Serializable, eComponentPropertyFlags::Editable, eComponentPropertyFlags::NonRemoveable);

    void Serialize(ArchiveWriter& _writer, const Ref<Scene>& _pScene, Entity _owner) const;
    void Deserialize(ArchiveReader& _reader, const Ref<Scene>& _pScene, Entity _owner);
    void DrawEditPanel(const Ref<Scene>& _pScene, Entity _owner);

    SceneNode node = {};
};

}   // namespace Jam