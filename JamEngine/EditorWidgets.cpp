#include "pch.h"

#include "EditorWidgets.h"

#include "AssetManager.h"
#include "Camera.h"
#include "DialogUtilities.h"
#include "Editor.h"
#include "EditorUtilities.h"
#include "Entity.h"
#include "FString.h"
#include "Graphics.h"
#include "ImGuiUtilities.h"
#include "Input.h"
#include "SceneManager.h"
#include "Transform.h"
#include "VertexLayoutStorage.h"

namespace jam
{
constexpr ImGuiDataTypeInfo k_dataTypeInfoTable[] = {
    {           sizeof(char),     "S8",    "%d",    "%d" }, // ImGuiDataType_S8
    {  sizeof(unsigned char),     "U8",    "%u",    "%u" },
    {          sizeof(short),    "S16",    "%d",    "%d" }, // ImGuiDataType_S16
    { sizeof(unsigned short),    "U16",    "%u",    "%u" },
    {            sizeof(int),    "S32",    "%d",    "%d" }, // ImGuiDataType_S32
    {   sizeof(unsigned int),    "U32",    "%u",    "%u" },
#ifdef _MSC_VER
    {          sizeof(ImS64),    "S64", "%I64d", "%I64d" }, // ImGuiDataType_S64
    {          sizeof(ImU64),    "U64", "%I64u", "%I64u" },
#else
    { sizeof(ImS64), "S64", "%lld", "%lld" },   // ImGuiDataType_S64
    { sizeof(ImU64), "U64", "%llu", "%llu" },
#endif
    {          sizeof(float),  "float",  "%.3f",    "%f" }, // ImGuiDataType_Float (float are promoted to double in va_arg)
    {         sizeof(double), "double",    "%f",   "%lf" }, // ImGuiDataType_Double
    {           sizeof(bool),   "bool",    "%d",    "%d" }, // ImGuiDataType_Bool
    {                      0,  "char*",    "%s",    "%s" }, // ImGuiDataType_String
};

IM_STATIC_ASSERT(IM_ARRAYSIZE(k_dataTypeInfoTable) == ImGuiDataType_COUNT);

}   // namespace jam

namespace jam
{

void Widget_CenteredText(
    const ImVec2 _region,
    const char*  _fmt,
    ...)
{
    va_list args;
    va_start(args, _fmt);
    Widget_CenteredTextV(_region, _fmt, args);
    va_end(args);
}

void Widget_CenteredTextV(
    const ImVec2  _region,
    const char*   _fmt,
    const va_list _args)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        return;
    }

    const char*  str      = FormatStringToTmpBufferV(_fmt, _args);
    const ImVec2 textSize = ImGui::CalcTextSize(str, NULL, true);
    const ImVec2 offset   = (_region - textSize) * 0.5f;
    ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + offset);
    ImGui::TextV(_fmt, _args);
}

void Widget_FrameAlignedText(
    const char* _fmt,
    ...)
{
    ImGui::AlignTextToFramePadding();

    va_list args;
    va_start(args, _fmt);
    ImGui::TextV(_fmt, args);
    va_end(args);
}

void Widget_FrameAlignedTextV(
    const char*   _fmt,
    const va_list _args)
{
    ImGuiWindow* pWin = ImGui::GetCurrentWindow();
    if (pWin->SkipItems)
    {
        return;
    }

    ImGui::AlignTextToFramePadding();
    ImGui::TextV(_fmt, _args);
}

bool Widget_DragTranslation(
    const char* _label,
    Vec3*       _translation,
    const float _speed,
    const float _min,
    const float _max)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        return false;
    }

    ImGui::PushID(_label);
    ImGui::BeginGroup();

    const ImGuiStyle& style          = ImGui::GetStyle();
    const ImVec2      btnSize        = { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };
    const float       availWidth     = ImGui::GetContentRegionAvail().x;
    const float       spacing        = style.ItemInnerSpacing.x;
    const float       dragInputWidth = (availWidth - (btnSize.x * 3.f) - (spacing * 5.f)) / 3.f;
    bool              bChanged       = false;

    if (ImGui::Button("X", btnSize))
    {
        _translation->x = 0.f;
        bChanged        = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragInputWidth);
    bChanged |= Widget_DragFloat("##x", &_translation->x, _speed, _min, _max, "%.2f", ImGuiSliderFlags_AlwaysClamp);

    ImGui::SameLine(0, spacing);
    if (ImGui::Button("Y", btnSize))
    {
        _translation->y = 0.f;
        bChanged        = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragInputWidth);
    bChanged |= Widget_DragFloat("##y", &_translation->y, _speed, _min, _max, "%.2f", ImGuiSliderFlags_AlwaysClamp);

    ImGui::SameLine(0, spacing);
    if (ImGui::Button("Z", btnSize))
    {
        _translation->z = 0.f;
        bChanged        = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragInputWidth);
    bChanged |= Widget_DragFloat("##z", &_translation->z, _speed, _min, _max, "%.2f", ImGuiSliderFlags_AlwaysClamp);

    ImGui::EndGroup();
    ImGui::PopID();
    return bChanged;
}

