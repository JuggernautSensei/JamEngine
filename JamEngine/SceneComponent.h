#pragma once
#include "ComponentProperties.h"
#include "ReflectionDB.h"
#include "SceneNode.h"

namespace Jam
{

struct SceneComponent
{
    JAM_REFLECT_COMPONENT(SceneComponent);
    JAM_COMPONENT_TAG(eComponentTag::NonRemoveable, eComponentTag::CannotCreateInEditor);
    JAM_COMPONENT_DISPLAY_NAME_IN_EDITOR("scene component");

    void Serialize(ArchiveWriter& _writer, const Ref<Scene>& _pScene, Entity _owner) const;
    void Deserialize(ArchiveReader& _reader, const Ref<Scene>& _pScene, Entity _owner);
    void DrawEditPanel(const Ref<Scene>& _pScene, Entity _owner);

    SceneNode node = {};
};

JAM_STATIC_ASSERT_IS_COMPONENT_SERIALIZABLE(SceneComponent);
JAM_STATIC_ASSERT_IS_COMPONENT_EDITABLE(SceneComponent);

}   // namespace Jam