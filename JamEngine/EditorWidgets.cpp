#include "pch.h"

#include "EditorWidgets.h"

#include "Editor.h"
#include "EditorUtilities.h"
#include "Entity.h"
#include "Graphics.h"
#include "ReflectionDB.h"
#include "Transform.h"
#include "VertexLayoutEntry.h"

namespace Jam
{

namespace
{
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

        Vec3  dir;
        float depth;
        bool  bPos;   // positive direction
    };

    struct WidgetInputTextCallback_UserData
    {
        String*                pStr           = nullptr;
        ImGuiInputTextCallback ChainCallback  = nullptr;
        void*                  pChainUserData = nullptr;
    };

    Int32 Widget_InputTextCallback(
        ImGuiInputTextCallbackData* _pData)
    {
        const WidgetInputTextCallback_UserData* pUserData = static_cast<WidgetInputTextCallback_UserData*>(_pData->UserData);
        if (_pData->EventFlag == ImGuiInputTextFlags_CallbackResize)
        {
            // Resize string callback
            // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
            String* pStr = pUserData->pStr;
            JAM_ASSERT(_pData->Buf == pStr->GetPtr(), "Invalid string buffer pointer in InputText callback.");
            pStr->Resize(_pData->BufTextLen);
            _pData->Buf = static_cast<char*>(pStr->GetPtr());
        }
        else if (pUserData->ChainCallback)
        {
            // Forward to user callback, if any
            _pData->UserData = pUserData->pChainUserData;
            return pUserData->ChainCallback(_pData);
        }
        return 0;
    }

    void FormatTextureInfoText_(
        ScratchBuffer&      _buf,
        const Ref<Texture>& _pTexture)
    {
        const TextureInfo texInfo = JAM_GFX.GetTextureInfo(_pTexture->GetHandle());
        _buf.Format("path: {}\n"
                    "size: {} x {} x {}\n"
                    "type: {}\n"
                    "format: {}\n"
                    "array size: {}\n"
                    "samples: {}\n"
                    "mip levels: {}",
                    _pTexture->GetPath(),
                    texInfo.width,
                    texInfo.height,
                    texInfo.depth,
                    NameOf(texInfo.type),
                    NameOf(texInfo.format),
                    texInfo.arraySize,
                    texInfo.samples,
                    texInfo.numMips);
    }

    void FormatShortcutDisplayText_Minimal_(
        ScratchBuffer&      _buf,
        const ImGuiKeyChord _shortcut)
    {
        if (_shortcut & ImGuiMod_Ctrl)
        {
            _buf.Append("Ctrl + ");
        }

        if (_shortcut & ImGuiMod_Shift)
        {
            _buf.Append("Shift + ");
        }

        if (_shortcut & ImGuiMod_Alt)
        {
            _buf.Append("Alt + ");
        }

        const ImGuiKey key = static_cast<ImGuiKey>(_shortcut & ~ImGuiMod_Mask_);
        if (key != ImGuiKey_None)
        {
            _buf.Append(EditorDisplayNameOf(key));
        }
    }

