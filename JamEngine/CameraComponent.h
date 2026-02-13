#pragma once
#include "Camera.h"
#include "ComponentMetadata.h"
#include "ComponentTag.h"
#include "ComponentProperties.h"
#include "ReflectionDB.h"

namespace Jam
{

struct CameraComponent
{
    JAM_REFLECT_COMPONENT(CameraComponent);
    JAM_COMPONENT_DISPLAY_NAME_IN_EDITOR("camera component");

    void Serialize(ArchiveWriter& _writer, const Ref<Scene>& _pScene, Entity _owner) const;
    void Deserialize(ArchiveReader& _reader, const Ref<Scene>& _pScene, Entity _owner);
    void DrawEditPanel(const Ref<Scene>& _pScene, Entity _owner);

    Camera camera   = {};
    bool   bEnabled = true;
};

JAM_STATIC_ASSERT_IS_COMPONENT_SERIALIZABLE(CameraComponent);
JAM_STATIC_ASSERT_IS_COMPONENT_EDITABLE(CameraComponent);

}   // namespace Jam