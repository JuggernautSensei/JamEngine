#include "pch.h"

#include "EditorWidgets.h"

#include "AssetManager.h"
#include "Camera.h"
#include "WindowsUtilities.h"
#include "Editor.h"
#include "EditorUtilities.h"
#include "Entity.h"
#include "Graphics.h"
#include "ImGuiUtilities.h"
#include "Input.h"
#include "MessageDialog.h"
#include "ScriptDB.h"
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

void ImGuiCopyData(const ImGuiDataType _dataType, void* _dst, const void* _src)
{
    JAM_ASSERT(_dataType != ImGuiDataType_String, "Cannot copy string data type");
    const ImGuiDataTypeInfo& typeInfo = k_dataTypeInfoTable[_dataType];
    ::memcpy(_dst, _src, static_cast<size_t>(typeInfo.Size));
}

class InfiniteDragHelper
{
public:
    void Begin()
    {
        JAM_ASSERT(!m_bCapturing, "Already capturing mouse");

        m_bCapturing  = true;
        m_capturedPos = ImGui::GetMousePos();
        Input::SetMouseVisibility(false);
    }

    void Update() const
    {
        if (!m_bCapturing)
        {
            return;
        }

        const Int32 x = static_cast<Int32>(m_capturedPos.x);
        const Int32 y = static_cast<Int32>(m_capturedPos.y);
        Input::SetMousePos(x, y);
        ImGui::GetIO().MousePos = m_capturedPos;
    }

    void End()
    {
        if (!m_bCapturing)
        {
            return;
        }

        m_bCapturing = false;
        Input::SetMouseVisibility(true);
    }

    [[nodiscard]] ImVec2 GetDragDelta(
        const ImGuiMouseButton _button    = ImGuiMouseButton_Left,
        const float            _threshold = 0.5f) const
    {
        return ImGui::GetMouseDragDelta(_button, _threshold);
    }

    [[nodiscard]] bool IsCapturing() const noexcept
    {
        return m_bCapturing;
    }

private:
    ImVec2 m_capturedPos = ImVec2 { 0.f, 0.f };
    bool   m_bCapturing  = false;
};

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
    const ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
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
    const ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
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
    const ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
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
    const ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
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

void Widget_Image(
    const TextureHandle _texh,
    const ImVec2&       _size,
    const ImVec2&       _uv0,
    const ImVec2&       _uv1,
    const ImVec4&       _tintCol,
    const ImVec4&       _borderCol)
{
    ImGui::Image(
        ImGuiGlue::ImTextureRefFrom(_texh),
        _size,
        _uv0,
        _uv1,
        _tintCol,
        _borderCol);
}

void Widget_ImageWithBg(
    const TextureHandle _texh,
    const ImVec2&       _size,
    const ImVec2&       _uv0,
    const ImVec2&       _uv1,
    const ImVec4&       _bgCol,
    const ImVec4&       _tintCol)
{
    ImGui::ImageWithBg(
        ImGuiGlue::ImTextureRefFrom(_texh),
        _size,
        _uv0,
        _uv1,
        _bgCol,
        _tintCol);
}

bool Widget_ImageButton(
    const char*         _label,
    const TextureHandle _texh,
    const ImVec2&       _size,
    const ImVec2&       _uv0,
    const ImVec2&       _uv1,
    const ImVec4&       _bgCol,
    const ImVec4&       _tintCol)
{
    return ImGui::ImageButton(
        _label,
        ImGuiGlue::ImTextureRefFrom(_texh),
        _size,
        _uv0,
        _uv1,
        _bgCol,
        _tintCol);
}

void Widget_Image(
    const Ref<ImageTexture>& _pTex,
    const ImVec2&            _size,
    const ImVec2&            _uv0,
    const ImVec2&            _uv1,
    const ImVec4&            _tintCol,
    const ImVec4&            _borderCol)
{
    ImGui::Image(
        ImGuiGlue::ImTextureRefFrom(_pTex),
        _size,
        _uv0,
        _uv1,
        _tintCol,
        _borderCol);
}

void Widget_ImageWithBg(
    const Ref<ImageTexture>& _pTex,
    const ImVec2&            _size,
    const ImVec2&            _uv0,
    const ImVec2&            _uv1,
    const ImVec4&            _bgCol,
    const ImVec4&            _tintCol)
{
    ImGui::ImageWithBg(
        ImGuiGlue::ImTextureRefFrom(_pTex),
        _size,
        _uv0,
        _uv1,
        _bgCol,
        _tintCol);
}