bool Widget_DragScale(
    const char* _label,
    Vec3&       _scaleNorm,
    const float _speedPerc,
    const float _minPerc,
    const float _maxPerc)
{
    Vec3 scalePerc = _scaleNorm * 100.f;
    bool bChanged  = Widget_DragScalePerc(_label, scalePerc, _speedPerc, _minPerc, _maxPerc);
    if (bChanged)
    {
        _scaleNorm = scalePerc * 0.01f;
    }
    return bChanged;
}

bool Widget_DragScalePerc(
    const char* _label,
    Vec3&       _scalePerc,
    const float _speedPerc,
    const float _minPerc,
    const float _maxPerc)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        return false;
    }

    ImGui::PushID(_label);
    ImGui::BeginGroup();

    const ImGuiStyle& style          = ImGui::GetStyle();
    const ImVec2      btnSize        = { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };
    const float       availWidth     = ImGui::GetContentRegionAvail().x;
    const float       spacing        = style.ItemInnerSpacing.x;
    const float       dragInputWidth = (availWidth - (btnSize.x * 3.f) - (spacing * 5.f)) / 3.f;
    bool              bChanged       = false;

    if (ImGui::Button("X", btnSize))
    {
        _scalePerc.x = 100.f;
        bChanged     = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragInputWidth);
    bChanged |= Widget_DragFloat("##x", &_scalePerc.x, _speedPerc, _minPerc, _maxPerc, "%.0f %%", ImGuiSliderFlags_AlwaysClamp);

    ImGui::SameLine(0, spacing);
    if (ImGui::Button("Y", btnSize))
    {
        _scalePerc.y = 100.f;
        bChanged     = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragInputWidth);
    bChanged |= Widget_DragFloat("##y", &_scalePerc.y, _speedPerc, _minPerc, _maxPerc, "%.0f %%", ImGuiSliderFlags_AlwaysClamp);

    ImGui::SameLine(0, spacing);
    if (ImGui::Button("Z", btnSize))
    {
        _scalePerc.z = 100.f;
        bChanged     = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragInputWidth);
    bChanged |= Widget_DragFloat("##z", &_scalePerc.z, _speedPerc, _minPerc, _maxPerc, "%.0f %%", ImGuiSliderFlags_AlwaysClamp);

    ImGui::EndGroup();
    ImGui::PopID();
    return bChanged;
}

bool Widget_DragPitchYawRoll(
    const char* _label,
    Vec3&       _pitchYawRollRad,
    const float _speedDeg,
    const float _minDeg,
    const float _maxDeg)
{
    Vec3       pitchYawRollDeg = ToDeg(_pitchYawRollRad);
    const bool bChanged        = Widget_DragPitchYawRollDeg(_label, pitchYawRollDeg, _speedDeg, _minDeg, _maxDeg);
    if (bChanged)
    {
        _pitchYawRollRad = ToRad(pitchYawRollDeg);
    }
    return bChanged;
}

bool Widget_DragPitchYaw(
    const char* _label,
    Vec2&       _pitchYawRad,
    const float _speedDeg,
    const float _minDeg,
    const float _maxDeg)
{
    Vec2       pitchYawDeg = ToDeg(_pitchYawRad);
    const bool bChanged    = Widget_DragPitchYawDeg(_label, pitchYawDeg, _speedDeg, _minDeg, _maxDeg);
    if (bChanged)
    {
        _pitchYawRad = ToRad(pitchYawDeg);
    }

    return bChanged;
}

