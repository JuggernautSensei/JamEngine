#pragma once
#include "ComponentDatabase.h"
#include "Mesh.h"
#include "Material.h"

namespace Jam
{

struct RenderableComponent
{
    JAM_COMPONENT_BODY(RenderableComponent);
    JAM_COMPONENT_METADATA_EDITOR_DISPLAY_NAME("renderable component");
    JAM_COMPONENT_METADATA_PROPERTY_FLAGS(eComponentPropertyFlags::Serializable, eComponentPropertyFlags::Editable);

    void Serialize(ArchiveWriter& _writer, const Ref<Scene>& _pScene, Entity _uid) const;
    void Deserialize(ArchiveReader& _reader, const Ref<Scene>& _pScene, Entity _uid);
    void DrawEditPanel(const Ref<Scene>& _pScene, Entity _uid);

    Ref<Mesh>     pMesh     = nullptr;
    Ref<Material> pMaterial = nullptr;
};

}   // namespace Jam