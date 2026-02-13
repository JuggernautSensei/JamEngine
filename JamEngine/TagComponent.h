#pragma once
#include "ReflectionDB.h"

namespace Jam
{

struct TagComponent
{
    JAM_REFLECT_COMPONENT(TagComponent);
    JAM_COMPONENT_DISPLAY_NAME_IN_EDITOR("tag component");

    TagComponent();
    explicit TagComponent(StringView _name);

    [[nodiscard]] bool operator==(const TagComponent& _other) const;
    [[nodiscard]] bool operator!=(const TagComponent& _other) const;
    [[nodiscard]] bool operator==(StringView _name) const;
    [[nodiscard]] bool operator!=(StringView _name) const;

    void Serialize(ArchiveWriter& _writer, const Ref<Scene>& _pScene, Entity _owner) const;
    void Deserialize(ArchiveReader& _reader, const Ref<Scene>& _pScene, Entity _owner);
    void DrawEditPanel(const Ref<Scene>& _pScene, Entity _entity);

    String name = "unknown";
};

JAM_STATIC_ASSERT_IS_COMPONENT_SERIALIZABLE(TagComponent);
JAM_STATIC_ASSERT_IS_COMPONENT_EDITABLE(TagComponent);

}   // namespace Jam