bool Widget_DragPitchYawRollDeg(
    const char* _label,
    Vec3&       _pitchYawRollDeg,
    const float _speedDeg,
    const float _minDeg,
    const float _maxDeg)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        return false;
    }

    ImGui::PushID(_label);
    ImGui::BeginGroup();

    const ImGuiStyle& style          = ImGui::GetStyle();
    const ImVec2      btnSize        = { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };
    const float       availWidth     = ImGui::GetContentRegionAvail().x;
    const float       spacing        = style.ItemInnerSpacing.x;
    const float       dragInputWidth = (availWidth - (btnSize.x * 3.f) - (spacing * 5.f)) / 3.f;
    bool              bChanged       = false;

    if (ImGui::Button("X", btnSize))
    {
        _pitchYawRollDeg.x = 0.f;
        bChanged           = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragInputWidth);
    bChanged |= Widget_DragFloat("##pitch", &_pitchYawRollDeg.x, _speedDeg, _minDeg, _maxDeg, "%.0f deg", ImGuiSliderFlags_WrapAround);

    ImGui::SameLine(0, spacing);
    if (ImGui::Button("Y", btnSize))
    {
        _pitchYawRollDeg.y = 0.f;
        bChanged           = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragInputWidth);
    bChanged |= Widget_DragFloat("##yaw", &_pitchYawRollDeg.y, _speedDeg, _minDeg, _maxDeg, "%.0f deg", ImGuiSliderFlags_WrapAround);

    ImGui::SameLine(0, spacing);
    if (ImGui::Button("Z", btnSize))
    {
        _pitchYawRollDeg.z = 0.f;
        bChanged           = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragInputWidth);
    bChanged |= Widget_DragFloat("##roll", &_pitchYawRollDeg.z, _speedDeg, _minDeg, _maxDeg, "%.0f deg", ImGuiSliderFlags_WrapAround);

    ImGui::EndGroup();
    ImGui::PopID();
    return bChanged;
}

bool Widget_DragPitchYawDeg(
    const char* _label,
    Vec2&       _pitchYawDeg,
    const float _speedDeg,
    const float _minDeg,
    const float _maxDeg)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        return false;
    }

    ImGui::PushID(_label);
    ImGui::BeginGroup();

    const ImGuiStyle& style          = ImGui::GetStyle();
    const ImVec2      btnSize        = { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };
    const float       availWidth     = ImGui::GetContentRegionAvail().x;
    const float       spacing        = style.ItemInnerSpacing.x;
    const float       dragInputWidth = (availWidth - (btnSize.x * 2.f) - (spacing * 3.f)) / 2.f;
    bool              bChanged       = false;

    if (ImGui::Button("X", btnSize))
    {
        _pitchYawDeg.x = 0.f;
        bChanged       = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragInputWidth);
    bChanged |= Widget_DragFloat("##pitch", &_pitchYawDeg.x, _speedDeg, _minDeg, _maxDeg, "%.0f deg", ImGuiSliderFlags_WrapAround);

    ImGui::SameLine(0, spacing);
    if (ImGui::Button("Y", btnSize))
    {
        _pitchYawDeg.y = 0.f;
        bChanged       = true;
    }
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(dragInputWidth);
    bChanged |= Widget_DragFloat("##yaw", &_pitchYawDeg.y, _speedDeg, _minDeg, _maxDeg, "%.0f deg", ImGuiSliderFlags_WrapAround);

    ImGui::EndGroup();
    ImGui::PopID();
    return bChanged;
}

bool Widget_DragAngle(
    const char* _label,
    float&      _pRad,
    const float _degSpeed,
    const float _minDeg,
    const float _maxDeg)
{
    float deg      = ToDeg(_pRad);
    bool  bChanged = Widget_DragFloat(_label, &deg, _degSpeed, _minDeg, _maxDeg, "%.0f deg", ImGuiSliderFlags_WrapAround);
    if (bChanged)
        _pRad = ToRad(deg);
    return bChanged;
}

bool Widget_DragPercent(
    const char* _label,
    float&      _pNorm,
    const float _speedPerc,
    const float _minPerc,
    const float _maxPerc,
    const char* _format)
{
    float perc     = _pNorm * 100.f;
    bool  bChanged = Widget_DragFloat(_label, &perc, _speedPerc, _minPerc, _maxPerc, _format, ImGuiSliderFlags_AlwaysClamp);
    if (bChanged)
    {
        _pNorm = perc / 100.f;
    }

    return bChanged;
}

