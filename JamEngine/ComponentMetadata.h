#pragma once

namespace Jam
{

class Entity;
class Scene;

/////////// Essencial Marker //////////

#define JAM_COMPONENT_METADATA_ESSENTIAL_MARKER() \
    static constexpr bool JAM_COMPONENT_METADATA_ESSENTIAL_MARKER = true

template<typename TCmp>
concept HasComponentMetadataEssentialMarker = requires {
    TCmp::JAM_COMPONENT_METADATA_ESSENTIAL_MARKER;
};

////////// Editor Display Name //////////
// provide more readable display name for editor, if not provided, the component type name will be used as fallback

#define JAM_COMPONENT_METADATA_EDITOR_DISPLAY_NAME(_name) \
    static constexpr StringView JAM_COMPONENT_METADATA_EDITOR_DISPLAY_NAME { _name }

template<typename TCmp>
concept HasComponentMetadataEditorDisplayName = requires {
    TCmp::JAM_COMPONENT_METADATA_EDITOR_DISPLAY_NAME;
};

template<typename TCmp>
    requires HasComponentMetadataEditorDisplayName<TCmp>
constexpr StringView GetComponentMetadataEditorDisplayName()
{
    return TCmp::JAM_COMPONENT_METADATA_EDITOR_DISPLAY_NAME;
}

////////// Property //////////

enum class eComponentPropertyFlags : UInt32
{
    None               = 0,
    Serializable       = 1 << 0,
    Editable           = 1 << 1,
    NonRemoveable      = 1 << 2,
    NonCreatableByUser = 1 << 3   // do not show this component in "add component" menu in editor, but it can still be created by code
};

#define JAM_COMPONENT_METADATA_PROPERTY_FLAGS(...) \
    static constexpr Flags<eComponentPropertyFlags> JAM_COMPONENT_PROPERTY_FLAGS { __VA_ARGS__ }

template<typename TCmp, eComponentPropertyFlags k_flags>
constexpr bool HasComponentPropertyV = TCmp::JAM_COMPONENT_PROPERTY_FLAGS.IsEnabled(k_flags);

template<typename TCmp>
concept CheckComponentSerializablePropertyValidation =
    HasComponentPropertyV<TCmp, eComponentPropertyFlags::Serializable>
    && requires(TCmp _cmp, const Ref<Scene>& pScene, const Entity& owner, ArchiveWriter& _writer, ArchiveReader& _reader) {
           _cmp.Serialize(_writer, pScene, owner);
           _cmp.Deserialize(_reader, pScene, owner);
       } && !requires(TCmp _cmp, const Ref<Scene>& pScene, const Entity& owner, const ArchiveWriter& _writer) {
           _cmp.Serialize(_writer, pScene, owner);
       } && !requires(TCmp _cmp, const Ref<Scene>& pScene, const Entity& owner, const ArchiveReader& _reader) {
           _cmp.Deserialize(_reader, pScene, owner);
       } && !requires(const TCmp _cmp, const Ref<Scene>& pScene, const Entity& owner, ArchiveReader& _reader) {
           _cmp.Deserialize(_reader, pScene, owner);
       };

template<typename TCmp>
concept CheckComponentEditablePropertyValidation =
    HasComponentPropertyV<TCmp, eComponentPropertyFlags::Editable>
    && requires(TCmp _cmp, const Ref<Scene>& pScene, const Entity& owner) {
           _cmp.DrawEditPanel(pScene, owner);
       } && !requires(const TCmp _cmp, const Ref<Scene>& pScene, const Entity& owner) {
           _cmp.DrawEditPanel(pScene, owner);
       };

#define JAM_STATIC_ASSERT_COMPONENT_PROPERTY_VALIDATION_CHECK(TCmp, _flags, _checkValidationConcept) \
    static_assert(!HasComponentPropertyV<TCmp, _flags> || _checkValidationConcept<TCmp>, "Component '" #TCmp "' has invalid component traits for the specified flags: " #_flags)   // NOLINT(bugprone-macro-parentheses)

#define JAM_STATIC_ASSERT_COMPONENT_PROPERTY_VALIDATION_CHECKS(TCmp)                                                                                  \
    JAM_STATIC_ASSERT_COMPONENT_PROPERTY_VALIDATION_CHECK(TCmp, eComponentPropertyFlags::Serializable, CheckComponentSerializablePropertyValidation); \
    JAM_STATIC_ASSERT_COMPONENT_PROPERTY_VALIDATION_CHECK(TCmp, eComponentPropertyFlags::Editable, CheckComponentEditablePropertyValidation)

}   // namespace Jam

// ... is properties and data data

