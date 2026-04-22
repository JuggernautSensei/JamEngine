#pragma once
#include "Archive.h"

namespace Jam
{

class Entity;
class Scene;

// 필수 메타데이터

#define JAM_COMPONENT_METADATA_ESSENTIAL_MARKER() \
    static constexpr bool COMPONENT_METADATA_ESSENTIAL_MARKER = true

template<typename TCmp>
concept HasComponentMetadataEssentialMarker =
    requires { { TCmp::COMPONENT_METADATA_ESSENTIAL_MARKER } -> std::convertible_to<bool>; }
    && TCmp::COMPONENT_METADATA_ESSENTIAL_MARKER;

// 메타데이터: 에디터에서 표시되는 이름

#define JAM_COMPONENT_METADATA_EDITOR_DISPLAY_NAME(_name) \
    static constexpr StringView COMPONENT_METADATA_EDITOR_DISPLAY_NAME { _name }

template<typename TCmp>
concept HasComponentMetadataEditorDisplayName =
    requires { TCmp::COMPONENT_METADATA_EDITOR_DISPLAY_NAME; };

template<typename TCmp>
    requires HasComponentMetadataEditorDisplayName<TCmp>
constexpr StringView GetComponentMetadataEditorDisplayName()
{
    return TCmp::COMPONENT_METADATA_EDITOR_DISPLAY_NAME;
}

// 속성 플래그

enum class eComponentPropertyFlags : UInt32
{
    None               = 0,
    Serializable       = 1 << 0,
    Editable           = 1 << 1,
    NonRemoveable      = 1 << 2,
    NonCreatableByUser = 1 << 3,   // 에디터 "Add Component" 미노출, 코드에서는 생성 가능
};

#define JAM_COMPONENT_METADATA_PROPERTY_FLAGS(...) \
    static constexpr Flags<eComponentPropertyFlags> COMPONENT_METADATA_PROPERTY_FLAGS { __VA_ARGS__ }

template<typename TCmp, eComponentPropertyFlags kFlags>
concept HasComponentPropertyFlag =
    requires { { TCmp::COMPONENT_METADATA_PROPERTY_FLAGS } -> std::convertible_to<Flags<eComponentPropertyFlags>>; }
    && TCmp::COMPONENT_METADATA_PROPERTY_FLAGS.IsEnabled(kFlags);

// 검증

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

// 검증

#define JAM_STATIC_ASSERT_COMPONENT_PROPERTY_VALIDATION_CHECKS(TCmp)              \
    static_assert(                                                                \
        !HasComponentPropertyFlag<TCmp, eComponentPropertyFlags::Serializable>    \
            || CheckComponentSerializablePropertyValidation<TCmp>,                \
        "Component '" #TCmp "': Serializable 플래그에 맞는 시그니처가 없습니다"); \
    static_assert(                                                                \
        !HasComponentPropertyFlag<TCmp, eComponentPropertyFlags::Editable>        \
            || CheckComponentEditablePropertyValidation<TCmp>,                    \
        "Component '" #TCmp "': Editable 플래그에 맞는 시그니처가 없습니다")

}   // namespace Jam