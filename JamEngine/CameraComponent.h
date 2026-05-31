#pragma once
#include "Camera.h"
#include "ComponentDatabase.h"

namespace Jam
{

struct CameraComponent
{
    JAM_COMPONENT_BODY(CameraComponent);
    JAM_COMPONENT_EDITOR_DISPLAY_NAME("camera component");
    JAM_COMPONENT_PROPERTY_FLAGS(eComponentPropertyFlags::Serializable, eComponentPropertyFlags::Editable);

    void Serialize(ArchiveWriter& _writer, Scene* _pScene, Entity _owner) const;
    void Deserialize(ArchiveReader& _reader, Scene* _pScene, Entity _owner);
    void DrawEditPanel(Scene* _pScene, Entity _owner);

    Camera camera   = {};
    bool   bEnabled = true;
};

}   // namespace Jam