void Widget_BorderedText(
    const char*   _text,
    const ImVec2& _size,
    const bool    _bDisabled)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        return;
    }

    ImGuiContext&     g        = *GImGui;
    const ImGuiStyle& style    = g.Style;
    const ImVec2      textSize = ImGui::CalcTextSize(_text, NULL, true);
    const ImVec2      pos      = window->DC.CursorPos;
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

void Widget_BorderedTextColored(
    const ImVec4& _color,
    const char*   _text,
    const ImVec2& _size)
{
    ImGui::PushStyleColor(ImGuiCol_Text, _color);
    Widget_BorderedText(_text, _size);
    ImGui::PopStyleColor();
}

void Widget_ShadowedText(
    const char* _fmt,
    ...)
{
    va_list args;
    va_start(args, _fmt);
    Widget_ShadowedTextV(_fmt, args);
    va_end(args);
}

void Widget_ShadowedTextV(
    const char*   _fmt,
    const va_list _args)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    const char* text = FormatStringToTmpBufferV(_fmt, _args);
    if (!text || *text == '\0')
        return;

    const ImVec2 textPos  = { window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset };
    const ImVec2 textSize = ImGui::CalcTextSize(text);
    const ImRect bb       = { textPos, textPos + textSize };

    ImGui::ItemSize(textSize, 0);
    if (!ImGui::ItemAdd(bb, 0))
        return;

    constexpr ImVec2 k_shadowOffset = ImVec2(1.f, 1.f);
    ImDrawList*      pDrawList      = ImGui::GetWindowDrawList();

    pDrawList->AddText(textPos + k_shadowOffset, IM_COL32_BLACK, text);
    pDrawList->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), text);
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

bool Widget_VertexLayoutEntryCombo(
    const char*             _label,
    Ref<VertexLayoutEntry>& _pVle)
{
    const VertexLayoutStorage& storage  = Editor::GetVertexLayoutStorage();
    const char*                preview  = _pVle ? _pVle->GetName().c_str() : "none";
    bool                       bChanged = false;

    if (ImGui::BeginCombo(_label, preview, ImGuiComboFlags_NoArrowButton))
    {
        for (const auto& [name, pVle]: storage)
        {
            const char* label     = name.c_str();
            const bool  bSelected = (_pVle && name == _pVle->GetName());

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

bool Widget_ProjectionEnumCombo(
    const char*  _label,
    eProjection& _projection)
{
    constexpr const char* k_names[] = { "perspective", "orthographic" };
    return Widget_EnumCombo(_label, reinterpret_cast<Int32&>(_projection), k_names);
}

bool Widget_MeshSelector(
    const char* _label,
    Ref<Mesh>&  _pMesh)
{
    ImGui::PushID(_label);
    bool bChanged = false;

    // preview item
    const char* text = _pMesh ? _pMesh->GetPath().c_str() : "no mesh";
    Widget_BorderedText(text, CalcFitButtonSize());

    // tooltip and popup
    const bool bTooltip = ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip);
    ImGui::OpenPopupOnItemClick("Menu");

    // drag drop
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* pPayload = AcceptFileDragDropPayload())
        {
            const fs::path path = ResolvePathDragDropPayload(pPayload);
            if (const std::optional<Ref<Mesh>> opt = AssetManager::GetOrLoadMesh(path))
            {
                _pMesh   = opt.value();
                bChanged = true;
            }
            else
            {
                std::string msg = std::format("failed to load mesh from path: {}\nerror: {}", path.native(), ErrorBuffer::Get());
                Editor::OpenCorePopupDialog("error", msg);
            }
        }
        ImGui::EndDragDropTarget();
    }

    // menu
    if (ImGui::BeginPopup("Menu"))
    {
        if (ImGui::MenuItem("clear"))
        {
            _pMesh   = nullptr;
            bChanged = true;
        }
        ImGui::EndPopup();
    }

    // tooltip
    if (bTooltip && ImGui::BeginTooltip())
    {
        constexpr const char* k_nullMsg = "drag & drop .jmsh file here!";
        ImGui::TextUnformatted(_pMesh ? text : k_nullMsg);
        ImGui::EndTooltip();
    }

    ImGui::PopID();
    return bChanged;
}