bool Widget_ImageButton(
    const char*              _label,
    const Ref<ImageTexture>& _pTex,
    const ImVec2&            _size,
    const ImVec2&            _uv0,
    const ImVec2&            _uv1,
    const ImVec4&            _bgCol,
    const ImVec4&            _tintCol)
{
    return ImGui::ImageButton(
        _label,
        ImGuiGlue::ImTextureRefFrom(_pTex),
        _size,
        _uv0,
        _uv1,
        _bgCol,
        _tintCol);
}

void Widget_CenteredText(
    const char*  _text,
    const ImVec2 _region)
{
    const ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        return;
    }

    // det region
    ImVec2 region = _region;
    if (IsZeroApprox(region.x))
    {
        region.x = ImGui::GetContentRegionAvail().x;
    }

    if (IsZeroApprox(region.y))
    {
        region.y = ImGui::GetTextLineHeightWithSpacing();
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
    const ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        return;
    }

    const ImGuiStyle& style    = ImGui::GetStyle();
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

    const bool  bValidScript = _pScript != nullptr;
    const char* preview      = bValidScript ? _pScript->GetTypeName().GetPtr() : "none";
    bool        bChanged     = false;

    if (ImGui::BeginCombo(_label, preview, ImGuiComboFlags_NoArrowButton))
    {
        for (const ScriptInfo& scriptInfo: db.GetContainer() | std::views::values)
        {
            StringView  name      = scriptInfo.name;
            const char* label     = name.GetPtr();
            const bool  bSelected = bValidScript && (preview == name);

            if (ImGui::Selectable(label, bSelected))
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

bool Widget_MeshSelector(
    const char* _label,
    Ref<Mesh>&  _pMesh)
{
    ImGui::PushID(_label);
    bool bChanged = false;

    // text
    const char* text = _pMesh ? _pMesh->GetPath().c_str() : "no mesh";
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
            if (const Optional<Ref<Mesh>> optMesh = AssetManager::GetOrLoadMesh(path))
            {
                _pMesh   = optMesh.value();
                bChanged = true;
            }
            else
            {
                StringView msg = GlobalBuffer::FormatTmp("failed to load mesh from path: {}\nerror: {}", path.native(), GlobalBuffer::GetLastError());
                Editor::PushDialog(MakeScoped<MessageDialog>("error", msg));
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
            if (const Optional<Ref<Material>> optMat = AssetManager::GetOrLoadMaterial(path))
            {
                _pMaterial = optMat.value();
                bChanged   = true;
            }
            else
            {
                StringView msg = GlobalBuffer::FormatTmp("failed to load material from path: {}\nerror: {}", path.native(), GlobalBuffer::GetLastError());
                Editor::PushDialog(MakeScoped<MessageDialog>("error", msg));
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
        Widget_ImageWithBg(_pImageTexture, _size);
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
            if (const Optional<Ref<ImageTexture>> optTex = AssetManager::GetOrLoadImageTexture(path))
            {
                _pImageTexture = optTex.value();
                bChanged       = true;
            }
            else
            {
                StringView msg = GlobalBuffer::FormatTmp("failed to load image texture from path: {}\nerror: {}", path.native(), GlobalBuffer::GetLastError());
                Editor::PushDialog(MakeScoped<MessageDialog>("error", msg));
            }
        }

        ImGui::EndDragDropTarget();
    }

    // tooltip
    if (bTooltip && ImGui::BeginTooltip())
    {
        if (_pImageTexture)
        {
            constexpr const char* const k_names[] = { "texture 2D", "texture 3D", "texture cube" };

            const TextureInfo info = Graphics::GetTextureInfo(_pImageTexture->GetHandle());
            ImGui::Text("path: %s", _pImageTexture->GetPath().c_str());
            ImGui::Text("size: %d x %d x %d", info.width, info.height, info.depth);
            ImGui::Text("type: %s", k_names[IntFrom(info.type)]);
            ImGui::Text("format: %s", StringFrom(info.format).GetPtr());
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
        }

        ImGui::EndPopup();
    }

    ImGui::PopID();
    return bChanged;
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

    const ImGuiContext& g        = *GImGui;
    const ImGuiStyle&   style    = g.Style;
    const ImGuiID       id       = window->GetID(_label);
    const ImVec2        textSize = ImGui::CalcTextSize(_label, NULL, true);
    const ImVec2        iconSize = { ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight() };
    const float         defaultW = iconSize.x + textSize.x + style.ItemInnerSpacing.x + style.FramePadding.x * 2.0f;
    const float         defaultH = iconSize.y + style.FramePadding.y * 2.0f;
    const ImVec2        size     = ImGui::CalcItemSize(_size, defaultW, defaultH);

    const ImVec2 cursor = window->DC.CursorPos;
    const ImRect bb     = { cursor, cursor + size };
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
    {
        return false;
    }

    bool       hovered, held;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

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

bool Widget_DragScalar(
    const char*                    _label,
    const ImGuiDataType            _dataType,
    void*                          _pData,
    const float                    _vSpeed,
    const void*                    _pMin,
    const void*                    _pMax,
    const char*                    _format,
    const ImGuiSliderFlags         _flags,
    const eEditorSliderWidgetFlags _flags2)
{
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

    if (bChanged)
    {
        if (_flags2 & eEditorSliderWidgetFlags_ClampMin)
        {
            ImGuiCopyData(_dataType, _pData, _pMin);
        }

        if (_flags2 & eEditorSliderWidgetFlags_ClampMax)
        {
            ImGuiCopyData(_dataType, _pData, _pMax);
        }
    }

    return bChanged;
}

bool Widget_DragScalarN(
    const char*                    _label,
    const ImGuiDataType            _dataType,
    void*                          _pData,
    const int                      _numComps,
    const float                    _speed,
    const void*                    _pMin,
    const void*                    _pMax,
    const char*                    _format,
    const ImGuiSliderFlags         _flags,
    const eEditorSliderWidgetFlags _flags2)
{
    const ImGuiWindow* window = ImGui::GetCurrentWindow();
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

        bDirty |= Widget_DragScalar("", _dataType, _pData, _speed, _pMin, _pMax, _format, _flags, _flags2);
        _pData = static_cast<Byte*>(_pData) + typesize;
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
    const char*                    label,
    float*                         p_data,
    const float                    v_speed,
    const float                    v_min,
    const float                    v_max,
    const char*                    format,
    const ImGuiSliderFlags         flags,
    const eEditorSliderWidgetFlags _flags2)
{
    return Widget_DragScalar(
        label,
        ImGuiDataType_Float,
        p_data,
        v_speed,
        &v_min,
        &v_max,
        format,
        flags,
        _flags2);
}

bool Widget_DragFloat2(
    const char*                    label,
    float                          p_data[2],
    const float                    v_speed,
    const float                    v_min,
    const float                    v_max,
    const char*                    format,
    const ImGuiSliderFlags         flags,
    const eEditorSliderWidgetFlags _flags2)
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
        flags,
        _flags2);
}

bool Widget_DragFloat3(
    const char*                    label,
    float                          p_data[3],
    const float                    v_speed,
    const float                    v_min,
    const float                    v_max,
    const char*                    format,
    const ImGuiSliderFlags         flags,
    const eEditorSliderWidgetFlags _flags2)
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
        flags,
        _flags2);
}

