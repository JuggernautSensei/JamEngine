#include "pch.h"

#include "EditorIcons.h"

namespace jam
{

const Ref<ImageTexture>& EditorIcons::GetIcon(
    const eEditorIcon _icon) const
{
    return pIcons[IntFrom(_icon)];
}

void EditorIcons::SetIcon(
    const eEditorIcon        _icon,
    const Ref<ImageTexture>& _pTexure)
{
    pIcons[IntFrom(_icon)] = _pTexure;
}

}   // namespace jam