    void FormatShortcutDisplayText_(
        ScratchBuffer&      _buf,
        const ImGuiKeyChord _shortcut)
    {
        _buf.Set("shortcut: ");
        FormatShortcutDisplayText_Minimal_(_buf, _shortcut);
    }

}   // namespace

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

    const ImGuiStyle& style     = ImGui::GetStyle();
    const float       textW     = ImGui::CalcTextSize("X").x;
    const float       btnW      = textW + 2.f * style.FramePadding.x;
    const float       itemW     = ImGui::CalcItemWidth();
    const float       spacing   = style.ItemInnerSpacing.x;
    const float       dragItemW = (itemW - (btnW * 3.f) - (spacing * 5.f)) / 3.f;
    bool              bChanged  = false;

    ImGui::PushID(_label);
    ImGui::BeginGroup();
    ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, spacing);

    if (ImGui::Button("X"))
    {
        _translation.x = 0.f;
        bChanged       = true;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(dragItemW);
    bChanged |= Widget_DragFloat("##X", _translation.x, _speed, _min, _max, _fmt, _flag);

    ImGui::SameLine();
    if (ImGui::Button("Y"))
    {
        _translation.y = 0.f;
        bChanged       = true;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(dragItemW);
    bChanged |= Widget_DragFloat("##Y", _translation.y, _speed, _min, _max, _fmt, _flag);

    ImGui::SameLine();
    if (ImGui::Button("Z"))
    {
        _translation.z = 0.f;
        bChanged       = true;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(dragItemW);
    bChanged |= Widget_DragFloat("##Z", _translation.z, _speed, _min, _max, _fmt, _flag);

    ImGui::PopStyleVar();
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

    const ImGuiStyle& style     = ImGui::GetStyle();
    const float       textW     = ImGui::CalcTextSize("X").x;
    const float       btnW      = textW + 2.f * style.FramePadding.x;
    const float       itemW     = ImGui::CalcItemWidth();
    const float       spacing   = style.ItemInnerSpacing.x;
    const float       dragItemW = (itemW - (btnW * 3.f) - (spacing * 5.f)) / 3.f;
    bool              bChanged  = false;

    ImGui::PushID(_label);
    ImGui::BeginGroup();
    ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, spacing);

    if (ImGui::Button("X"))
    {
        _scaleNorm.x = 1.f;
        bChanged     = true;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(dragItemW);
    bChanged |= Widget_DragPercent("##X", _scaleNorm.x, _speedPerc, _minPerc, _maxPerc, _fmt, _flag);

    ImGui::SameLine();
    if (ImGui::Button("Y"))
    {
        _scaleNorm.y = 1.f;
        bChanged     = true;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(dragItemW);
    bChanged |= Widget_DragPercent("##Y", _scaleNorm.y, _speedPerc, _minPerc, _maxPerc, _fmt, _flag);

    ImGui::SameLine();
    if (ImGui::Button("Z"))
    {
        _scaleNorm.z = 1.f;
        bChanged     = true;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(dragItemW);
    bChanged |= Widget_DragPercent("##Z", _scaleNorm.z, _speedPerc, _minPerc, _maxPerc, _fmt, _flag);

    ImGui::PopStyleVar();
    ImGui::EndGroup();
    ImGui::PopID();

    return bChanged;
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
    if (Widget_DragFloat(_label, perc, _speedPerc, _minPerc, _maxPerc, _format, _flag))
    {
        _pNorm = perc * 0.01f;
        return true;
    }

    return false;
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
    if (Widget_DragFloat(_label, deg, _speedDeg, _minDeg, _maxDeg, _fmt, _flag))
    {
        _pRad = ToRad(deg);
        return true;
    }

    return false;
}

bool Widget_DragEuler(
    const char*            _label,
    Vec3&                  _eulerRad,
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

    const ImGuiStyle& style         = ImGui::GetStyle();
    const float       spacing       = style.ItemInnerSpacing.x;
    const float       textW         = ImGui::CalcTextSize("X").x;
    const float       btnW          = textW + 2.f * style.FramePadding.x;
    const float       itemW         = ImGui::CalcItemWidth();
    const float       dragItemWidth = (itemW - (btnW * 3.f) - (spacing * 5.f)) / 3.f;
    bool              bChanged      = false;

    ImGui::PushID(_label);
    ImGui::BeginGroup();
    ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, spacing);

    if (ImGui::Button("Y"))
    {
        _eulerRad.x = 0.f;
        bChanged    = true;
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(dragItemWidth);
    bChanged |= Widget_DragAngle("##Yaw", _eulerRad.x, _speedDeg, _minDeg, _maxDeg, _fmt, _flag);

    ImGui::SameLine();
    if (ImGui::Button("X"))
    {
        _eulerRad.y = 0.f;
        bChanged    = true;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(dragItemWidth);
    bChanged |= Widget_DragAngle("##Pitch", _eulerRad.y, _speedDeg, _minDeg, _maxDeg, _fmt, _flag);

    ImGui::SameLine();
    if (ImGui::Button("Z"))
    {
        _eulerRad.z = 0.f;
        bChanged    = true;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(dragItemWidth);
    bChanged |= Widget_DragAngle("##Roll", _eulerRad.z, _speedDeg, _minDeg, _maxDeg, _fmt, _flag);

    ImGui::PopStyleVar();
    ImGui::EndGroup();
    ImGui::PopID();

    return bChanged;
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

void Widget_ItemDescriptionTooltip(
    const char* _summery,
    const char* _desc)
{
    if (ImGui::BeginItemTooltip())
    {
        const ImGuiContext&      g     = *GImGui;
        const ImGuiLastItemData& data  = g.LastItemData;
        const ImGuiKeyChord      chord = data.Shortcut;

        /*

        [SUMMERY] [SHORTCUT (optional)]
        -------------------------------------
        [DESCRIPTION]

        */

        // summery
        ImGui::TextUnformatted(_summery);

        // shortcut
        if (chord != ImGuiKey_None)
        {
            ScratchBuffer buf;
            FormatShortcutDisplayText_(buf, chord);
            ImGui::SameLine();
            ImGui::TextDisabled("%s", buf.GetPtr());
        }

        // description
        ImGui::Separator();
        Widget_TextWrapped(_desc, 100.f);
        ImGui::EndTooltip();
    }
}

void Widget_ImageFrame(
    const Ref<Texture>& _pTexture,
    const ImVec2        _frameSize,
    const ImVec2        _uv0,
    const ImVec2        _uv1,
    const ImVec4        _bgCol,
    const bool          _bBorder)
{
    JAM_ASSERT(_pTexture, "Texture is null");
    const Vec2i  texRes  = _pTexture->GetResolution();
    const ImVec2 texSize = { NumericCast<float>(texRes.x), NumericCast<float>(texRes.y) };
    Widget_ImageFrame(ImTextureRefFrom(_pTexture), texSize, _frameSize, _uv0, _uv1, _bgCol, _bBorder);
}

void WidgetInfiniteDragger::OnAfterWidget()
{
    ImGuiContext& g        = *GImGui;
    const bool    bActived = ImGui::IsItemActivated();

    // on begin drag
    if (bActived)
    {
        m_capturedPos = ImGui::GetMousePos();
        JAM_INPUT.SetMouseVisibility(false);

        // for debug
        if (JAM_DEBUG_ENABLED)
        {
            JAM_ASSERT(!s_bSharedBitForDebug, "Another WidgetInfiniteDragger is already capturing mouse. Only one can capture at a time.");
            s_bSharedBitForDebug = true;
        }
    }

    // dragging
    if (!bActived && ImGui::IsItemActive())
    {
        // fix mouse pos to captured pos
        JAM_INPUT.SetMousePos(NumericCast<Int32>(m_capturedPos.x), NumericCast<Int32>(m_capturedPos.y));

        // for imgui use
        ImGuiIO& io     = ImGui::GetIO();
        io.MousePosPrev = m_capturedPos;
        io.MouseDelta   = io.MousePos - io.MousePosPrev;
    }

    // on end drag
    if (ImGui::IsItemDeactivated() || ImGui::TempInputIsActive(g.LastItemData.ID))
    {
        JAM_INPUT.SetMouseVisibility(true);
    }
}

ImVec2 WidgetInfiniteDragger::GetDragDelta(const ImGuiMouseButton _button, const float _threshold) const
{
    return ImGui::GetMouseDragDelta(_button, _threshold);
}

ImTextureProxy::ImTextureProxy() = default;

ImTextureProxy::ImTextureProxy(
    const Ref<Texture>& _pTexture)
    : texture(ImTextureRefFrom(_pTexture))
{
    JAM_ASSERT(_pTexture, "Invalid texture passed to ImTextureProxy");
    JAM_ASSERT(_pTexture->IsValid(), "Invalid texture passed to ImTextureProxy");
}

ImTextureProxy::ImTextureProxy(
    const ImTextureRef& _texture)
    : texture(_texture)
{
}

ImTextureProxy::ImTextureProxy(
    const TextureHandle _texh)
    : texture(ImTextureRefFrom(_texh))
{
}

void EulerProxy::OnBeforeWidget(const Vec3& _forward)
{
    if (!m_bIsEditing)
    {
        m_eulerRad = EulerFrom(_forward);
    }
}

void EulerProxy::OnBeforeWidget(const Mtx4& _mtx)
{
    if (!m_bIsEditing)
    {
        m_eulerRad = _mtx.ToEuler();
    }
}

void EulerProxy::OnBeforeWidget(const Quat& _quat)
{
    if (!m_bIsEditing)
    {
        m_eulerRad = _quat.ToEuler();
    }
}

void EulerProxy::OnBeforeWidget(const Transform& _t)
{
    if (!m_bIsEditing)
    {
        m_eulerRad = _t.ToEuler();
    }
}

void EulerProxy::OnAfterWidget()
{
    m_bIsEditing = ImGui::IsItemActive();
}

Vec3& EulerProxy::GetEulerProxy()
{
    return m_eulerRad;
}

void Widget_Text(
    const char* _text,
    const bool  _bFrameAligned)
{
    const ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return;
    }

    if (_bFrameAligned)
    {
        ImGui::AlignTextToFramePadding();
    }

    ImGui::TextUnformatted(_text);
}

void Widget_TextFrame(
    const char*  _text,
    const ImVec2 _frameSize,
    const ImVec4 _bgCol,
    const bool   _bRenderBorder)
{
    const ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return;
    }

    const ImGuiStyle& style    = ImGui::GetStyle();
    const ImVec2      textSize = ImGui::CalcTextSize(_text, NULL, true);
    const ImVec2      pos      = pWin->DC.CursorPos;
    const ImVec2      size     = ImGui::CalcItemSize(_frameSize, textSize.x + style.FramePadding.x * 2.0f, textSize.y + style.FramePadding.y * 2.0f);

    const ImRect bb { pos, pos + size };
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, 0))
    {
        return;
    }

    // calc center aligned text pos
    ImGui::RenderFrame(bb.Min, bb.Max, ImGui::ColorConvertFloat4ToU32(_bgCol), _bRenderBorder, style.FrameRounding);
    ImGui::RenderTextClipped(bb.Min, bb.Max, _text, NULL, &textSize, ImGui::GetStyle().ButtonTextAlign, &bb);
}

bool Widget_IconButton(
    const char*          _text,
    const ImTextureProxy _icon,
    const ImVec2         _size)
{
    ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return false;
    }

    // button
    bool              bPressed = false;
    const ImGuiStyle& style    = ImGui::GetStyle();
    const ImGuiID     id       = pWin->GetID(_text);
    const ImVec2      textSize = ImGui::CalcTextSize(_text, NULL, true);
    const float       iconLen  = Max(ImGui::GetTextLineHeight(), _size.y - style.FramePadding.y * 2.0f);
    const float       defaultW = iconLen + textSize.x + style.IndentSpacing + style.FramePadding.x * 2.0f;
    const float       defaultH = ImGui::GetFrameHeight();
    const ImVec2      size     = ImGui::CalcItemSize(_size, defaultW, defaultH);

    const ImVec2 cursor = pWin->DC.CursorPos;
    const ImRect bb     = { cursor, cursor + size };
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
    {
        return false;
    }

    bool hovered = false;
    bool held    = false;
    bPressed     = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    // render frame
    const ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive
                                         : hovered         ? ImGuiCol_ButtonHovered
                                                           : ImGuiCol_Button);

    ImGui::RenderNavCursor(bb, id);
    ImGui::RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);

    // render image
    const ImVec2 imageSize = { iconLen, iconLen };
    const ImVec2 iconP0    = cursor + style.FramePadding;
    const ImVec2 iconP1    = iconP0 + imageSize;
    ImDrawList*  pDrawList = ImGui::GetWindowDrawList();
    pDrawList->AddImage(_icon.texture, iconP0, iconP1);

    // render text
    const ImVec2 textP0 = { iconP1.x + style.ItemInnerSpacing.x, iconP0.y };
    const ImVec2 textP1 = bb.Max - style.FramePadding;
    ImGui::RenderTextClipped(textP0, textP1, _text, NULL, &textSize, style.ButtonTextAlign);
    return bPressed;
}

