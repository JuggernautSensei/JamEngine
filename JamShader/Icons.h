#pragma once

enum class eIcon
{
    OpenedDirectory,
    ClosedDirectory,
    RegularFile,

    Clone,
    Remove,
    ReorderGrab,
};

extern EnumArray<eIcon, StringView> g_embededIconSVGResources;