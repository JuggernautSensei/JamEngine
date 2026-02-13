#pragma once

namespace Jam
{

// Component reflection marker

struct ComponentReflectionMarker
{
};

#define JAM_COMPONENT_REFLECTION_MARKER_VARIABLE_NAME __JAM_COMPONENT_REFLECTION_MARKER__

template<typename TComp>
concept IsComponentReflected = requires {
    { TComp::JAM_COMPONENT_REFLECTION_MARKER_VARIABLE_NAME } -> std::same_as<ComponentReflectionMarker>;
};

// Component display name in editor

struct ComponentDisplayNameInEditor
{
    constexpr explicit ComponentDisplayNameInEditor(
        const StringView _name)
        : name(_name)
    {
    }

    StringView name = {};
};

#define JAM_COMPONENT_DISPLAY_NAME_IN_EDITOR_VARIABLE_NAME __JAM_COMPONENT_DISPLAY_NAME_IN_EDITOR__

#define JAM_COMPONENT_DISPLAY_NAME_IN_EDITOR(_name) \
    static constexpr ComponentDisplayNameInEditor JAM_COMPONENT_DISPLAY_NAME_IN_EDITOR_VARIABLE_NAME { _name }

template<typename TComp>
concept HasComponentDisplayNameInEditor = requires {
    { TComp::JAM_COMPONENT_DISPLAY_NAME_IN_EDITOR_VARIABLE_NAME } -> std::same_as<ComponentDisplayNameInEditor>;
};

#define JAM_GET_COMPONENT_DISPLAY_NAME_IN_EDITOR(_cmpType) \
    _cmpType::JAM_COMPONENT_DISPLAY_NAME_IN_EDITOR_VARIABLE_NAME.name

}   // namespace Jam