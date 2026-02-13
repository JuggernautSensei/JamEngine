#pragma once
#include "ReflectionDB.h"
#include "Environment.h"

namespace Jam
{

struct EnvironmentComponent
{
    JAM_REFLECT_COMPONENT(EnvironmentComponent);
    JAM_COMPONENT_TAG(eComponentTag::NonRemoveable, eComponentTag::CannotCreateInEditor);
    JAM_COMPONENT_DISPLAY_NAME_IN_EDITOR("environment component");

    void Serialize(ArchiveWriter& _writer, const Ref<Scene>& _pScene, Entity _owner) const;
    void Deserialize(ArchiveReader& _reader, const Ref<Scene>& _pScene, Entity _owner);
    void DrawEditPanel(const Ref<Scene>& _pScene, Entity _owner);

    Environment environment = {};
};

JAM_STATIC_ASSERT_IS_COMPONENT_SERIALIZABLE(EnvironmentComponent);
JAM_STATIC_ASSERT_IS_COMPONENT_EDITABLE(EnvironmentComponent);

}   // namespace Jam
