#include "pch.h"

#include "EditorWidgets.h"

#include "AssetManager.h"
#include "Camera.h"
#include "Editor.h"
#include "EditorUtilities.h"
#include "Entity.h"
#include "Graphics.h"
#include "ImGuiUtilities.h"
#include "ScriptDB.h"
#include "Transform.h"
#include "VertexLayoutEntry.h"

namespace jam
{

enum class eAcroballAxis
{
    X,
    Y,
    Z
};

struct AcroballAxisData
{
    static bool CompareDepthAscending(
        const AcroballAxisData& a,
        const AcroballAxisData& b)
    {
        return a.depth < b.depth;
    }

    const char* name;
    ImVec4      color;
    ImVec4      outlineColor;

    Vec3          dir;
    eAcroballAxis axis;
    float         depth;
    bool          bPos;   // positive direction
};

static StringView TmpImageTextureInfoString(
    const Ref<ImageTexture>& _pTexture)
{
    constexpr const char* const k_names[] = { "texture 2D", "texture 3D", "texture cube" };

    const TextureInfo        texInfo    = Graphics::GetTextureInfo(_pTexture->GetHandle());
    const TextureFormatInfo& formatInfo = TextureFormatInfoFrom(texInfo.format);

    return GlobalBuffer::FormatTmp("path: {}\n"
                                   "size: {} x {} x {}\n"
                                   "type: {}\n"
                                   "format: {}\n"
                                   "array size: {}\n"
                                   "samples: {}\n"
                                   "mip levels: {}",
                                   _pTexture->GetPath().native(),
                                   texInfo.width,
                                   texInfo.height,
                                   texInfo.depth,
                                   k_names[IntFrom(texInfo.type)],
                                   formatInfo.name,
                                   texInfo.arraySize,
                                   texInfo.samples,
                                   texInfo.numMips);
}

}   // namespace jam

namespace jam
{

bool Widget_DragTranslation(
    const char*            _label,
    Vec3&                  _translation,
    const float            _speed,
    const float            _min,
    const float            _max,
    const char*            _fmt,
    const ImGuiSliderFlags _flag)
{
    const ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return false;
    }

    ImGui::PushID(_label);
    ImGui::BeginGroup();

    const ImGuiStyle& style     = ImGui::GetStyle();
    const float       textW     = ImGui::CalcTextSize("X").x;
    const float       btnW      = textW + 2.f * style.FramePadding.x;
    const float       itemW     = ImGui::CalcItemWidth();
    const float       spacing   = style.ItemInnerSpacing.x;
    const float       dragItemW = (itemW - (btnW * 3.f) - (spacing * 5.f)) / 3.f;
    bool              bChanged  = false;

    if (ImGui::Button("X"))
    {
        _translation.x = 0.f;
        bChanged       = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragItemW);
    bChanged |= Widget_DragFloat("##X", &_translation.x, _speed, _min, _max, _fmt, _flag);

    ImGui::SameLine(0, spacing);
    if (ImGui::Button("Y"))
    {
        _translation.y = 0.f;
        bChanged       = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragItemW);
    bChanged |= Widget_DragFloat("##Y", &_translation.y, _speed, _min, _max, _fmt, _flag);

    ImGui::SameLine(0, spacing);
    if (ImGui::Button("Z"))
    {
        _translation.z = 0.f;
        bChanged       = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragItemW);
    bChanged |= Widget_DragFloat("##Z", &_translation.z, _speed, _min, _max, _fmt, _flag);

    ImGui::EndGroup();
    ImGui::PopID();
    return bChanged;
}

bool Widget_DragScale(
    const char*            _label,
    Vec3&                  _scaleNorm,
    const float            _speedPerc,
    const float            _minPerc,
    const float            _maxPerc,
    const char*            _fmt,
    const ImGuiSliderFlags _flag)
{
    const ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return false;
    }

    ImGui::PushID(_label);
    ImGui::BeginGroup();

    Vec3              scalePerc = _scaleNorm * 100.f;
    const ImGuiStyle& style     = ImGui::GetStyle();
    const float       textW     = ImGui::CalcTextSize("X").x;
    const float       btnW      = textW + 2.f * style.FramePadding.x;
    const float       itemW     = ImGui::CalcItemWidth();
    const float       spacing   = style.ItemInnerSpacing.x;
    const float       dragItemW = (itemW - (btnW * 3.f) - (spacing * 5.f)) / 3.f;
    bool              bChanged  = false;

    if (ImGui::Button("X"))
    {
        scalePerc.x = 100.f;
        bChanged    = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragItemW);
    bChanged |= Widget_DragFloat("##x", &scalePerc.x, _speedPerc, _minPerc, _maxPerc, _fmt, _flag);

    ImGui::SameLine(0, spacing);
    if (ImGui::Button("Y"))
    {
        scalePerc.y = 100.f;
        bChanged    = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragItemW);
    bChanged |= Widget_DragFloat("##y", &scalePerc.y, _speedPerc, _minPerc, _maxPerc, _fmt, _flag);

    ImGui::SameLine(0, spacing);
    if (ImGui::Button("Z"))
    {
        scalePerc.z = 100.f;
        bChanged    = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragItemW);
    bChanged |= Widget_DragFloat("##z", &scalePerc.z, _speedPerc, _minPerc, _maxPerc, _fmt, _flag);

    ImGui::EndGroup();
    ImGui::PopID();

    if (bChanged)
    {
        _scaleNorm = scalePerc * 0.01f;
        return true;
    }
    else
    {
        return false;
    }
}