bool Widget_MaterialSelector(
    const char*    _label,
    Ref<Material>& _pMaterial)
{
    ImGui::PushID(_label);
    bool bChanged = false;

    // preview item
    const char* text = _pMaterial ? _pMaterial->GetPath().c_str() : "no material";
    Widget_BorderedText(text, CalcFitButtonSize());

    const bool bTooltip = ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip);
    ImGui::OpenPopupOnItemClick("Menu");

    // drag drop
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* pPayload = AcceptFileDragDropPayload())
        {
            const fs::path path = ResolvePathDragDropPayload(pPayload);
            if (std::optional<Ref<Material>> opt = AssetManager::GetOrLoadMaterial(path))
            {
                _pMaterial = opt.value();
                bChanged   = true;
            }
            else
            {
                std::string msg = std::format("failed to load material from path: {}\nerror: {}", path.native(), ErrorBuffer::Get());
                Editor::OpenCorePopupDialog("error", msg);
            }
        }
        ImGui::EndDragDropTarget();
    }

    // menu
    if (ImGui::BeginPopup("Menu"))
    {
        if (ImGui::MenuItem("clear"))
        {
            _pMaterial = nullptr;
            bChanged   = true;
        }
        ImGui::EndPopup();
    }

    // tooltip
    if (bTooltip && ImGui::BeginTooltip())
    {
        constexpr const char* k_nullMsg = "drag & drop .jmat file here!";
        ImGui::TextUnformatted(_pMaterial ? text : k_nullMsg);
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
        ImGui::ImageWithBg(ImTextureRefFrom(_pImageTexture->GetHandle()), _size);
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
            if (std::optional<Ref<ImageTexture>> opt = AssetManager::GetOrLoadImageTexture(path))
            {
                _pImageTexture = opt.value();
                bChanged       = true;
            }
            else
            {
                const std::string msg = std::format("failed to load image texture from path: {}\nerror: {}", path.native(), ErrorBuffer::Get());
                Editor::OpenCorePopupDialog("error", msg);
            }
        }

        ImGui::EndDragDropTarget();
    }

    // tooltip
    if (bTooltip && ImGui::BeginTooltip())
    {
        if (_pImageTexture)
        {
            constexpr const char* const k_typenames[] = { "texture 2D", "texture 3D", "texture cube" };

            TextureInfo info = Graphics::GetTextureInfo(_pImageTexture->GetHandle());
            ImGui::Text("path: %s", _pImageTexture->GetPath().c_str());
            ImGui::Text("size: %d x %d x %d", info.width, info.height, info.depth);
            ImGui::Text("type: %s", k_typenames[IntFrom(info.type)]);
            ImGui::Text("format: %s", StringFrom(info.format).data());
            ImGui::Text("array size: %d", info.arraySize);
            ImGui::Text("samples: %d", info.samples);
            ImGui::Text(" mip levels: %d", info.numMips);
        }
        else
        {
            ImGui::TextUnformatted("no image!\ndrag & drop image file here to load");
        }
        ImGui::EndTooltip();
    }

    // item popup
    if (ImGui::BeginPopup("Menu"))
    {
        if (ImGui::MenuItem("clear"))
        {
            _pImageTexture = nullptr;
            bChanged       = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::PopID();
    return bChanged;
}

bool Widget_DragScalar(
    const char*            label,
    const ImGuiDataType    data_type,
    void*                  p_data,
    const float            v_speed,
    const void*            p_min,
    const void*            p_max,
    const char*            format,
    const ImGuiSliderFlags flags)
{
    static bool   s_bIsCapturingMouse = false;
    static ImVec2 s_capturedMousePos  = ImVec2(0.f, 0.f);

    ImGuiIO&      io                = ImGui::GetIO();
    const bool    bDirty            = ImGui::DragScalar(label, data_type, p_data, v_speed, p_min, p_max, format, flags);
    const ImGuiID id                = ImGui::GetItemID();
    const bool    bIsActivated      = ImGui::IsItemActivated();
    const bool    bIsActive         = ImGui::IsItemActive();
    const bool    bIsDeactived      = ImGui::IsItemDeactivated();
    const bool    bIsTmpInputActive = ImGui::TempInputIsActive(id);

    if (bIsActivated)
    {
        s_capturedMousePos  = ImGui::GetMousePos();
        s_bIsCapturingMouse = true;
        Input::SetMouseVisibility(false);
    }

    if (bIsActive && s_bIsCapturingMouse)
    {
        const Int32 x = static_cast<Int32>(s_capturedMousePos.x);
        const Int32 y = static_cast<Int32>(s_capturedMousePos.y);

        Input::SetMousePos(x, y);
        io.MousePos = s_capturedMousePos;
    }

    if ((bIsDeactived || bIsTmpInputActive) && s_bIsCapturingMouse)
    {
        s_bIsCapturingMouse = false;
        Input::SetMouseVisibility(true);
    }

    return bDirty;
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
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        return false;
    }

    const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    bool        bDirty  = false;

    ImGui::BeginGroup();

    ImGui::PushID(_label);
    ImGui::PushMultiItemsWidths(_numComps, ImGui::CalcItemWidth());
    const size_t typesize = k_dataTypeInfoTable[_dataType].Size;
    for (Int32 i = 0; i < _numComps; i++)
    {
        ImGui::PushID(i);
        if (i > 0)
        {
            ImGui::SameLine(0, spacing);
        }

        bDirty |= Widget_DragScalar("", _dataType, _pData, _speed, _pMin, _pMax, _format, _flags);
        _pData = static_cast<void*>(static_cast<char*>(_pData) + typesize);
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
    return Widget_DragScalar(label, ImGuiDataType_Float, p_data, v_speed, &v_min, &v_max, format, flags);
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
    return Widget_DragScalarN(label, ImGuiDataType_Float, p_data, 2, v_speed, &v_min, &v_max, format, flags);
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
    return Widget_DragScalarN(label, ImGuiDataType_Float, p_data, 3, v_speed, &v_min, &v_max, format, flags);
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
    return Widget_DragScalarN(label, ImGuiDataType_Float, p_data, 4, v_speed, &v_min, &v_max, format, flags);
}

bool Widget_IconButton(
    const char*        _label,
    const ImTextureRef _icon,
    const ImVec2       _size)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        return false;
    }

    ImGuiContext&     g        = *GImGui;
    const ImGuiStyle& style    = g.Style;
    const ImGuiID     id       = window->GetID(_label);
    const ImVec2      textSize = ImGui::CalcTextSize(_label, NULL, true);
    const ImVec2      iconSize = { ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight() };
    const float       defaultW = iconSize.x + textSize.x + style.ItemInnerSpacing.x + style.FramePadding.x * 2.0f;
    const float       defaultH = iconSize.y + style.FramePadding.y * 2.0f;
    const ImVec2      size     = ImGui::CalcItemSize(_size, defaultW, defaultH);

    const ImVec2 cursor = window->DC.CursorPos;
    const ImRect bb     = { cursor, cursor + size };
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
    {
        return false;
    }

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    // render frame
    const ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive
                                         : hovered         ? ImGuiCol_ButtonHovered
                                                           : ImGuiCol_Button);
    ImGui::RenderNavCursor(bb, id);
    ImGui::RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);

    // render image
    ImDrawList*  pDrawList   = ImGui::GetWindowDrawList();
    const ImVec2 imageMinPos = cursor + style.FramePadding;
    pDrawList->AddImage(_icon, imageMinPos, imageMinPos + iconSize);

    // render text
    const ImVec2 posMin = { imageMinPos.x + iconSize.x + style.ItemInnerSpacing.x, imageMinPos.y };
    const ImVec2 posMax = { bb.Max.x - style.FramePadding.x, bb.Max.y - style.FramePadding.y };
    ImGui::RenderTextClipped(posMin, posMax, _label, NULL, &textSize, style.ButtonTextAlign, &bb);
    return pressed;
}