bool Widget_DragFloat4(
    const char*                    label,
    float                          p_data[4],
    const float                    v_speed,
    const float                    v_min,
    const float                    v_max,
    const char*                    format,
    const ImGuiSliderFlags         flags,
    const eEditorSliderWidgetFlags _flags2)
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
        flags,
        _flags2);
}

bool Widget_SliderScalar(
    const char*                    label,
    const ImGuiDataType            _dataType,
    void*                          _pData,
    const void*                    _pMin,
    const void*                    _pMax,
    const char*                    _format,
    const ImGuiSliderFlags         _flags,
    const eEditorSliderWidgetFlags _flag2)
{
    static InfiniteDragHelper s_dragHelper;

    const bool    bChanged          = ImGui::SliderScalar(label, _dataType, _pData, _pMin, _pMax, _format, _flags);
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

    if (bChanged)
    {
        if (_flag2 & eEditorSliderWidgetFlags_ClampMin)
        {
            ImGuiCopyData(_dataType, _pData, _pMin);
        }

        if (_flag2 & eEditorSliderWidgetFlags_ClampMax)
        {
            ImGuiCopyData(_dataType, _pData, _pMax);
        }
    }

    return bChanged;
}

bool Widget_SliderScalarN(
    const char*                    _label,
    const ImGuiDataType            _dataType,
    void*                          _pData,
    const int                      _numComps,
    const void*                    _pMin,
    const void*                    _pMax,
    const char*                    _format,
    const ImGuiSliderFlags         _flags,
    const eEditorSliderWidgetFlags _flags2)
{
    const ImGuiWindow* window = ImGui::GetCurrentWindow();
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

        bDirty |= Widget_SliderScalar("", _dataType, _pData, _pMin, _pMax, _format, _flags, _flags2);
        _pData = static_cast<Byte*>(_pData) + typesize;
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

bool Widget_SliderFloat(
    const char*                    label,
    float*                         p_data,
    const float                    v_min,
    const float                    v_max,
    const char*                    format,
    const ImGuiSliderFlags         flags,
    const eEditorSliderWidgetFlags _flags2)
{
    return Widget_SliderScalar(
        label,
        ImGuiDataType_Float,
        p_data,
        &v_min,
        &v_max,
        format,
        flags,
        _flags2);
}

bool Widget_SliderFloat2(
    const char*                    label,
    float                          p_data[2],
    const float                    v_min,
    const float                    v_max,
    const char*                    format,
    const ImGuiSliderFlags         flags,
    const eEditorSliderWidgetFlags _flags2)
{
    return Widget_SliderScalarN(
        label,
        ImGuiDataType_Float,
        p_data,
        2,
        &v_min,
        &v_max,
        format,
        flags,
        _flags2);
}

bool Widget_SliderFloat3(
    const char*                    label,
    float                          p_data[3],
    const float                    v_min,
    const float                    v_max,
    const char*                    format,
    const ImGuiSliderFlags         flags,
    const eEditorSliderWidgetFlags _flags2)
{
    return Widget_SliderScalarN(
        label,
        ImGuiDataType_Float,
        p_data,
        3,
        &v_min,
        &v_max,
        format,
        flags,
        _flags2);
}

bool Widget_SliderFloat4(
    const char*                    label,
    float                          p_data[4],
    const float                    v_min,
    const float                    v_max,
    const char*                    format,
    const ImGuiSliderFlags         flags,
    const eEditorSliderWidgetFlags _flags2)
{
    return Widget_SliderScalarN(
        label,
        ImGuiDataType_Float,
        p_data,
        4,
        &v_min,
        &v_max,
        format,
        flags,
        _flags2);
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

    const ImGuiContext& g     = *GImGui;
    const ImGuiStyle&   style = g.Style;
    const ImGuiID       id    = pWin->GetID(_label);

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
    static InfiniteDragHelper s_dragHelper;
    static float              s_pitchRad = 0.f;
    static float              s_yawRad   = 0.f;

    if (bPressed)
    {
        // capture current rotation
        const auto [p, y, _] = _transform.ToPitchYawRoll();
        s_pitchRad           = p;
        s_yawRad             = y;

        // freeze cursor
        s_dragHelper.Begin();
    }

    if (bReleased)
    {
        s_dragHelper.End();
    }

    if (bHold)
    {
        s_dragHelper.Update();
        const auto [dx, dy] = s_dragHelper.GetDragDelta();

        if (!(IsZeroApprox(dx) && IsZeroApprox(dy)))
        {
            constexpr float k_sensitivity = 0.005f;

            // apply rotation
            s_yawRad += dx * k_sensitivity;
            s_yawRad = Wrap(s_yawRad, -k_2pi, k_2pi);
            s_pitchRad += dy * k_sensitivity;
            s_pitchRad = Clamp(s_pitchRad, ToRad(-89.f), ToRad(89.f));
            _transform.SetRotation(s_pitchRad, s_yawRad, 0.f);

            // dirty check
            bChanged = true;
        }
    }

    // render
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();

    // background circle
    {
        const eEditorStyleColor col = bHold    ? eEditorStyleColor_AcroballBgActive
                                    : bHovered ? eEditorStyleColor_AcroballBgHovered
                                               : eEditorStyleColor_AcroballBg;

        const ImU32 bgCol = Editor::GetStyleColorU32(col);
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
        static bool CompareDepthAscending(
            const AxisData& a,
            const AxisData& b)
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

        // draw line + circle at the end
        pDrawList->AddLine(center, endPos, colorU32, k_lineThickness);
        pDrawList->AddCircleFilled(endPos, k_circleRadius, colorU32, 20);

        // draw outline
        constexpr float k_t1         = 1.2f;
        const ImVec4    outlineColor = { finalColor.x * k_t1, finalColor.y * k_t1, finalColor.z * k_t1, finalColor.w };
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

    return bChanged;
}

bool Widget_SingleFileDialog(
    const char*                             _label,
    fs::path&                               _path,
    const eSingleFileDialogType             _type,
    const StringView                        _title,
    const std::span<const FileDialogFilter> _filtersOrNull)
{
    const ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        return false;
    }

    const ImGuiStyle& style      = ImGui::GetStyle();
    const float       width      = ImGui::CalcItemWidth();
    const ImVec2      btnSize    = ImGui::CalcTextSize("open") + style.FramePadding * 2;
    const float       inputItemW = Max(0.f, width - btnSize.x - style.ItemSpacing.x);
    bool              bDirty     = false;

    // path preview
    ImGui::SetNextItemWidth(inputItemW);
    ImGui::InputText(_label, const_cast<std::string*>(&_path.native()), ImGuiInputTextFlags_ReadOnly);

    // tooltip (full path)
    if (ImGui::BeginItemTooltip())
    {
        ImGui::TextUnformatted(_path.empty() ? "no file selected" : _path.c_str());
        ImGui::EndTooltip();
    }

    // open button & logic
    ImGui::SameLine();
    if (ImGui::Button("open", btnSize))
    {
        eFileDialogMode mode;
        switch (_type)
        {
            case eSingleFileDialogType::OpenFile:
                mode = eFileDialogMode::OpenFile;
                break;
            case eSingleFileDialogType::SaveFile:
                mode = eFileDialogMode::SaveFile;
                break;
            case eSingleFileDialogType::SelectFolder:
                mode = eFileDialogMode::SelectFolder;
                break;
            default:
                JAM_ASSERT(0, "unknown single file dialog type");
                mode = eFileDialogMode::OpenFile;
                break;
        }

        // create file dialog
        FileDialog fd { mode };
        fd.SetTitle(_title);
        if (!_filtersOrNull.empty())
        {
            fd.SetFilters(_filtersOrNull);
        }

        // succeeded
        if (fd.Open())
        {
            _path = fd.GetFirstSelectedPath();
        }
    }

    return bDirty;
}

bool Widget_Combo(const char* _label, Int32& _curItem, const std::span<const char* const> _items)
{
    const Int32 numItems = NarrowCast<Int32>(_items.size());
    const char* preview  = _curItem >= 0 && _curItem < numItems ? _items[_curItem] : "none";
    bool        bChanged = false;

    if (ImGui ::BeginCombo(_label, preview, ImGuiComboFlags_NoArrowButton))
    {
        for (Int32 i = 0; i < numItems; ++i)
        {
            const bool bSelected = (i == _curItem);
            if (ImGui::Selectable(_items[i], bSelected))
            {
                _curItem = i;
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

bool Widget_TextureChannelCombo(
    const char*      _label,
    eTextureChannel& _colorChannel,
    const bool       _bExcludeAlphaChannel)
{
    if (_bExcludeAlphaChannel)
    {
        const char* k_names[] = { "R", "G", "B" };
        return Widget_Combo(_label, reinterpret_cast<Int32&>(_colorChannel), k_names);
    }
    else
    {
        const char* k_names[] = { "R", "G", "B", "A" };
        return Widget_Combo(_label, reinterpret_cast<Int32&>(_colorChannel), k_names);
    }
}

bool Widget_TextureFormatCombo(
    const char*     _label,
    eTextureFormat& _format,
    const bool      _bExcludeDepthFormat)
{
    const StringView preview  = StringFrom(_format);
    bool             bChanged = false;

    if (ImGui::BeginCombo(_label, preview.GetPtr(), ImGuiComboFlags_NoArrowButton))
    {
        for (const eTextureFormat format: EnumRanges<eTextureFormat>())
        {
            // unkown format and depth format are excluded
            if (_bExcludeDepthFormat && IsDepthFormat(format))
            {
                continue;
            }

            const bool       bSelected = (format == _format);
            const StringView label     = StringFrom(format);

            if (ImGui::Selectable(label.GetPtr(), bSelected))
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
    constexpr const char* k_names[] = {
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

    return Widget_Combo(_label, reinterpret_cast<Int32&>(_format), k_names);
}

}   // namespace jam