bool Widget_DragPitchYawRoll(
    const char*            _label,
    Vec3&                  _pitchYawRollRad,
    const float            _speedDeg,
    const float            _minDeg,
    const float            _maxDeg,
    const char*            _fmt,
    const ImGuiSliderFlags _flag)
{
    const ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return false;
    }

    ImGui::PushID(_label);
    ImGui::BeginGroup();

    Vec3              pyrDeg        = ToDeg(_pitchYawRollRad);
    const ImGuiStyle& style         = ImGui::GetStyle();
    const float       textW         = ImGui::CalcTextSize("X").x;
    const float       btnW          = textW + 2.f * style.FramePadding.x;
    const float       itemW         = ImGui::CalcItemWidth();
    const float       spacing       = style.ItemInnerSpacing.x;
    const float       dragItemWidth = (itemW - (btnW * 3.f) - (spacing * 5.f)) / 3.f;
    bool              bChanged      = false;

    if (ImGui::Button("X"))
    {
        pyrDeg.x = 0.f;
        bChanged = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragItemWidth);
    bChanged |= Widget_DragFloat("##pitch", &pyrDeg.x, _speedDeg, _minDeg, _maxDeg, _fmt, _flag);

    ImGui::SameLine(0, spacing);
    if (ImGui::Button("Y"))
    {
        pyrDeg.y = 0.f;
        bChanged = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragItemWidth);
    bChanged |= Widget_DragFloat("##yaw", &pyrDeg.y, _speedDeg, _minDeg, _maxDeg, _fmt, _flag);

    ImGui::SameLine(0, spacing);
    if (ImGui::Button("Z"))
    {
        pyrDeg.z = 0.f;
        bChanged = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragItemWidth);
    bChanged |= Widget_DragFloat("##roll", &pyrDeg.z, _speedDeg, _minDeg, _maxDeg, _fmt, _flag);

    ImGui::EndGroup();
    ImGui::PopID();

    if (bChanged)
    {
        _pitchYawRollRad = ToRad(pyrDeg);
        return true;
    }
    else
    {
        return false;
    }
}

bool Widget_DragPitchYaw(
    const char*            _label,
    Vec2&                  _pitchYawRad,
    const float            _speedDeg,
    const float            _minDeg,
    const float            _maxDeg,
    const char*            _fmt,
    const ImGuiSliderFlags _flag)
{
    const ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return false;
    }

    ImGui::PushID(_label);
    ImGui::BeginGroup();

    Vec2              pyDeg     = ToDeg(_pitchYawRad);
    const ImGuiStyle& style     = ImGui::GetStyle();
    const float       textW     = ImGui::CalcTextSize("X").x;
    const float       btnW      = textW + 2.f * style.FramePadding.x;
    const float       itemW     = ImGui::CalcItemWidth();
    const float       spacing   = style.ItemInnerSpacing.x;
    const float       dragItemW = (itemW - (btnW * 2.f) - (spacing * 4.f)) / 2.f;
    bool              bChanged  = false;

    if (ImGui::Button("X"))
    {
        pyDeg.x  = 0.f;
        bChanged = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragItemW);
    bChanged |= Widget_DragFloat("##pitch", &pyDeg.x, _speedDeg, _minDeg, _maxDeg, _fmt, _flag);

    ImGui::SameLine(0, spacing);
    if (ImGui::Button("Y"))
    {
        pyDeg.y  = 0.f;
        bChanged = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragItemW);
    bChanged |= Widget_DragFloat("##yaw", &pyDeg.y, _speedDeg, _minDeg, _maxDeg, _fmt, _flag);

    ImGui::EndGroup();
    ImGui::PopID();

    if (bChanged)
    {
        _pitchYawRad = ToRad(pyDeg);
        return true;
    }
    else
    {
        return false;
    }
}

bool Widget_DragAngle(
    const char*            _label,
    float&                 _pRad,
    const float            _speedDeg,
    const float            _minDeg,
    const float            _maxDeg,
    const char*            _fmt,
    const ImGuiSliderFlags _flag)
{
    float deg = ToDeg(_pRad);
    if (Widget_DragFloat(_label, &deg, _speedDeg, _minDeg, _maxDeg, _fmt, _flag))
    {
        _pRad = ToRad(deg);
        return true;
    }
    else
    {
        return false;
    }
}

bool Widget_DragPercent(
    const char*            _label,
    float&                 _pNorm,
    const float            _speedPerc,
    const float            _minPerc,
    const float            _maxPerc,
    const char*            _format,
    const ImGuiSliderFlags _flag)
{
    float perc = _pNorm * 100.f;
    if (Widget_DragFloat(_label, &perc, _speedPerc, _minPerc, _maxPerc, _format, _flag))
    {
        _pNorm = perc * 0.01f;
        return true;
    }
    else
    {
        return false;
    }
}

