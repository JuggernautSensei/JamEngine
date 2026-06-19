#pragma once
#include "ComponentDatabase.h"

namespace Jam
{

class Mesh;
class Material;

struct RenderableComponent
{
    JAM_COMPONENT_BODY(RenderableComponent);
    JAM_COMPONENT_DISPLAY_NAME("renderable component");
    JAM_COMPONENT_PROPERTY_FLAGS(eComponentPropertyFlags::Serializable, eComponentPropertyFlags::Editable);

    void Serialize(ArchiveWriter& _writer, Scene* _pScene, Entity _uid) const;
    void Deserialize(ArchiveReader& _reader, Scene* _pScene, Entity _uid);
    void DrawEditPanel(Scene* _pScene, Entity _uid);

    Ref<Mesh>     pMesh     = nullptr;
    Ref<Material> pMaterial = nullptr;
};

}   // namespace Jam