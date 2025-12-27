#pragma once
#include "ComponentDB.h"

namespace jam
{

class Mesh;
class Material;

struct Renderable
{
    Ref<Mesh>     pMesh     = nullptr;
    Ref<Material> pMaterial = nullptr;
};

struct RenderableComponent :
    SerializableComponent<RenderableComponent>,
    EditableComponent<RenderableComponent>
{
    JAM_COMPONENT(RenderableComponent);

    void Serialize(JsonWriter _writer, const Ref<Scene>& _pScene, Entity _owner) const;
    void Deserialize(JsonReader _reader, const Ref<Scene>& _pScene, Entity _owner);
    void DrawEditor(Ref<Scene>& _pScene, Entity _entity);

    Renderable renderable;
};

}   // namespace jam