bool Widget_Acroball(
    const char*  _label,
    Transform&   _transform,
    const ImVec2 _pos,
    const float  _length)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        return false;
    }

    ImGuiContext&     g     = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID     id    = window->GetID(_label);

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
    bool bHovered, bHold;
    bool bReleased = ImGui::ButtonBehavior(bb, id, &bHovered, &bHold);
    bool bPressed  = bHold && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool bDirty    = false;

    // infinite drag
    static bool   s_bIsCapturingMouse = false;
    static ImVec2 s_capturedMousePos  = ImVec2(0.f, 0.f);
    static float  s_pitchRad          = 0.f;
    static float  s_yawRad            = 0.f;

    if (bPressed)
    {
        // capture current rotation
        const auto [p, y, _] = _transform.ToPitchYawRoll();
        s_pitchRad           = p;
        s_yawRad             = y;

        // freeze cursor
        s_capturedMousePos  = ImGui::GetMousePos();
        s_bIsCapturingMouse = true;
        Input::SetMouseVisibility(false);
    }

    if (bReleased && s_bIsCapturingMouse)
    {
        s_bIsCapturingMouse = false;
        Input::SetMouseVisibility(true);
    }

    if (bHold)
    {
        const auto [dx, dy] = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.5f);

        if (!(IsZeroApprox(dx) && IsZeroApprox(dy)))
        {
            constexpr float k_sensitivity = 0.005f;
            ImGuiIO&        io            = ImGui::GetIO();

            // apply rotation
            s_yawRad += dx * k_sensitivity;
            s_yawRad = Wrap(s_yawRad, -k_2pi, k_2pi);
            s_pitchRad += dy * k_sensitivity;
            s_pitchRad = Clamp(s_pitchRad, ToRad(-89.f), ToRad(89.f));
            _transform.SetRotation(s_pitchRad, s_yawRad, 0.f);

            // freeze cursor
            const Int32 x = static_cast<Int32>(s_capturedMousePos.x);
            const Int32 y = static_cast<Int32>(s_capturedMousePos.y);
            Input::SetMousePos(x, y);
            io.MousePos = s_capturedMousePos;

            // dirty check
            bDirty = true;
        }
    }

    // render
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();

    // background circle
    {
        const eEditorStyleColor colIdx = bHold    ? eEditorStyleColor_AcroballBgActive
                                       : bHovered ? eEditorStyleColor_AcroballBgHovered
                                                  : eEditorStyleColor_AcroballBg;

        const ImU32 bgCol = Editor::GetStyleColorU32(colIdx);
        pDrawList->AddCircleFilled(center, radius, bgCol, 64);
    }

    // axis lines and circles
    constexpr float             k_circleRadius          = 10.f;
    constexpr float             k_circleOutlineTickness = 2.f;
    constexpr float             k_lineThickness         = 3.5f;
    constexpr const char* const k_axisNames[]           = { "X", "Y", "Z" };

    enum class eAxis
    {
        X,
        Y,
        Z
    };

    struct AxisData
    {
        static bool CompareDepthAscending(const AxisData& a, const AxisData& b)
        {
            return a.depth < b.depth;
        }

        Vec3  dir;
        eAxis axis;
        float depth;
        bool  bPos;
    };

    const ImVec4 colors[] = {
        Editor::GetStyleColor(eEditorStyleColor_AxisX),
        Editor::GetStyleColor(eEditorStyleColor_AxisY),
        Editor::GetStyleColor(eEditorStyleColor_AxisZ)
    };

    const Vec3 axisX = _transform.GetRight();
    const Vec3 axisY = _transform.GetUp();
    const Vec3 axisZ = _transform.GetForward();

    AxisData axes[6] = {
        {  axisX, eAxis::X,  axisX.z,  true },
        { -axisX, eAxis::X, -axisX.z, false },
        {  axisY, eAxis::Y,  axisY.z,  true },
        { -axisY, eAxis::Y, -axisY.z, false },
        {  axisZ, eAxis::Z,  axisZ.z,  true },
        { -axisZ, eAxis::Z, -axisZ.z, false }
    };

    // sort for rendering
    std::ranges::sort(axes, AxisData::CompareDepthAscending);

    // render
    const float axisLen = radius * 0.7f;
    for (const AxisData& axis: axes)
    {
        const ImVec2 endPos = { center.x + axis.dir.x * axisLen, center.y - axis.dir.y * axisLen };

        // depth based color
        const float depth01     = axis.depth + 1.f;
        float       t0          = Lerp(0.5f, 1.f, depth01);   // bright for front faces
        t0                      = Clamp(t0, 0.f, 1.f);
        const ImVec4 baseColor  = colors[IntFrom(axis.axis)];
        const ImVec4 finalColor = { baseColor.x * t0, baseColor.y * t0, baseColor.z * t0, baseColor.w };
        const ImU32  colorU32   = ImGui::GetColorU32(finalColor);

        // draw line + circle
        pDrawList->AddLine(center, endPos, colorU32, k_lineThickness);
        pDrawList->AddCircleFilled(endPos, k_circleRadius, colorU32, 20);

        // draw outline
        constexpr float k_t1         = 1.2f;
        const ImVec4    outlineColor = {
            finalColor.x * k_t1,
            finalColor.y * k_t1,
            finalColor.z * k_t1,
            finalColor.w
        };

        pDrawList->AddCircle(endPos, k_circleRadius, ImGui::GetColorU32(outlineColor), 20, k_circleOutlineTickness);

        // draw label
        if (axis.bPos)
        {
            const char*  text     = k_axisNames[IntFrom(axis.axis)];
            const ImVec2 textSize = ImGui::CalcTextSize(text);
            const ImVec2 textPos  = endPos - textSize * 0.5f;
            pDrawList->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), text);
        }
    }

    return bDirty;
}

