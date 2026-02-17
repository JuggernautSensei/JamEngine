#pragma once
#include "EnumArray.h"

namespace Jam
{

class Texture;

enum class eEditorIcon
{
    Cursor,
    Stop,
    Menu,
    Play,
    Rotation,
    Scale,
    Translation,
};

using EditorIcons = EnumArray<eEditorIcon, Ref<Texture>>;

}   // namespace Jam