void Widget_ImageFrame(
    const ImTextureProxy _tex,
    const ImVec2         _texSize,
    const ImVec2         _frameSize,
    const ImVec2         _uv0,
    const ImVec2         _uv1,
    const ImVec4         _bgCol,
    const bool           _bBorder)
{
    const ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return;
    }

    const ImGuiStyle& style   = ImGui::GetStyle();
    const ImVec2      pos     = ImGui::GetCursorScreenPos();
    const float       ar      = Div(_texSize.x, _texSize.y);
    const ImVec2      imgSize = (ar > 1.f) ? ImVec2 { _frameSize.x, _frameSize.x / ar } : ImVec2 { _frameSize.y * ar, _frameSize.y };
    const ImVec2      offset  = (_frameSize - imgSize) * 0.5f;
    const ImVec2      imgP0   = pos + offset;
    const ImVec2      imgP1   = imgP0 + imgSize;

    const ImRect bb { pos, pos + _frameSize };
    ImGui::ItemSize(_frameSize);
    if (!ImGui::ItemAdd(bb, 0))
    {
        return;
    }

    ImGui::RenderFrame(bb.Min, bb.Max, ImGui::ColorConvertFloat4ToU32(_bgCol), _bBorder, style.FrameRounding);
    ImGui::GetWindowDrawList()->AddImage(_tex.texture, imgP0, imgP1, _uv0, _uv1);
}

void Widget_Image(
    const ImTextureProxy _tex,
    const ImVec2         _size,
    const ImVec2         _uv0,
    const ImVec2         _uv1)
{
    ImGui::Image(_tex.texture, _size, _uv0, _uv1);
}

void Widget_ImageFrame(
    const Ref<ImageTexture>& _pTexture,
    const ImVec2             _frameSize,
    const ImVec2             _uv0,
    const ImVec2             _uv1,
    const ImVec4             _bgCol,
    const bool               _bBorder)
{
    JAM_ASSERT(_pTexture, "Texture is null");
    const Vec2i  texRes  = _pTexture->GetResolution();
    const ImVec2 texSize = { static_cast<float>(texRes.x), static_cast<float>(texRes.y) };
    Widget_ImageFrame(ImGuiGlue::ImTextureRefFrom(_pTexture), texSize, _frameSize, _uv0, _uv1, _bgCol, _bBorder);
}

void Widget_CenteredText(
    const char*  _text,
    const ImVec2 _region)
{
    const ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return;
    }

    // det region
    const ImVec2 avail  = ImGui::GetContentRegionAvail();
    ImVec2       region = _region;
    if (IsZeroApprox(region.x))
    {
        region.x = avail.x;
    }

    if (IsZeroApprox(region.y))
    {
        region.y = avail.y;
    }

    // render
    const ImVec2 textSize = ImGui::CalcTextSize(_text, NULL, true);
    const ImVec2 offset   = (region - textSize) * 0.5f;
    ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + offset);
    ImGui::TextUnformatted(_text);
}

void Widget_FrameAlignedText(
    const char* _text)
{
    const ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return;
    }

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(_text);
}

void Widget_BorderedText(
    const char*  _text,
    const ImVec2 _size,
    const bool   _bDisabled)
{
    const ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return;
    }

    const ImGuiStyle& style    = ImGui::GetStyle();
    const ImVec2      textSize = ImGui::CalcTextSize(_text, NULL, true);
    const ImVec2      pos      = pWin->DC.CursorPos;
    const ImVec2      size     = ImGui::CalcItemSize(_size, textSize.x + style.FramePadding.x * 2.0f, textSize.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, 0))
    {
        return;
    }

    ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

    if (_bDisabled)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_TextDisabled));
    }

    ImGui::RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, _text, NULL, &textSize, style.ButtonTextAlign, &bb);

    if (_bDisabled)
    {
        ImGui::PopStyleColor();
    }
}

