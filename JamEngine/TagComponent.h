#pragma once
#include "ComponentDB.h"

namespace jam
{

struct TagComponent :
    SerializableComponent<TagComponent>,
    EditableComponent<TagComponent>
{
    JAM_COMPONENT(TagComponent);

    TagComponent();
    explicit TagComponent(std::string_view _name);

    [[nodiscard]] bool operator==(const TagComponent& _other) const;
    [[nodiscard]] bool operator!=(const TagComponent& _other) const;
    [[nodiscard]] bool operator==(std::string_view _name) const;
    [[nodiscard]] bool operator!=(std::string_view _name) const;

    [[nodiscard]] Json Serialize(const Ref<Scene>& _pScene, Entity _entity) const;
    void               Deserialize(const Json& _json, const Ref<Scene>& _pScene, Entity _entity);
    void               DrawEditor(Ref<Scene>& _pScene, Entity _entity);

    std::string name = "unknown";
};

}   // namespace jam