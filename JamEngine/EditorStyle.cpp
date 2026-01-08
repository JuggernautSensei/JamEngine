#include "pch.h"

#include "EditorStyle.h"

namespace jam
{

EditorStyle::EditorStyle()
{
    // var
    {
        SetVar(eEditorStyleVar::AcroballAxisHeadRadius, 10.f);
        SetVar(eEditorStyleVar::AcroballAxisHeadOutlineThickness, 2.f);
        SetVar(eEditorStyleVar::AcroballAxisLineThickness, 3.5f);
        SetVar(eEditorStyleVar::AcroballAxisLineLengthRatio, 0.7f);
    }

    // color
    {
        SetColor(eEditorStyleColor::Error, ImVec4FromHex(0xFF5656FF));
        SetColor(eEditorStyleColor::Correct, ImVec4FromHex(0x50C878FF));
        SetColor(eEditorStyleColor::Info, ImVec4FromHex(0x4DA8DAFF));
        SetColor(eEditorStyleColor::AcroballAxisX, ImVec4FromHex(0xF63652FF));
        SetColor(eEditorStyleColor::AcroballAxisY, ImVec4FromHex(0x70A41CFF));
        SetColor(eEditorStyleColor::AcroballAxisZ, ImVec4FromHex(0x2F84E3FF));
        SetColor(eEditorStyleColor::AcroballAxisXOutline, ImVec4FromHex(0xF63652FF) * 1.25f);
        SetColor(eEditorStyleColor::AcroballAxisYOutline, ImVec4FromHex(0x70A41CFF) * 1.25f);
        SetColor(eEditorStyleColor::AcroballAxisZOutline, ImVec4FromHex(0x2F84E3FF) * 1.25f);
        SetColor(eEditorStyleColor::RunButtonBg, ImVec4FromHex(0x388e3cFF));
        SetColor(eEditorStyleColor::RunButtonBgHovered, ImVec4FromHex(0x388e3cFF) * 1.15f);
        SetColor(eEditorStyleColor::RunButtonBgActive, ImVec4FromHex(0x388e3cFF) * 1.25f);
        SetColor(eEditorStyleColor::EditButtonBg, ImVec4FromHex(0x2772dbFF));
        SetColor(eEditorStyleColor::EditButtonBgHovered, ImVec4FromHex(0x2772dbFF) * 1.15f);
        SetColor(eEditorStyleColor::EditButtonBgActive, ImVec4FromHex(0x2772dbFF) * 1.25f);
        SetColor(eEditorStyleColor::AcroballBg, { 1.f, 1.f, 1.f, 0.f });
        SetColor(eEditorStyleColor::AcroballBgHovered, { 1.f, 1.f, 1.f, 0.2f });
        SetColor(eEditorStyleColor::AcroballBgActive, { 1.f, 1.f, 1.f, 0.3f });
    }

    // clamp alpha
    for (ImVec4& col: colors)
    {
        col.w = Clamp(col.w, 0.f, 1.f);
    }
}

float EditorStyle::GetVar(const eEditorStyleVar _var) const
{
    return vars[IntFrom(_var)];
}

ImVec4 EditorStyle::GetColor(const eEditorStyleColor _color) const
{
    return colors[IntFrom(_color)];
}

void EditorStyle::SetVar(const eEditorStyleVar _var, const float _value)
{
    vars[IntFrom(_var)] = _value;
}

void EditorStyle::SetColor(const eEditorStyleColor _color, const ImVec4& _value)
{
    colors[IntFrom(_color)] = _value;
}

}   // namespace jam