bool Widget_OpenFileDialog(
    const char*                             _label,
    fs::path&                               _path,
    const std::span<const FileDialogFilter> _filters)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        return false;
    }

    const ImGuiStyle& style      = ImGui::GetStyle();
    const float       width      = ImGui::CalcItemWidth();
    const ImVec2      btnSize    = ImGui::CalcTextSize("open") + style.FramePadding * 2;
    const float       inputWidth = Max(0.f, width - btnSize.x - style.ItemSpacing.x);
    bool              bDirty     = false;

    ImGui::SetNextItemWidth(inputWidth);
    ImGui::InputText(_label, const_cast<std::string*>(&_path.native()), ImGuiInputTextFlags_ReadOnly);

    // tooltip
    if (ImGui::BeginItemTooltip())
    {
        ImGui::TextUnformatted(_path.empty() ? "no file selected" : _path.c_str());
        ImGui::EndTooltip();
    }

    // open
    ImGui::SameLine();
    if (ImGui::Button("open", btnSize))
    {
        if (std::optional<fs::path> opt = OpenFileDialog("open file", {}, _filters))
        {
            _path  = *opt;
            bDirty = true;
        }
    }

    return bDirty;
}

bool Widget_TextureChannelCombo(
    const char*      _label,
    eTextureChannel& _colorChannel,
    const bool       _bExcludeAlphaChannel)
{
    if (_bExcludeAlphaChannel)
    {
        const char* k_names[] = { "R", "G", "B" };
        return Widget_EnumCombo(_label, reinterpret_cast<Int32&>(_colorChannel), k_names);
    }
    else
    {
        const char* k_names[] = { "R", "G", "B", "A" };
        return Widget_EnumCombo(_label, reinterpret_cast<Int32&>(_colorChannel), k_names);
    }
}

