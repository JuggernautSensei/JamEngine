#pragma once
#include "ComponentDB.h"

namespace jam
{

struct TagComponent :
    SerializableComponent<TagComponent>,
    EditableComponent<TagComponent>
{
    JAM_COMPONENT(TagComponent);

    TagComponent() = default;
    explicit TagComponent(const std::string_view _name)
        : name(_name)
    {
    }

    [[nodiscard]] bool operator==(const TagComponent& _other) const { return name == _other.name; }
    [[nodiscard]] bool operator!=(const TagComponent& _other) const { return !(*this == _other); }
    [[nodiscard]] bool operator==(const std::string_view _name) const { return name == _name; }
    [[nodiscard]] bool operator!=(const std::string_view _name) const { return !(*this == _name); }

    [[nodiscard]] Json Serialize(const Ref<Scene>& _pScene, Entity _entity) const;
    bool               Deserialize(const Json& _json, const Ref<Scene>& _pScene, Entity _entity);
    void               DrawEditor(Ref<Scene>& _pScene, Entity _entity);

    std::string name = "unknown";
};

}   // namespace jam