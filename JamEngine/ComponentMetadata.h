#pragma once
#include "Archive.h"

namespace Jam
{

class Entity;
class Scene;

// essential marker for runtime reflection
#define JAM_COMPONENT_ESSENTIAL_MARKER() \
    static constexpr bool COMPONENT_ESSENTIAL_MARKER = true

// check
template<typename TCmp>
concept HasComponentEssentialMarker =
    requires { { TCmp::COMPONENT_ESSENTIAL_MARKER } -> std::convertible_to<bool>; }
    && TCmp::COMPONENT_ESSENTIAL_MARKER;

// editor display name
#define JAM_COMPONENT_EDITOR_DISPLAY_NAME(_name) \
    static constexpr StringView COMPONENT_EDITOR_DISPLAY_NAME { _name }

// check
template<typename TCmp>
concept HasComponentEditorDisplayName =
    requires { TCmp::COMPONENT_EDITOR_DISPLAY_NAME; };

// editor display name getter
template<typename TCmp>
    requires HasComponentEditorDisplayName<TCmp>
constexpr StringView GetComponentEditorDisplayName()
{
    return TCmp::COMPONENT_EDITOR_DISPLAY_NAME;
}

// component property flags
enum class eComponentPropertyFlags : UInt32
{
    None                 = 0,
    Serializable         = 1 << 0,
    Editable             = 1 << 1,
    NonRemoveable        = 1 << 2,
    NonCreatableOnEditor = 1 << 3,   // 에디터 "Add Component" 미노출, 코드에서는 생성 가능
};

// component property flags declaration
#define JAM_COMPONENT_PROPERTY_FLAGS(...) \
    static constexpr Flags<eComponentPropertyFlags> COMPONENT_PROPERTY_FLAGS { __VA_ARGS__ }

// check
template<typename TCmp, eComponentPropertyFlags kFlags>
concept HasComponentPropertyFlag =
    requires { { TCmp::COMPONENT_PROPERTY_FLAGS } -> std::convertible_to<Flags<eComponentPropertyFlags>>; }
    && TCmp::COMPONENT_PROPERTY_FLAGS.IsEnabled(kFlags);

// property validation checks
template<typename TCmp>
concept CheckComponentSerializablePropertyValidation =
    HasComponentPropertyFlag<TCmp, eComponentPropertyFlags::Serializable>
    && requires(TCmp& cmp, Scene* scene, const Entity& owner, ArchiveWriter& writer, ArchiveReader& reader) {
           cmp.Serialize(writer, scene, owner);
           cmp.Deserialize(reader, scene, owner);
       };

template<typename TCmp>
concept CheckComponentEditablePropertyValidation =
    HasComponentPropertyFlag<TCmp, eComponentPropertyFlags::Editable>
    && requires(TCmp& cmp, Scene* scene, const Entity& owner) { cmp.DrawEditPanel(scene, owner); };

// short cut
#define JAM_STATIC_ASSERT_CHECK_COMPONENT_PROPERTY_VALIDATIONS(TCmp)                                                                                                                                                      \
    static_assert(!HasComponentPropertyFlag<TCmp, eComponentPropertyFlags::Serializable> || CheckComponentSerializablePropertyValidation<TCmp>, "Component '" #TCmp "': Serializable 플래그에 맞는 시그니처가 없습니다"); \
    static_assert(!HasComponentPropertyFlag<TCmp, eComponentPropertyFlags::Editable> || CheckComponentEditablePropertyValidation<TCmp>, "Component '" #TCmp "': Editable 플래그에 맞는 시그니처가 없습니다")

}   // namespace Jam