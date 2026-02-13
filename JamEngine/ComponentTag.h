#pragma once

namespace Jam
{

class Scene;

enum class eComponentTag : UInt32
{
    None                 = 0,
    NonRemoveable        = 1 << 0,
    CannotCreateInEditor = 1 << 1,
};

#define JAM_COMPONENT_TAG_VARIABLE __JAM_COMPONENT_TAG__

#define JAM_COMPONENT_TAG(...) \
    inline static constexpr Flags<eComponentTag> JAM_COMPONENT_TAG_VARIABLE { __VA_ARGS__ };

template<typename TCmp, eComponentTag k_tag>
concept IsComponentTagEnabled = TCmp::JAM_COMPONENT_TAG_VARIABLE & Flags<eComponentTag>(k_tag);
}   // namespace Jam