bool Widget_Button(
    const ButtonDesc& _desc)
{
    ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return false;
    }

    // shortcut
    if (_desc.shortcut != 0)
    {
        ImGui::SetNextItemShortcut(_desc.shortcut);
    }

    // button
    bool bPressed = false;
    if (_desc.direction != ImGuiDir_None)
    {
        bPressed = ImGui::ArrowButton(_desc.text, _desc.direction);
    }
    else
    {
        const ImGuiStyle& style         = ImGui::GetStyle();
        const bool        bHasIcon      = _desc.icon.IsValid();
        const ImGuiID     id            = pWin->GetID(_desc.text);
        const ImVec2      textSize      = _desc.bRenderText ? ImGui::CalcTextSize(_desc.text, NULL, true) : ImVec2 { 0.f, 0.f };
        const ImVec2      iconSize      = bHasIcon ? ImVec2 { ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight() } : ImVec2 { 0.f, 0.f };
        const float       innerSpacingX = (bHasIcon && _desc.bRenderText) ? style.ItemInnerSpacing.x : 0.f;
        const float       defaultW      = iconSize.x + textSize.x + innerSpacingX + style.FramePadding.x * 2.0f;
        const float       defaultH      = ImGui::GetTextLineHeight() + style.FramePadding.y * 2.0f;
        const ImVec2      size          = ImGui::CalcItemSize(_desc.size, defaultW, defaultH);

        const ImVec2 cursor = pWin->DC.CursorPos;
        const ImRect bb     = { cursor, cursor + size };
        ImGui::ItemSize(size, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, id))
        {
            return false;
        }

        bool hovered, held;
        bPressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

        // render frame
        const ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive
                                             : hovered         ? ImGuiCol_ButtonHovered
                                                               : ImGuiCol_Button);
        ImGui::RenderNavCursor(bb, id);
        ImGui::RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);

        // render image
        const ImVec2 iconP0 = cursor + style.FramePadding;
        const ImVec2 iconP1 = iconP0 + iconSize;
        if (bHasIcon)
        {
            ImDrawList* pDrawList = ImGui::GetWindowDrawList();
            pDrawList->AddImage(_desc.icon.texture, iconP0, iconP1);
        }

        // render text
        if (_desc.bRenderText)
        {
            const ImVec2 textP0 = { iconP1.x + innerSpacingX, iconP0.y };
            const ImVec2 textP1 = { bb.Max.x - style.FramePadding.x, bb.Max.y - style.FramePadding.y };
            ImGui::RenderTextClipped(textP0, textP1, _desc.text, NULL, &textSize, style.ButtonTextAlign, &bb);
        }
    }

    if (_desc.bRenderDescriptionTooltip)
    {
        if (ImGui::BeginItemTooltip())
        {
            // label
            ImGui::TextUnformatted(_desc.text);

            // shortcut
            if (_desc.shortcut != 0)
            {
                const ImGuiKey   key      = static_cast<ImGuiKey>(_desc.shortcut & ~(ImGuiMod_Mask_));
                const StringView svText   = GlobalBuffer::FormatTmp("(short cut: {}{}{}{})",
                                                                  (_desc.shortcut & ImGuiMod_Ctrl) ? "ctrl + " : "",
                                                                  (_desc.shortcut & ImGuiMod_Shift) ? "shift + " : "",
                                                                  (_desc.shortcut & ImGuiMod_Alt) ? "alt + " : "",
                                                                  EditorDisplayStringFrom(key));
                const char*      text     = svText.GetPtr();
                const ImVec2     textSize = ImGui::CalcTextSize(text);
                const ImVec2     avail    = ImGui::GetCursorScreenPos();
                ImGui::SetCursorScreenPos({ avail.x - textSize.x, avail.y });
                ImGui::TextDisabled(text);   // right aligned
            }

            // description
            if (_desc.tooltipDescription)
            {
                ImGui::Separator();
                ImGui::TextUnformatted(_desc.tooltipDescription);
            }

            ImGui::EndTooltip();
        }
    }

    return bPressed;
}

bool Widget_DragScalar(
    const char*            _label,
    const ImGuiDataType    _dataType,
    void*                  _pData,
    const float            _vSpeed,
    const void*            _pMin,
    const void*            _pMax,
    const char*            _format,
    const ImGuiSliderFlags _flags)
{
    JAM_ASSERT(ImGui::DataTypeCompare(_dataType, _pData, _pMin) <= 0, "min value is greater than data");

    static InfiniteDragHelper s_dragHelper;

    const bool    bChanged          = ImGui::DragScalar(_label, _dataType, _pData, _vSpeed, _pMin, _pMax, _format, _flags);
    const ImGuiID id                = ImGui::GetItemID();
    const bool    bIsActivated      = ImGui::IsItemActivated();
    const bool    bIsActive         = ImGui::IsItemActive();
    const bool    bIsDeactived      = ImGui::IsItemDeactivated();
    const bool    bIsTmpInputActive = ImGui::TempInputIsActive(id);

    if (bIsActivated)
    {
        s_dragHelper.Begin();
    }

    if (bIsActive)
    {
        s_dragHelper.Update();
    }

    if (bIsDeactived || bIsTmpInputActive)
    {
        s_dragHelper.End();
    }

    return bChanged;
}

bool Widget_DragScalarN(
    const char*            _label,
    const ImGuiDataType    _dataType,
    void*                  _pData,
    const int              _numComps,
    const float            _speed,
    const void*            _pMin,
    const void*            _pMax,
    const char*            _format,
    const ImGuiSliderFlags _flags)
{
    const ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return false;
    }

    const ImGuiDataTypeInfo* pInfo   = ImGui::DataTypeGetInfo(_dataType);
    const float              spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    bool                     bDirty  = false;

    ImGui::BeginGroup();

    ImGui::PushID(_label);
    ImGui::PushMultiItemsWidths(_numComps, ImGui::CalcItemWidth());
    for (Int32 i = 0; i < _numComps; i++)
    {
        ImGui::PushID(i);
        if (i > 0)
        {
            ImGui::SameLine(0, spacing);
        }

        bDirty |= Widget_DragScalar("", _dataType, _pData, _speed, _pMin, _pMax, _format, _flags);
        _pData = static_cast<Byte*>(_pData) + pInfo->Size;
        ImGui::PopID();
        ImGui::PopItemWidth();
    }
    ImGui::PopID();

    const char* label_end = ImGui::FindRenderedTextEnd(_label);
    if (_label != label_end)
    {
        ImGui::SameLine(0, spacing);
        ImGui::TextEx(_label, label_end);
    }

    ImGui::EndGroup();
    return bDirty;
}

bool Widget_DragFloat(
    const char*            label,
    float*                 p_data,
    const float            v_speed,
    const float            v_min,
    const float            v_max,
    const char*            format,
    const ImGuiSliderFlags flags)
{
    return Widget_DragScalar(
        label,
        ImGuiDataType_Float,
        p_data,
        v_speed,
        &v_min,
        &v_max,
        format,
        flags);
}

