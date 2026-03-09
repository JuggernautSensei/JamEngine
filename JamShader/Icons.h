#pragma once

enum class eIcon
{
    OpenedDirectory,
    ClosedDirectory,
    RegularFile,

    CloneShaderItemMacro,
    RemoveShaderItemMacro,
    ReorderShaderItemMaco,
};

extern EnumArray<eIcon, StringView> g_embededIconSVGResources;