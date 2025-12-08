#pragma once
#include "ComponentDB.h"
#include "VisibleItem.h"

namespace jam
{

struct RenderableComponent :
    SerializableComponent<RenderableComponent>,
    EditableComponent<RenderableComponent>
{
    JAM_COMPONENT(RenderableComponent);

    [[nodiscard]] Json Serialize(const Ref<Scene>& _pScene, Entity _entity) const;
    bool               Deserialize(const Json& _json, Ref<Scene>& _pScene, Entity _entity);
    void               DrawEditor(Ref<Scene>& _pScene, Entity _entity);

    Renderable renderable;
};

}   // namespace jam