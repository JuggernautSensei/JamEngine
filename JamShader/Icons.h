#pragma once

enum class eIcon
{
    OpenedDirectory,
    ClosedDirectory,
    RegularFile,
};

extern EnumArray<eIcon, StringView> g_embededIconSVGResources;