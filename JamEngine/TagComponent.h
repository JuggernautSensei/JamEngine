#pragma once
#include "ComponentDatabase.h"

namespace Jam
{

struct TagComponent
{
    JAM_COMPONENT_BODY(TagComponent);
    JAM_COMPONENT_DISPLAY_NAME("tag component");
    JAM_COMPONENT_PROPERTY_FLAGS(eComponentPropertyFlags::Serializable, eComponentPropertyFlags::Editable);

    TagComponent();
    explicit TagComponent(StringView _name);

    [[nodiscard]] bool operator==(const TagComponent& _other) const;
    [[nodiscard]] bool operator!=(const TagComponent& _other) const;
    [[nodiscard]] bool operator==(StringView _name) const;
    [[nodiscard]] bool operator!=(StringView _name) const;

    void Serialize(ArchiveWriter& _writer, Scene* _pScene, Entity _owner) const;
    void Deserialize(ArchiveReader& _reader, Scene* _pScene, Entity _owner);
    void DrawEditPanel(Scene* _pScene, Entity _entity);

    String name = "unknown";
};

}   // namespace Jam