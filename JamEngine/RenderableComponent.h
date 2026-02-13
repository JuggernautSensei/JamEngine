#pragma once
#include "ReflectionDB.h"
#include "ComponentProperties.h"
#include "Mesh.h"
#include "Material.h"

namespace Jam
{

struct RenderableComponent
{
    JAM_REFLECT_COMPONENT(RenderableComponent);
    JAM_COMPONENT_DISPLAY_NAME_IN_EDITOR("renderable component");

    void Serialize(ArchiveWriter& _writer, const Ref<Scene>& _pScene, Entity _uid) const;
    void Deserialize(ArchiveReader& _reader, const Ref<Scene>& _pScene, Entity _uid);
    void DrawEditPanel(const Ref<Scene>& _pScene, Entity _uid);

    Ref<Mesh>     pMesh     = nullptr;
    Ref<Material> pMaterial = nullptr;
};

JAM_STATIC_ASSERT_IS_COMPONENT_SERIALIZABLE(RenderableComponent);
JAM_STATIC_ASSERT_IS_COMPONENT_EDITABLE(RenderableComponent);

}   // namespace Jam