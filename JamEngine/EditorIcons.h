#pragma once

namespace Jam
{

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

struct EditorIcons
{
    [[nodiscard]] const Ref<Texture>& GetIcon(eEditorIcon _icon) const;
    void                              SetIcon(eEditorIcon _icon, const Ref<Texture>& _pTexure);

    Ref<Texture> pIcons[CountOf<eEditorIcon>()] = {};
};

}   // namespace Jam