#pragma once
#include "ComponentDB.h"

namespace jam
{

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

    [[nodiscard]] Json Serialize(const Ref<Scene>& _pScene, Entity _entity) const;
    void               Deserialize(const Json& _json, Ref<Scene>& _pScene, Entity _entity);
    void               DrawEditor(Ref<Scene>& _pScene, Entity _entity);

    Renderable renderable;
};

}   // namespace jam