#pragma once
#include "WindowsUtilities.h"
#include "ImGuiEngine.h"
#include "Entity.h"
#include "GraphicsBase.h"
#include "Texture.h"
#include "Material.h"
#include "Mesh.h"

namespace Jam
{
class FileDialog;

class VertexLayoutEntry;

// helper for infinite drag behavior
// use like this:
// ...
// static WidgetInfiniteDragger dragger = {};
// ImGui::DragXXX(...); // normal drag widget
// dragger.OnAfterWidget(); // next line after drag widget
// ...
// this must use with imgui widget
// you cannot use it generally.
// you can use InfiniteDragger in "InfiniteDragg.h" for general use.
class WidgetInfiniteDragger
{
public:
    void OnAfterWidget();

    [[nodiscard]] ImVec2 GetDragDelta(ImGuiMouseButton _button = ImGuiMouseButton_Left, float _threshold = 0.5f) const;

private:
    ImVec2 m_capturedPos = { 0.f, 0.f };

    inline static bool s_bSharedBitForDebug = false;
};

struct ImTextureProxy
{
    ImTextureProxy();
    /* implicit */ ImTextureProxy(const Ref<Texture>& _pTexture);
    /* implicit */ ImTextureProxy(const ImTextureRef& _texture);
    /* implicit */ ImTextureProxy(TextureHandle _texh);

    ImTextureRef texture = ImTextureID_Invalid;
};

// helper to store euler angles properly when editing
// to avoid gimbal lock and sudden jumps
// use like this:
// ...
// static EulerProxy eulerProxy = {};
// eulerProxy.OnBeforeWidget(transform);
// Vector3& euler = eulerProxy.GetEulerProxy();
// if(Widget_DragEuler("Rotation", euler, ...))
// {
//      transform.SetRotation(euler);
// }
// eulerProxy.OnAfterWidget();
// ...
class EulerProxy
{
public:
    void OnBeforeWidget(const Vector3& _forward);
    void OnBeforeWidget(const Matrix4x4& _mtx);
    void OnBeforeWidget(const Quaternion& _quat);
    void OnBeforeWidget(const Transform& _t);
    void OnAfterWidget();