bool Widget_TextureFormatCombo(
    const char*     _label,
    eTextureFormat& _format,
    const bool      _bExcludeDepthFormat)
{
    const char* preview  = StringFrom(_format).data();
    bool        bChanged = false;

    if (ImGui::BeginCombo(_label, preview, ImGuiComboFlags_NoArrowButton))
    {
        for (const eTextureFormat format: EnumRanges<eTextureFormat>())
        {
            // unkown format and depth format are excluded
            if (_bExcludeDepthFormat && IsDepthFormat(format))
            {
                continue;
            }

            const bool  bSelected = (format == _format);
            const char* label     = StringFrom(format).data();

            if (ImGui::Selectable(label, bSelected))
            {
                _format  = format;
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

bool Widget_ImageFormatCombo(const char* _label, eImageFormat& _format)
{
    constexpr const char* const k_names[] = {
        "dds",
        "exr",
        "hdr",
        "tga",
        "bmp",
        "jpg",
        "jpeg",
        "png",
        "tif",
        "tiff",
        "gif",
        "hdp",
        "jxr",
        "wdp",
        "ico",
        "heif",
        "heic"
    };

    return Widget_EnumCombo(_label, reinterpret_cast<Int32&>(_format), k_names);
}

bool Widget_EnumCombo(
    const char*                        _label,
    Int32&                             _value,
    const std::span<const char* const> _labels)
{
    const char* preview  = _labels[_value];
    bool        bChanged = false;

    if (ImGui::BeginCombo(_label, preview))
    {
        for (Int32 i = 0; _labels[i] != nullptr; i++)
        {
            const bool  bSelected = (i == _value);
            const char* label     = _labels[i];

            if (ImGui::Selectable(label, bSelected))
            {
                _value   = i;
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

}   // namespace jam