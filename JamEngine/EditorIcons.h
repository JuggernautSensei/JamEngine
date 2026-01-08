#pragma once

namespace jam
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

    Min = Cursor,
    Max = Translation,
};
JAM_ENUM_STATIC_ASSERT(eEditorIcon);

struct EditorIcons
{
    [[nodiscard]] const Ref<ImageTexture>& GetIcon(eEditorIcon _icon) const;
    void                                   SetIcon(eEditorIcon _icon, const Ref<ImageTexture>& _pTexure);

    Ref<ImageTexture> pIcons[NumEnums<eEditorIcon>()] = {};
};

}   // namespace jam