bool Widget_DragFloat2(
    const char*            label,
    float                  p_data[2],
    const float            v_speed,
    const float            v_min,
    const float            v_max,
    const char*            format,
    const ImGuiSliderFlags flags)
{
    return Widget_DragScalarN(
        label,
        ImGuiDataType_Float,
        p_data,
        2,
        v_speed,
        &v_min,
        &v_max,
        format,
        flags);
}

bool Widget_DragFloat3(
    const char*            label,
    float                  p_data[3],
    const float            v_speed,
    const float            v_min,
    const float            v_max,
    const char*            format,
    const ImGuiSliderFlags flags)
{
    return Widget_DragScalarN(
        label,
        ImGuiDataType_Float,
        p_data,
        3,
        v_speed,
        &v_min,
        &v_max,
        format,
        flags);
}

bool Widget_DragFloat4(
    const char*            label,
    float                  p_data[4],
    const float            v_speed,
    const float            v_min,
    const float            v_max,
    const char*            format,
    const ImGuiSliderFlags flags)
{
    return Widget_DragScalarN(
        label,
        ImGuiDataType_Float,
        p_data,
        4,
        v_speed,
        &v_min,
        &v_max,
        format,
        flags);
}

bool Widget_DragInt(
    const char*            label,
    int*                   p_data,
    const float            v_speed,
    const int              v_min,
    const int              v_max,
    const char*            format,
    const ImGuiSliderFlags flags)
{
    return Widget_DragScalar(
        label,
        ImGuiDataType_S32,
        p_data,
        v_speed,
        &v_min,
        &v_max,
        format,
        flags);
}

bool Widget_DragInt2(
    const char*            label,
    int                    p_data[2],
    const float            v_speed,
    const int              v_min,
    const int              v_max,
    const char*            format,
    const ImGuiSliderFlags flags)
{
    return Widget_DragScalarN(
        label,
        ImGuiDataType_S32,
        p_data,
        2,
        v_speed,
        &v_min,
        &v_max,
        format,
        flags);
}

