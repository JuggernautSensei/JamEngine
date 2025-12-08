#pragma once
#include "Camera.h"
#include "ComponentDB.h"

namespace jam
{

struct CameraComponent :
    SerializableComponent<CameraComponent>,
    EditableComponent<CameraComponent>
{
    JAM_COMPONENT(CameraComponent);

    [[nodiscard]] Json Serialize(const Ref<Scene>& _pScene, Entity _entity) const;
    bool               Deserialize(const Json& _json, const Ref<Scene>& _pScene, Entity _entity);
    void               DrawEditor(const Ref<Scene>& _pScene, Entity _entity);

    Camera camera = {};
    bool   enable = true;
};

}   // namespace jam