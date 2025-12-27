#pragma once
#include "Camera.h"
#include "WindowsUtilities.h"
#include "Entity.h"
#include "GraphicsBase.h"
#include "ImageBase.h"
#include "ImageTexture.h"
#include "Material.h"
#include "Mesh.h"
#include "Transform.h"
#include "VertexLayoutStorage.h"

namespace jam
{

class Script;

enum class eSingleFileDialogType
{
    OpenFile,
    SelectFolder,
    SaveFile,
};

enum eEditorSliderWidgetFlags_
{
    eEditorSliderWidgetFlags_None     = 0,
    eEditorSliderWidgetFlags_ClampMin = 1 << 1,
    eEditorSliderWidgetFlags_ClampMax = 1 << 2,

    eEditorSliderWidgetFlags_ClampBoth = eEditorSliderWidgetFlags_ClampMin | eEditorSliderWidgetFlags_ClampMax,
};

using eEditorSliderWidgetFlags = EnumIntType<eEditorSliderWidgetFlags_>;

// base
void Widget_CenteredText(const char* _text, ImVec2 _region = ImVec2 { 0.f, 0.f });
void Widget_FrameAlignedText(const char* _text);
void Widget_BorderedText(const char* _text, ImVec2 _size = ImVec2 { 0.f, 0.f }, bool _bDisabled = false);

bool Widget_IconButton(const char* _label, ImTextureRef _icon, ImVec2 _size = { 0, 0 });

bool Widget_DragScalar(const char* _label, ImGuiDataType _dataType, void* _pData, float _vSpeed, const void* _pMin, const void* _pMax, const char* _format, ImGuiSliderFlags _flags, eEditorSliderWidgetFlags _flags2);
bool Widget_DragScalarN(const char* _label, ImGuiDataType _dataType, void* _pData, int _numComps, float _speed, const void* _pMin, const void* _pMax, const char* _format, ImGuiSliderFlags _flags, eEditorSliderWidgetFlags _flags2);
bool Widget_DragFloat(const char* label, float* p_data, float v_speed = 1.f, float v_min = 0.f, float v_max = 0.f, const char* format = "%.3f", ImGuiSliderFlags flags = 0, eEditorSliderWidgetFlags _flags2 = 0);
bool Widget_DragFloat2(const char* label, float p_data[2], float v_speed = 1.f, float v_min = 0.f, float v_max = 0.f, const char* format = "%.3f", ImGuiSliderFlags flags = 0, eEditorSliderWidgetFlags _flags2 = 0);
bool Widget_DragFloat3(const char* label, float p_data[3], float v_speed = 1.f, float v_min = 0.f, float v_max = 0.f, const char* format = "%.3f", ImGuiSliderFlags flags = 0, eEditorSliderWidgetFlags _flags2 = 0);
bool Widget_DragFloat4(const char* label, float p_data[4], float v_speed = 1.f, float v_min = 0.f, float v_max = 0.f, const char* format = "%.3f", ImGuiSliderFlags flags = 0, eEditorSliderWidgetFlags _flags2 = 0);

bool Widget_SliderScalar(const char* label, ImGuiDataType _dataType, void* _pData, const void* _pMin, const void* _pMax, const char* _format, ImGuiSliderFlags _flags, eEditorSliderWidgetFlags _flag2);
bool Widget_SliderScalarN(const char* label, ImGuiDataType _dataType, void* _pData, int _numComps, const void* _pMin, const void* _pMax, const char* _format, ImGuiSliderFlags _flags, eEditorSliderWidgetFlags _flags2);
bool Widget_SliderFloat(const char* label, float* p_data, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0, eEditorSliderWidgetFlags _flags2 = 0);
bool Widget_SliderFloat2(const char* label, float p_data[2], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0, eEditorSliderWidgetFlags _flags2 = 0);
bool Widget_SliderFloat3(const char* label, float p_data[3], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0, eEditorSliderWidgetFlags _flags2 = 0);
bool Widget_SliderFloat4(const char* label, float p_data[4], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0, eEditorSliderWidgetFlags _flags2 = 0);

void Widget_HelpMark(const char* _desc);

bool Widget_DragTranslation(const char* _label, Vec3& _translation, float _speed = 0.01f, float _min = -FLT_MAX, float _max = FLT_MAX, const char* _fmt = "%.3fm", ImGuiSliderFlags _flag = 0);
bool Widget_DragScale(const char* _label, Vec3& _scaleNorm, float _speedPerc = 1.f, float _minPerc = 0.f, float _maxPerc = FLT_MAX, const char* _fmt = "%.0fdeg", ImGuiSliderFlags _flag = ImGuiSliderFlags_AlwaysClamp);
bool Widget_DragPitchYawRoll(const char* _label, Vec3& _pitchYawRollRad, float _speedDeg = 1.f, float _minDeg = -359.f, float _maxDeg = 359.f, const char* _fmt = "%.0fdeg", ImGuiSliderFlags _flag = ImGuiSliderFlags_WrapAround);
bool Widget_DragPitchYaw(const char* _label, Vec2& _pitchYawRad, float _speedDeg = 1.f, float _minDeg = -359.f, float _maxDeg = 359.f, const char* _fmt = "%.0fdeg", ImGuiSliderFlags _flag = ImGuiSliderFlags_WrapAround);
bool Widget_DragAngle(const char* _label, float& _pRad, float _speedDeg = 1.f, float _minDeg = -359.f, float _maxDeg = 359.f, const char* _fmt = "%.0fdeg", ImGuiSliderFlags _flag = ImGuiSliderFlags_WrapAround);
bool Widget_DragPercent(const char* _label, float& _pNorm, float _speedPerc = 1.f, float _minPerc = 0.f, float _maxPerc = 100.f, const char* _format = "%.0f%%", ImGuiSliderFlags _flag = 0);

void Widget_Image(TextureHandle _texh, const ImVec2& _size, const ImVec2& _uv0 = ImVec2 { 0.f, 0.f }, const ImVec2& _uv1 = ImVec2 { 1.f, 1.f }, const ImVec4& _tintCol = ImVec4 { 1.f, 1.f, 1.f, 1.f }, const ImVec4& _borderCol = ImVec4 { 0.f, 0.f, 0.f, 0.f });
void Widget_ImageWithBg(TextureHandle _texh, const ImVec2& _size, const ImVec2& _uv0 = ImVec2 { 0.f, 0.f }, const ImVec2& _uv1 = ImVec2 { 1.f, 1.f }, const ImVec4& _bgCol = ImVec4 { 0.f, 0.f, 0.f, 0.f }, const ImVec4& _tintCol = ImVec4 { 1.f, 1.f, 1.f, 1.f });
bool Widget_ImageButton(const char* _label, TextureHandle _texh, const ImVec2& _size, const ImVec2& _uv0 = ImVec2 { 0.f, 0.f }, const ImVec2& _uv1 = ImVec2 { 1.f, 1.f }, const ImVec4& _bgCol = ImVec4 { 1.f, 1.f, 1.f, 1.f }, const ImVec4& _tintCol = ImVec4 { 0.f, 0.f, 0.f, 0.f });

void Widget_Image(const Ref<ImageTexture>& _pTex, const ImVec2& _size, const ImVec2& _uv0 = ImVec2 { 0.f, 0.f }, const ImVec2& _uv1 = ImVec2 { 1.f, 1.f }, const ImVec4& _tintCol = ImVec4 { 1.f, 1.f, 1.f, 1.f }, const ImVec4& _borderCol = ImVec4 { 0.f, 0.f, 0.f, 0.f });
void Widget_ImageWithBg(const Ref<ImageTexture>& _pTex, const ImVec2& _size, const ImVec2& _uv0 = ImVec2 { 0.f, 0.f }, const ImVec2& _uv1 = ImVec2 { 1.f, 1.f }, const ImVec4& _bgCol = ImVec4 { 0.f, 0.f, 0.f, 0.f }, const ImVec4& _tintCol = ImVec4 { 1.f, 1.f, 1.f, 1.f });
bool Widget_ImageButton(const char* _label, const Ref<ImageTexture>& _pTex, const ImVec2& _size, const ImVec2& _uv0 = ImVec2 { 0.f, 0.f }, const ImVec2& _uv1 = ImVec2 { 1.f, 1.f }, const ImVec4& _bgCol = ImVec4 { 1.f, 1.f, 1.f, 1.f }, const ImVec4& _tintCol = ImVec4 { 0.f, 0.f, 0.f, 0.f });

// sugar
bool Widget_VertexLayoutEntryCombo(const char* _label, Ref<VertexLayoutEntry>& _pVle);
bool Widget_ScriptCombo(const char* _label, Ref<Script>& _pScript, Entity _owner);   // owner for creating script instance
bool Widget_ProjectionCombo(const char* _label, eProjection& _projection);
bool Widget_ImageFormatCombo(const char* _label, eImageFormat& _format);
bool Widget_TextureChannelCombo(const char* _label, eTextureChannel& _colorChannel, bool _bExcludeAlphaChannel = false);
bool Widget_TextureFormatCombo(const char* _label, eTextureFormat& _format, bool _bExcludeDepthFormat = false);

bool Widget_MeshSelector(const char* _label, Ref<Mesh>& _pMesh);
bool Widget_MaterialSelector(const char* _label, Ref<Material>& _pMaterial);
bool Widget_ImageTextureSelector(const char* _label, Ref<ImageTexture>& _pImageTexture, const ImVec2& _size = ImVec2 { 128.f, 128.f });

bool Widget_Acroball(const char* _label, Transform& _transform, ImVec2 _pos, float _length);
bool Widget_SingleFileDialog(const char* _label, fs::path& _path, eSingleFileDialogType _type, StringView _title, std::span<const FileDialogFilter> _filtersOrNull);
bool Widget_Combo(const char* _label, Int32& _currentItem, std::span<const char* const> _items);

}   // namespace jam