void Widget_HelpMark(
    const char* _desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(_desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

bool Widget_MenuItem(
    const char*         _label,
    const ImGuiKeyChord _shortcut,
    bool*               _pSelected,
    const bool          _bEnabled)
{
    const char* shortcutText = nullptr;
    if (_shortcut != 0)
    {
        ImGui::SetNextItemShortcut(_shortcut);

        const ImGuiKey   key  = static_cast<ImGuiKey>(_shortcut & ~(ImGuiMod_Mask_));
        const StringView text = GlobalBuffer::FormatTmp("{}{}{}{}",
                                                        (_shortcut & ImGuiMod_Ctrl) ? "ctrl + " : "",
                                                        (_shortcut & ImGuiMod_Shift) ? "shift + " : "",
                                                        (_shortcut & ImGuiMod_Alt) ? "alt + " : "",
                                                        EditorDisplayStringFrom(key));

        shortcutText = text.GetPtr();
    }

    return ImGui::MenuItem(_label, shortcutText, _pSelected, _bEnabled);
}

bool Widget_MenuItem(
    const char*         _label,
    const ImGuiKeyChord _shortcut,
    const bool          _bSelected,
    const bool          _bEnabled)
{
    const char* shortcutText = nullptr;
    if (_shortcut != 0)
    {
        ImGui::SetNextItemShortcut(_shortcut);

        const ImGuiKey   key  = static_cast<ImGuiKey>(_shortcut & ~(ImGuiMod_Mask_));
        const StringView text = GlobalBuffer::FormatTmp("{}{}{}{}",
                                                        (_shortcut & ImGuiMod_Ctrl) ? "ctrl + " : "",
                                                        (_shortcut & ImGuiMod_Shift) ? "shift + " : "",
                                                        (_shortcut & ImGuiMod_Alt) ? "alt + " : "",
                                                        EditorDisplayStringFrom(key));

        shortcutText = text.GetPtr();
    }

    return ImGui::MenuItem(_label, shortcutText, _bSelected, _bEnabled);
}

bool Widget_VertexLayoutEntryCombo(
    const char*             _label,
    Ref<VertexLayoutEntry>& _pVle)
{
    const VertexLayoutMap& map      = Editor::GetVertexLayoutMap();
    const StringView       preview  = _pVle ? _pVle->GetName() : "none";
    bool                   bChanged = false;

    if (ImGui::BeginCombo(_label, preview.GetPtr(), ImGuiComboFlags_NoArrowButton))
    {
        for (const auto& [name, pVle]: map)
        {
            const char* label     = name.c_str();
            const bool  bSelected = (_pVle == pVle);

            if (ImGui::Selectable(label, bSelected))
            {
                _pVle    = pVle;
                bChanged = true;
            }

            if (bSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    return bChanged;
}

bool Widget_ScriptCombo(
    const char*  _label,
    Ref<Script>& _pScript,
    const Entity _owner)
{
    const ScriptDB db = ScriptDB::GetInstance();

    const bool       bValidScript = _pScript != nullptr;
    const StringView preview      = bValidScript ? _pScript->GetTypeName() : "none";
    bool             bChanged     = false;

    if (ImGui::BeginCombo(_label, preview.GetPtr(), ImGuiComboFlags_NoArrowButton))
    {
        for (const ScriptInfo& scriptInfo: db.GetContainer() | std::views::values)
        {
            const StringView name      = scriptInfo.name;
            const bool       bSelected = bValidScript && (preview == name);

            if (ImGui::Selectable(name.GetPtr(), bSelected))
            {
                _pScript = db.Create(name, _owner);
                bChanged = true;
            }

            if (bSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    return bChanged;
}

bool Widget_ProjectionCombo(
    const char*  _label,
    eProjection& _projection)
{
    constexpr const char* k_names[] = { "perspective", "orthographic" };
    return Widget_Combo(_label, reinterpret_cast<Int32&>(_projection), k_names);
}

bool Widget_TextureChannelCombo(
    const char*      _label,
    eTextureChannel& _colorChannel,
    const bool       _bExcludeAlphaChannel)
{
    if (_bExcludeAlphaChannel)
    {
        constexpr const char* k_names[] = { "R", "G", "B" };
        return Widget_Combo(_label, reinterpret_cast<Int32&>(_colorChannel), k_names);
    }
    else
    {
        constexpr const char* k_names[] = { "R", "G", "B", "A" };
        return Widget_Combo(_label, reinterpret_cast<Int32&>(_colorChannel), k_names);
    }
}

bool Widget_TextureFormatCombo(
    const char*     _label,
    eTextureFormat& _format,
    const bool      _bExcludeDepthFormat)
{
    const TextureFormatInfo& previewInfo = TextureFormatInfoFrom(_format);
    const StringView         preview     = previewInfo.name;
    bool                     bChanged    = false;

    // begin combo
    if (ImGui::BeginCombo(_label, preview.GetPtr(), ImGuiComboFlags_NoArrowButton))
    {
        for (const eTextureFormat format: eTextureFormat {})
        {
            // unkown format and depth format are excluded
            if (_bExcludeDepthFormat && IsDepthFormat(format))
            {
                continue;
            }

            // render selectable
            const bool               bSelected = (format == _format);
            const TextureFormatInfo& info      = TextureFormatInfoFrom(format);
            const StringView         label     = info.name;

            if (ImGui::Selectable(label.GetPtr(), bSelected))
            {
                _format  = format;
                bChanged = true;
            }

            // set default focus
            if (bSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    return bChanged;
}

bool Widget_MeshSelector(
    const char* _label,
    Ref<Mesh>&  _pMesh)
{
    const char* text     = _pMesh ? _pMesh->GetPath().c_str() : "no mesh";
    bool        bChanged = false;

    // push id
    ImGui::PushID(_label);

    // text
    Widget_BorderedText(text, CalcFitButtonSize());

    // action
    const bool bTooltip = ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip);
    ImGui::OpenPopupOnItemClick("Menu");

    // drag drop
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* pPayload = AcceptFileDragDropPayload())
        {
            const fs::path path = ResolvePathDragDropPayload(pPayload);
            if (MoveIfHasValue(AssetManager::LoadMesh(path), _pMesh))
            {
                bChanged = true;
            }
            else
            {
                const StringView msg = GlobalBuffer::FormatTmp("failed to load mesh from path: {}. {}", path.native(), GlobalBuffer::GetLastError());
                Editor::PushMessageDialog("error", msg);
            }
        }
        ImGui::EndDragDropTarget();
    }

    // menu
    if (ImGui::BeginPopup("Menu"))
    {
        if (Widget_MenuItem("clear"))
        {
            _pMesh   = nullptr;
            bChanged = true;
        }
        ImGui::EndPopup();
    }

    // tooltip
    if (bTooltip && ImGui::BeginTooltip())
    {
        if (_pMesh)
        {
            ImGui::TextUnformatted(text);
        }
        else
        {
            ImGui::TextUnformatted("no mesh!\ndrag & drop .jmsh file for load\nor .obj, .fbx for import.");
        }
        ImGui::EndTooltip();
    }

    ImGui::PopID();
    return bChanged;
}

bool Widget_MaterialSelector(
    const char*    _label,
    Ref<Material>& _pMaterial)
{
    bool        bChanged = false;
    const char* text     = _pMaterial ? _pMaterial->GetPath().c_str() : "no material";

    // push id
    ImGui::PushID(_label);

    // preview item
    Widget_BorderedText(text, CalcFitButtonSize());

    const bool bTooltip = ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip);
    ImGui::OpenPopupOnItemClick("Menu");

    // drag drop
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* pPayload = AcceptFileDragDropPayload())
        {
            const fs::path path = ResolvePathDragDropPayload(pPayload);
            if (MoveIfHasValue(AssetManager::LoadMaterial(path), _pMaterial))
            {
                bChanged = true;
            }
            else
            {
                const StringView msg = GlobalBuffer::FormatTmp("failed to load material from path: {}. {}", path.native(), GlobalBuffer::GetLastError());
                Editor::PushMessageDialog("error", msg);
            }
        }
        ImGui::EndDragDropTarget();
    }

    // menu
    if (ImGui::BeginPopup("Menu"))
    {
        if (Widget_MenuItem("clear"))
        {
            _pMaterial = nullptr;
            bChanged   = true;
        }
        ImGui::EndPopup();
    }

    // tooltip
    if (bTooltip && ImGui::BeginTooltip())
    {
        if (_pMaterial)
        {
            ImGui::TextUnformatted(text);
        }
        else
        {
            ImGui::TextUnformatted("no material!\ndrag & drop .jmat file here!");
        }
        ImGui::EndTooltip();
    }

    ImGui::PopID();
    return bChanged;
}

bool Widget_ImageTextureSelector(
    const char*        _label,
    Ref<ImageTexture>& _pImageTexture,
    const ImVec2&      _size)
{
    ImGui::PushID(_label);
    bool bChanged = false;

    // preview item
    if (_pImageTexture)
    {
        Widget_ImageFrame(_pImageTexture, _size, { 0.f, 0.f }, { 1.f, 1.f }, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg), true);
    }
    else
    {
        Widget_BorderedText("no image", _size, true);
    }

    // tooltip & context menu
    const bool bTooltip = ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip);
    ImGui::OpenPopupOnItemClick("Menu");

    // drag drop
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* pPayload = AcceptFileDragDropPayload())
        {
            const fs::path path = ResolvePathDragDropPayload(pPayload);
            if (MoveIfHasValue(AssetManager::LoadImageTexture(path), _pImageTexture))
            {
                bChanged = true;
            }
            else
            {
                const StringView msg = GlobalBuffer::FormatTmp("failed to load image texture from path: {}. {}", path.native(), GlobalBuffer::GetLastError());
                Editor::PushMessageDialog("error", msg);
            }
        }

        ImGui::EndDragDropTarget();
    }

    // tooltip
    if (bTooltip && ImGui::BeginTooltip())
    {
        if (_pImageTexture)
        {
            const StringView info = TmpImageTextureInfoString(_pImageTexture);
            ImGui::TextUnformatted(info.GetPtr());
        }
        else
        {
            ImGui::TextUnformatted("no image!\ndrag & drop .jtex file for load\nor image file(.png, .dds, ect) for import.");
        }
        ImGui::EndTooltip();
    }

    // item popup
    if (ImGui::BeginPopup("Menu"))
    {
        if (Widget_MenuItem("clear"))
        {
            _pImageTexture = nullptr;
            bChanged       = true;
        }

        ImGui::EndPopup();
    }

    ImGui::PopID();
    return bChanged;
}

bool Widget_Acroball(
    const char*  _label,
    Transform&   _transform,
    const ImVec2 _pos,
    const float  _length)
{
    ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return false;
    }

    const ImGuiStyle& style = ImGui::GetStyle();
    const ImGuiID     id    = pWin->GetID(_label);

    // add item
    const float  radius = _length * 0.5f;
    const ImVec2 size   = { _length, _length };
    const ImRect bb     = { _pos, _pos + size };
    const ImVec2 center = bb.GetCenter();

    ImGui::SetCursorScreenPos(_pos);
    ImGui::ItemSize(bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
    {
        return false;
    }

    // interaction
    bool       bHovered, bHold;
    const bool bReleased = ImGui::ButtonBehavior(bb, id, &bHovered, &bHold);
    const bool bPressed  = bHold && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool       bChanged  = false;

    // infinite drag
    static InfiniteDragHelper s_dragHelper = {};
    static float              s_pitchRad   = 0.f;
    static float              s_yawRad     = 0.f;

    // pressed: store current rotation
    if (bPressed)
    {
        // capture current rotation
        const auto [p, y, _] = _transform.ToPitchYawRoll();
        s_pitchRad           = p;
        s_yawRad             = y;

        // freeze cursor
        s_dragHelper.Begin();
    }

    // released: unfreeze cursor
    if (bReleased)
    {
        s_dragHelper.End();
    }

    // held: update rotation
    if (bHold)
    {
        s_dragHelper.Update();
        const auto [dx, dy] = s_dragHelper.GetDragDelta();
        if (!IsZeroApprox({ dx, dy }))
        {
            constexpr float k_sensitivity = 0.005f;

            // apply rotation
            s_yawRad   = Wrap(s_yawRad + dx * k_sensitivity, -k_2pi, k_2pi);
            s_pitchRad = Clamp(s_pitchRad + dy * k_sensitivity, ToRad(-89.f), ToRad(89.f));
            _transform.SetRotation(s_pitchRad, s_yawRad, 0.f);

            // dirty check
            bChanged = true;
        }
    }

    // background circle
    const eEditorStyleColor col = bHold    ? eEditorStyleColor::AcroballBgActive
                                : bHovered ? eEditorStyleColor::AcroballBgHovered
                                           : eEditorStyleColor::AcroballBg;

    const ImU32 bgCol     = Editor::GetStyleColorU32(col);
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();
    pDrawList->AddCircleFilled(center, radius, bgCol, 64);

    // color
    const ImVec4 axisXCol = Editor::GetStyleColor(eEditorStyleColor::AcroballAxisX);
    const ImVec4 axisYCol = Editor::GetStyleColor(eEditorStyleColor::AcroballAxisY);
    const ImVec4 axisZCol = Editor::GetStyleColor(eEditorStyleColor::AcroballAxisZ);

    const ImVec4 axisXOutlineCol = Editor::GetStyleColor(eEditorStyleColor::AcroballAxisXOutline);
    const ImVec4 axisYOutlineCol = Editor::GetStyleColor(eEditorStyleColor::AcroballAxisYOutline);
    const ImVec4 axisZOutlineCol = Editor::GetStyleColor(eEditorStyleColor::AcroballAxisZOutline);

    // style value
    const float axisLineThickness    = Editor::GetStyleVar(eEditorStyleVar::AcroballAxisLineThickness);
    const float axisLineRatio        = Editor::GetStyleVar(eEditorStyleVar::AcroballAxisLineLengthRatio);
    const float axisLineLen          = radius * axisLineRatio;
    const float headRadius           = Editor::GetStyleVar(eEditorStyleVar::AcroballAxisHeadRadius);
    const float headOutlineThickness = Editor::GetStyleVar(eEditorStyleVar::AcroballAxisHeadOutlineThickness);

    // acroball data
    const Vec3 axisX = _transform.GetRight();
    const Vec3 axisY = _transform.GetUp();
    const Vec3 axisZ = _transform.GetForward();

    // axis data
    AcroballAxisData axes[6] = {
        {  "X", axisXCol, axisXOutlineCol,  axisX, eAcroballAxis::X,  axisX.z,  true },
        { "-X", axisXCol, axisXOutlineCol, -axisX, eAcroballAxis::X, -axisX.z, false },
        {  "Y", axisYCol, axisYOutlineCol,  axisY, eAcroballAxis::Y,  axisY.z,  true },
        { "-Y", axisYCol, axisYOutlineCol, -axisY, eAcroballAxis::Y, -axisY.z, false },
        {  "Z", axisZCol, axisZOutlineCol,  axisZ, eAcroballAxis::Z,  axisZ.z,  true },
        { "-Z", axisZCol, axisZOutlineCol, -axisZ, eAcroballAxis::Z, -axisZ.z, false },
    };

    // sort by depth
    std::ranges::sort(axes, AcroballAxisData::CompareDepthAscending);

    // render
    for (const AcroballAxisData& axis: axes)
    {
        const ImVec2 endPos = { center.x + axis.dir.x * axisLineLen, center.y - axis.dir.y * axisLineLen };

        // depth based color (far faces are darker)
        const float  depth01    = (axis.depth + 1.f) * 0.5f;                   // map from [-1,1] to [0,1]
        const float  t0         = Clamp(Lerp(0.5f, 1.f, depth01), 0.f, 1.f);   // bright for front faces
        const ImVec4 axisCol    = { axis.color.x * t0, axis.color.y * t0, axis.color.z * t0, axis.color.w };
        const ImU32  axisColU32 = ImGui::GetColorU32(axisCol);

        // draw line + circle at the end
        pDrawList->AddLine(center, endPos, axisColU32, axisLineThickness);
        pDrawList->AddCircleFilled(endPos, headRadius, axisColU32, 20);

        // draw outline
        const ImVec4 axisOutlineCol    = { axis.outlineColor.x * t0, axis.outlineColor.y * t0, axis.outlineColor.z * t0, axis.outlineColor.w };
        const ImU32  axisOutlineColU32 = ImGui::GetColorU32(axisOutlineCol);
        pDrawList->AddCircle(endPos, headRadius, axisOutlineColU32, 20, headOutlineThickness);

        // draw label
        if (axis.bPos)
        {
            const char*  text     = axis.name;
            const ImVec2 textSize = ImGui::CalcTextSize(text);
            const ImVec2 textPos  = endPos - textSize * 0.5f;
            pDrawList->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), text);
        }
    }

    return bChanged;
}

