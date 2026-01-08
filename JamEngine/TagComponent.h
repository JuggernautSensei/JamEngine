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
    explicit TagComponent(StringView _name);

    [[nodiscard]] bool operator==(const TagComponent& _other) const;
    [[nodiscard]] bool operator!=(const TagComponent& _other) const;
    [[nodiscard]] bool operator==(StringView _name) const;
    [[nodiscard]] bool operator!=(StringView _name) const;

    void Serialize(JsonWriter _writer, const Ref<Scene>& _pScene, Entity _owner) const;
    void Deserialize(JsonReader _reader, const Ref<Scene>& _pScene, Entity _owner);
    void Panel_EditComponent(Ref<Scene>& _pScene, Entity _entity);

    std::string name = "unknown";
};

}   // namespace jam