void Widget_TextWrapped(
    const char* _text,
    const float _wrapWidth)
{
    ImGui::PushTextWrapPos(_wrapWidth);
    ImGui::TextUnformatted(_text);
    ImGui::PopTextWrapPos();
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

    static WidgetInfiniteDragger s_dragHelper = {};

    const bool bChanged = ImGui::DragScalar(_label, _dataType, _pData, _vSpeed, _pMin, _pMax, _format, _flags);
    s_dragHelper.OnAfterWidget();
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
    const ImGuiStyle&        style   = ImGui::GetStyle();
    const float              spacing = style.ItemInnerSpacing.x;
    bool                     bDirty  = false;

    ImGui::BeginGroup();
    ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, spacing);

    ImGui::PushID(_label);
    ImGui::PushMultiItemsWidths(_numComps, ImGui::CalcItemWidth());
    for (Int32 i = 0; i < _numComps; i++)
    {
        ImGui::PushID(i);
        if (i > 0)
        {
            ImGui::SameLine();
        }

        bDirty |= Widget_DragScalar("", _dataType, _pData, _speed, _pMin, _pMax, _format, _flags);
        _pData = static_cast<Byte*>(_pData) + pInfo->Size;
        ImGui::PopID();
        ImGui::PopItemWidth();
    }
    ImGui::PopID();

    ImGui::PopStyleVar();
    ImGui::EndGroup();
    return bDirty;
}