    [[nodiscard]] Vector3& GetEulerProxy();

private:
    Vector3 m_eulerRad   = Vec3::k_zero;
    bool    m_bIsEditing = false;
};

enum class eTextAlign
{
    Left,
    Center,
    Right
};

// base
void Widget_Text(const char* _text, bool _bFrameAligned = false);
void Widget_TextFrame(const char* _text, ImVec2 _frameSize = { 0.f, 0.f }, ImVec4 _bgCol = { 0.f, 0.f, 0.f, 0.f }, bool _bRenderBorder = false, eTextAlign _align = eTextAlign::Center);
bool Widget_IconButton(const char* _text, ImTextureProxy _icon, ImVec2 _size = { 0.f, 0.f });
void Widget_TextWrapped(const char* _text, float _wrapWidth);

bool Widget_DragScalar(const char* _label, ImGuiDataType _dataType, void* _pData, float _vSpeed, const void* _pMin, const void* _pMax, const char* _format, ImGuiSliderFlags _flags);
bool Widget_DragScalarN(const char* _label, ImGuiDataType _dataType, void* _pData, int _numComps, float _speed, const void* _pMin, const void* _pMax, const char* _format, ImGuiSliderFlags _flags);
bool Widget_DragFloat(const char* label, float& p_data, float v_speed = 0.01f, float v_min = 0.f, float v_max = 0.f, const char* format = "%.3f", ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp);
bool Widget_DragFloat2(const char* label, float p_data[2], float v_speed = 0.01f, float v_min = 0.f, float v_max = 0.f, const char* format = "%.3f", ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp);
bool Widget_DragFloat3(const char* label, float p_data[3], float v_speed = 0.01f, float v_min = 0.f, float v_max = 0.f, const char* format = "%.3f", ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp);
bool Widget_DragFloat4(const char* label, float p_data[4], float v_speed = 0.01f, float v_min = 0.f, float v_max = 0.f, const char* format = "%.3f", ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp);
bool Widget_DragInt(const char* label, int& p_data, float v_speed = 0.05f, int v_min = 0, int v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp);
bool Widget_DragInt2(const char* label, int p_data[2], float v_speed = 0.05f, int v_min = 0, int v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp);

void Widget_HelpMark(const char* _desc);
bool Widget_MenuItem(const char* _label, ImGuiKeyChord _shortcut = 0, bool _bSelected = false, bool _bEnabled = true);

bool Widget_DragTranslation(const char* _label, Vector3& _translation, float _speed = 0.01f, float _min = 0.f, float _max = 0.f, const char* _fmt = "%.3fm", ImGuiSliderFlags _flag = ImGuiSliderFlags_AlwaysClamp);
bool Widget_DragScale(const char* _label, Vector3& _scaleNorm, float _speedPerc = 1.f, float _minPerc = 0.001f, float _maxPerc = Limits<float>::k_max, const char* _fmt = "%.1f%", ImGuiSliderFlags _flag = ImGuiSliderFlags_AlwaysClamp);
bool Widget_DragPercent(const char* _label, float& _pNorm, float _speedPerc = 1.f, float _minPerc = 0.001f, float _maxPerc = Limits<float>::k_max, const char* _format = "%.1f%%", ImGuiSliderFlags _flag = ImGuiSliderFlags_AlwaysClamp);
bool Widget_DragAngle(const char* _label, float& _pRad, float _speedDeg = 1.f, float _minDeg = -359.f, float _maxDeg = 359.f, const char* _fmt = "%.1fdeg", ImGuiSliderFlags _flag = ImGuiSliderFlags_WrapAround);
bool Widget_DragEuler(const char* _label, Vector3& _eulerRad, float _speedDeg = 1.f, float _minDeg = -359.f, float _maxDeg = 359.f, const char* _fmt = "%.1fdeg", ImGuiSliderFlags _flag = ImGuiSliderFlags_WrapAround);

void Widget_ImageFrame(ImTextureProxy _texture, ImVec2 _texSize, ImVec2 _frameSize, ImVec2 _uv0 = { 0.f, 0.f }, ImVec2 _uv1 = { 1.f, 1.f }, ImVec4 _bgCol = { 0.f, 0.f, 0.f, 0.f }, bool _bBorder = false);
void Widget_Image(ImTextureProxy _texture, ImVec2 _size, ImVec2 _uv0 = { 0.f, 0.f }, ImVec2 _uv1 = { 1.f, 1.f });
bool Widget_ImageButton(const char* _label, ImTextureProxy _texture, ImVec2 _size, ImVec2 _uv0 = { 0.f, 0.f }, ImVec2 _uv1 = { 1.f, 1.f }, ImVec4 _bgCol = { 0.f, 0.f, 0.f, 0.f }, ImVec4 _tintCol = { 1.f, 1.f, 1.f, 1.f });
void Widget_ItemDescriptionTooltip(const char* _summery, const char* _desc, ImGuiKeyChord _shortcut = 0);

bool Widget_VertexLayoutEntryCombo(const char* _label, Ref<VertexLayoutEntry>& _pVle);
void Widget_Dummy(const char* _label, ImVec2 _size);

// asset
bool Widget_MeshPicker(const char* _label, Ref<Mesh>& _pMesh);
bool Widget_MaterialPicker(const char* _label, Ref<Material>& _pMaterial);
bool Widget_TexturePicker(const char* _label, Ref<Texture>& _pTexture, const ImVec2& _size = ImVec2 { 128.f, 128.f });

bool Widget_Acroball(const char* _label, Transform& _t, ImVec2 _pos, float _length);
bool Widget_FileDialog(const char* _label, String& _path, FileDialog& _fd);
bool Widget_InputText(const char* _label, String& _str, ImGuiInputTextFlags _flags = 0, ImGuiInputTextCallback _callback = nullptr, void* _pUserData = nullptr);
bool Widget_InputTextMultiline(const char* _label, String& _str, ImVec2 _size = { 0.f, 0.f }, ImGuiInputTextFlags _flags = 0, ImGuiInputTextCallback _callback = nullptr, void* _pUserData = nullptr);
void Widget_ReadOnlyInputText(const char* _label, StringView _str);
void Widget_ReadOnlyInputTextMultiline(const char* _label, StringView _str, ImVec2 _size);

struct EnumWidgetItem
{
    const char* name  = nullptr;
    Int32       value = 0;
};

namespace Details   // enum combo helper
{
    template<typename TEnum>
        requires std::is_enum_v<TEnum>
    constexpr auto BuildEnumWidgetItem()
    {
        EnumArray<TEnum, EnumWidgetItem> items = {};
        for (const TEnum& value: EnumRanges<TEnum>())
        {
            items[value] = { NameOf(value).GetPtr(), NumericCast<Int32>(value) };
        }
        return items;
    }

    template<typename TEnum>
        requires std::is_enum_v<TEnum>
    constexpr auto k_enumWidgetItems = BuildEnumWidgetItem<TEnum>();

}   // namespace Details

bool Widget_EnumCombo(const char* _label, EnumWidgetItem& _curItem, Span<const EnumWidgetItem> _items);
bool Widget_EnumToggle(const char* _label, EnumWidgetItem& _curItem, Span<const EnumWidgetItem> _items);
bool Widget_EnumFlags(const char* _label, Int32& _flags, Span<const EnumWidgetItem> _flagsItems, Span<const EnumWidgetItem> _setItems);

template<typename TEnum>
    requires(std::is_enum_v<TEnum> && !std::is_const_v<TEnum>)
bool Widget_EnumCombo(const char* _label, TEnum& _curValue)
{
    EnumWidgetItem curItem = { NameOf(_curValue).GetPtr(), NumericCast<Int32>(UnderlyingFrom(_curValue)) };
    if (Widget_EnumCombo(_label, curItem, Details::k_enumWidgetItems<TEnum>))
    {
        _curValue = EnumCast<TEnum>(curItem.value).GetValue();
        return true;
    }
    return false;
}

template<typename TEnum>
    requires(std::is_enum_v<TEnum> && !std::is_const_v<TEnum>)
bool Widget_EnumToggle(const char* _label, TEnum& _curValue)
{
    EnumWidgetItem curItem = { NameOf(_curValue).GetPtr(), NumericCast<Int32>(_curValue) };
    if (Widget_EnumToggle(_label, curItem, Details::k_enumWidgetItems<TEnum>))
    {
        _curValue = EnumCast<TEnum>(curItem.value).GetValue();
        return true;
    }
    return false;
}

}   // namespace Jam