bool Widget_Combo(
    const char*                   _label,
    Int32&                        _curItem,
    const Span<const char* const> _items)
{
    const Int32 numItems = _items.size();
    const char* preview  = 0 <= _curItem && _curItem < numItems ? _items[_curItem] : "none";
    bool        bChanged = false;

    // render combo
    if (ImGui::BeginCombo(_label, preview, ImGuiComboFlags_NoArrowButton))
    {
        for (Int32 i = 0; i < numItems; ++i)
        {
            const bool  bSelected = (i == _curItem);
            const char* label     = _items[i];

            // render selectable
            if (ImGui::Selectable(label, bSelected))
            {
                _curItem = i;
                bChanged = true;
            }

            // set default focus
            if (bSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    return bChanged;
}

void Widget_WrappedText(
    const char* _text,
    const float _wrapWidth)
{
    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + _wrapWidth);   // wrap at 400px
    ImGui::TextUnformatted(_text);
    ImGui::PopTextWrapPos();
}

bool Widget_PathSelector(
    const char* _label,
    fs::path&   _path,
    FileDialog& _fd)
{
    ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return false;
    }

    const ImGuiStyle& style    = ImGui::GetStyle();
    const float       btnW     = ImGui::CalcTextSize("open").x + style.FramePadding.x * 2.0f;
    const float       itemW    = ImGui::CalcItemWidth();
    const float       inputW   = itemW - (btnW + style.ItemInnerSpacing.x);
    bool              bChanged = false;

    // inputW
    ImGui::SetNextItemWidth(inputW);
    bChanged |= ImGui::InputText(_label, const_cast<std::string*>(&_path.native()), ImGuiInputTextFlags_ReadOnly);

    // button
    ImGui::SameLine(0.f, style.ItemInnerSpacing.x);
    if (ImGui::Button("open", { btnW, 0.f }))
    {
        if (_fd.Open())
        {
            _path    = _fd.GetFirstSelectedPath();
            bChanged = true;
        }
    }

    return bChanged;
}

}   // namespace jam