bool Widget_DragFloat(
    const char*            label,
    float&                 p_data,
    const float            v_speed,
    const float            v_min,
    const float            v_max,
    const char*            format,
    const ImGuiSliderFlags flags)
{
    return Widget_DragScalar(
        label,
        ImGuiDataType_Float,
        &p_data,
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
    int&                   p_data,
    const float            v_speed,
    const int              v_min,
    const int              v_max,
    const char*            format,
    const ImGuiSliderFlags flags)
{
    return Widget_DragScalar(
        label,
        ImGuiDataType_S32,
        &p_data,
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
    const bool          _bSelected,
    const bool          _bEnabled)
{
    ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return false;
    }

    ScratchBuffer buf;   // for shortcut text
    const char*   shortcutText = nullptr;
    if (_shortcut != 0)
    {
        ImGui::SetNextItemShortcut(_shortcut);
        FormatShortcutDisplayText_Minimal_(buf, _shortcut);
        shortcutText = buf.GetPtr();
    }

    return ImGui::MenuItem(_label, shortcutText, _bSelected, _bEnabled);
}

bool Widget_VertexLayoutEntryCombo(
    const char*             _label,
    Ref<VertexLayoutEntry>& _pVle)
{
    ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return false;
    }

    const VertexLayoutMap& map      = JAM_EDITOR.GetVertexLayoutMap();
    const char*            preview  = _pVle ? _pVle->GetName().GetPtr() : "none";
    bool                   bChanged = false;

    if (ImGui::BeginCombo(_label, preview, ImGuiComboFlags_NoArrowButton))
    {
        for (const auto& [name, pVle]: map.GetContainer())
        {
            const char* label     = name.GetPtr();
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

bool Widget_MeshPicker(
    const char* _label,
    Ref<Mesh>&  _pMesh)
{
    ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return false;
    }

    const char* text     = _pMesh ? _pMesh->GetPath().c_str() : "no mesh";
    bool        bChanged = false;

    // push id
    ImGui::PushID(_label);

    // text
    Widget_TextFrame(text, CalcFitButtonSize());

    // action
    const bool bTooltip = ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip);
    ImGui::OpenPopupOnItemClick("Menu");

    // drag drop
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* pPayload = AcceptFileDragDropPayload())
        {
            const Path path = ResolvePathDragDropPayload(pPayload);
            if (Optional<Ref<Mesh>> pMesh = JAM_EDITOR.LoadMesh(path))
            {
                _pMesh = pMesh.Take();
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

bool Widget_MaterialPicker(
    const char*    _label,
    Ref<Material>& _pMaterial)
{
    ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return false;
    }

    bool        bChanged = false;
    const char* text     = _pMaterial ? _pMaterial->GetPath().c_str() : "no material";

    // push id
    ImGui::PushID(_label);

    // preview item
    Widget_TextFrame(text, CalcFitButtonSize());

    const bool bTooltip = ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip);
    ImGui::OpenPopupOnItemClick("Menu");

    // drag drop
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* pPayload = AcceptFileDragDropPayload())
        {
            const Path path = ResolvePathDragDropPayload(pPayload);
            if (Optional<Ref<Material>> pMaterial = JAM_EDITOR.LoadMaterial(path))
            {
                _pMaterial = pMaterial.Take();
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

bool Widget_TexturePicker(
    const char*   _label,
    Ref<Texture>& _pTexture,
    const ImVec2& _size)
{
    ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return false;
    }

    ImGui::PushID(_label);
    bool bChanged = false;

    // preview item
    if (_pTexture)
    {
        Widget_ImageFrame(_pTexture, _size, { 0.f, 0.f }, { 1.f, 1.f }, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg), true);
    }
    else
    {
        Widget_TextFrame("no image", _size, {}, true);
    }

    // tooltip & context menu
    const bool bTooltip = ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip);
    ImGui::OpenPopupOnItemClick("Menu");

    // drag drop
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* pPayload = AcceptFileDragDropPayload())
        {
            const Path path = ResolvePathDragDropPayload(pPayload);
            if (Optional<Ref<Texture>> pTexture = JAM_EDITOR.LoadTexture(path))
            {
                _pTexture = pTexture.Take();
            }
        }

        ImGui::EndDragDropTarget();
    }

    // tooltip
    if (bTooltip && ImGui::BeginTooltip())
    {
        if (_pTexture)
        {
            ScratchBuffer tmp;
            FormatTextureInfoText_(tmp, _pTexture);
            ImGui::TextUnformatted(tmp.GetPtr());
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
            _pTexture = nullptr;
            bChanged  = true;
        }

        ImGui::EndPopup();
    }

    ImGui::PopID();
    return bChanged;
}

