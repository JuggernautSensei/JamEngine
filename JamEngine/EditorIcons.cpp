#include "pch.h"

#include "EditorIcons.h"

namespace Jam
{

const Ref<Texture>& EditorIcons::GetIcon(
    const eEditorIcon _icon) const
{
    return pIcons[UnderlyingFrom(_icon)];
}

void EditorIcons::SetIcon(
    const eEditorIcon        _icon,
    const Ref<Texture>& _pTexure)
{
    pIcons[UnderlyingFrom(_icon)] = _pTexure;
}

}   // namespace Jam
