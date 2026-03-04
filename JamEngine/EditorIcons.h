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

extern EnumArray<eEditorIcon, StringView> g_embededEditorIconSVGSources;

}   // namespace Jam