bool Widget_Acroball(
    const char*  _label,
    Transform&   _t,
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
    bool bHovered = false;
    bool bHold    = false;
    ImGui::ButtonBehavior(bb, id, &bHovered, &bHold);
    bool bChanged = false;

    // infinite drag
    static WidgetInfiniteDragger s_dragHelper = {};
    s_dragHelper.OnAfterWidget();

    // stored rotation
    static EulerProxy s_proxy = {};
    s_proxy.OnBeforeWidget(_t);
    s_proxy.OnAfterWidget();

    // held: update rotation
    if (bHold)
    {
        const ImVec2 delta = s_dragHelper.GetDragDelta();
        if (!IsZeroApprox(delta))
        {
            constexpr float k_sensitivity = 0.005f;

            // apply rotation
            Vec3& euler = s_proxy.GetEulerProxy();
            euler.x     = Wrap(euler.x + delta.x * k_sensitivity, -k_2pi, k_2pi);
            euler.y     = Clamp(euler.y + delta.y * k_sensitivity, ToRad(-89.f), ToRad(89.f));
            euler.z     = 0.f;   // no roll
            _t.SetRotation(euler);

            // dirty check
            bChanged = true;
        }
    }

    // background circle
    const eEditorStyleColor col = bHold    ? eEditorStyleColor::AcroballBgActive
                                : bHovered ? eEditorStyleColor::AcroballBgHovered
                                           : eEditorStyleColor::AcroballBg;

    const ImU32 bgCol     = JAM_EDITOR.GetStyleColorU32(col);
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();
    pDrawList->AddCircleFilled(center, radius, bgCol, 64);

    // value
    const ImVec4 axisXCol = JAM_EDITOR.GetStyleColor(eEditorStyleColor::AcroballAxisX);
    const ImVec4 axisYCol = JAM_EDITOR.GetStyleColor(eEditorStyleColor::AcroballAxisY);
    const ImVec4 axisZCol = JAM_EDITOR.GetStyleColor(eEditorStyleColor::AcroballAxisZ);

    const ImVec4 axisXOutlineCol = JAM_EDITOR.GetStyleColor(eEditorStyleColor::AcroballAxisXOutline);
    const ImVec4 axisYOutlineCol = JAM_EDITOR.GetStyleColor(eEditorStyleColor::AcroballAxisYOutline);
    const ImVec4 axisZOutlineCol = JAM_EDITOR.GetStyleColor(eEditorStyleColor::AcroballAxisZOutline);

    // style value
    const float axisLineThickness    = JAM_EDITOR.GetStyleVar(eEditorStyleVar::AcroballAxisLineThickness);
    const float axisLineRatio        = JAM_EDITOR.GetStyleVar(eEditorStyleVar::AcroballAxisLineLengthRatio);
    const float axisLineLen          = radius * axisLineRatio;
    const float headRadius           = JAM_EDITOR.GetStyleVar(eEditorStyleVar::AcroballAxisHeadRadius);
    const float headOutlineThickness = JAM_EDITOR.GetStyleVar(eEditorStyleVar::AcroballAxisHeadOutlineThickness);

    // acroball data
    const Vec3 axisX = _t.GetRight();
    const Vec3 axisY = _t.GetUp();
    const Vec3 axisZ = _t.GetForward();

    // axis data
    AcroballAxisData axes[6] = {
        {  "X", axisXCol, axisXOutlineCol,  axisX,  axisX.z,  true },
        { "-X", axisXCol, axisXOutlineCol, -axisX, -axisX.z, false },
        {  "Y", axisYCol, axisYOutlineCol,  axisY,  axisY.z,  true },
        { "-Y", axisYCol, axisYOutlineCol, -axisY, -axisY.z, false },
        {  "Z", axisZCol, axisZOutlineCol,  axisZ,  axisZ.z,  true },
        { "-Z", axisZCol, axisZOutlineCol, -axisZ, -axisZ.z, false },
    };

    // sort by depth
    std::ranges::sort(axes, AcroballAxisData::CompareDepthAscending);

    // render
    for (const AcroballAxisData& axis: axes)
    {
        const ImVec2 endPos = { center.x + axis.dir.x * axisLineLen, center.y - axis.dir.y * axisLineLen };

        // depth based value (far faces are darker)
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

bool Widget_FileDialog(
    const char* _label,
    Path&       _path,
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
    if (ImGui::InputText(_label, const_cast<std::string*>(&_path.native()), ImGuiInputTextFlags_ReadOnly))
    {
        bChanged = true;
    }

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

bool Widget_InputText(
    const char*                  _label,
    String&                      _str,
    ImGuiInputTextFlags          _flags,
    const ImGuiInputTextCallback _callback,
    void*                        _pUserData)
{
    JAM_ASSERT((_flags & ImGuiInputTextFlags_CallbackResize) == 0);
    _flags |= ImGuiInputTextFlags_CallbackResize;

    WidgetInputTextCallback_UserData userData = {};
    userData.pStr                             = &_str;
    userData.ChainCallback                    = _callback;
    userData.pChainUserData                   = _pUserData;
    return ImGui::InputText(_label, static_cast<char*>(_str.GetPtr()), _str.GetCapacity() + 1, _flags, Widget_InputTextCallback, &userData);
}

bool Widget_EnumCombo(
    const char*                     _label,
    EnumComboItem&                  _curItem,
    const Span<const EnumComboItem> _items)
{
    bool bChanged = false;
    if (ImGui::BeginCombo(_label, _curItem.name, ImGuiComboFlags_NoArrowButton))
    {
        for (const EnumComboItem& item: _items)
        {
            const bool bSelected = (_curItem.value == item.value);
            if (ImGui::Selectable(item.name, bSelected))
            {
                _curItem = item;
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

bool Widget_EnumToggle(
    const char*                     _label,
    EnumComboItem&                  _curItem,
    const Span<const EnumComboItem> _items)
{
    ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return false;
    }

    const ImGuiStyle& style = ImGui::GetStyle();
    const float       w     = ImGui::CalcItemWidth();
    const ImGuiID     id    = pWin->GetID(_label);
    const ImVec2      size  = { w, ImGui::GetFrameHeight() };
    const ImRect      bb    = { ImGui::GetCursorScreenPos(), ImGui::GetCursorScreenPos() + size };
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id))
    {
        return false;
    }

    ImGui::BeginGroup();
    ImGui::PushID(_label);

    // frame
    ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

    // buttons
    const float  btnWidth = Div(bb.GetWidth(), NumericCast<float>(_items.size()));
    const ImVec2 btnSize  = { btnWidth, bb.GetHeight() };
    bool         bChanged = false;
    Int32        idx      = 0;
    Int32        curIdx   = 0;
    for (const EnumComboItem& item: _items)
    {
        const ImVec2  btnP0 = { bb.Min.x + btnWidth * NumericCast<float>(idx), bb.Min.y };
        const ImVec2  btnP1 = btnP0 + btnSize;
        const ImRect  btnBb = { btnP0, btnP1 };
        const ImGuiID btnId = pWin->GetID(idx);

        bool bSelected = false;
        if (_curItem.value == item.value)
        {
            bSelected = true;
            curIdx    = idx;
        }

        bool       bHovered = false;
        bool       bHeld    = false;
        const bool bPressed = ImGui::ButtonBehavior(btnBb, btnId, &bHovered, &bHeld);
        if (bPressed)
        {
            _curItem = item;
            bChanged = true;
        }

        // render button
        const ImU32 col = ImGui::GetColorU32((bHeld && bHovered) ? ImGuiCol_ButtonActive
                                             : bHovered          ? ImGuiCol_ButtonHovered
                                             : bSelected         ? ImGuiCol_Button
                                                                 : ImGuiCol_FrameBg);

        ImGui::RenderNavCursor(btnBb, btnId);
        ImGui::RenderFrame(btnBb.Min, btnBb.Max, col, true, style.FrameRounding);
        ImGui::RenderTextClipped(btnBb.Min + style.FramePadding, btnBb.Max - style.FramePadding, item.name, nullptr, nullptr, style.ButtonTextAlign, &btnBb);
        ++idx;
    }

    ImGui::PopID();
    ImGui::EndGroup();

    // short cut
    const ImGuiLastItemData& lastData = ImGui::GetCurrentContext()->LastItemData;
    if (lastData.StatusFlags & ImGuiItemStatusFlags_HasShortcut)
    {
        // toggle wrap around
        if (ImGui::Shortcut(lastData.Shortcut))
        {
            const Int32 wrappedIdx = (curIdx + 1) % _items.size();
            _curItem               = _items[wrappedIdx];
            bChanged               = true;
        }
    }

    return bChanged;
}

}   // namespace Jam