#include "pch.h"

#include "ImGuiUtilities.h"

namespace jam
{

ImVec4 ImVec4FromHex(const UInt32 _rgba)
{
    return ImVec4 {
        static_cast<float>((_rgba >> 24) & 0xFF) / 255.f,
        static_cast<float>((_rgba >> 16) & 0xFF) / 255.f,
        static_cast<float>((_rgba >> 8) & 0xFF) / 255.f,
        static_cast<float>(_rgba & 0xFF) / 255.f
    };
}

ImTextureRef ImTextureRefFrom(const TextureHandle _texh)
{
    return _texh.value;
}

ImVec2 CalcFitButtonSize(const Int32 _count, ImVec2 _region)
{
    if (_region == ImVec2 { 0.f, 0.f })
    {
        _region   = ImGui::GetContentRegionAvail();
        _region.y = 0.f;
    }

    if (_count == 1)
        return _region;

    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float totalW  = _region.x - spacing * static_cast<float>(_count - 1);
    const float buttonW = totalW / static_cast<float>(_count);
    return ImVec2 { buttonW, _region.y };
}

void SetNextItemWidthToAvailableWidth()
{
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
}

}