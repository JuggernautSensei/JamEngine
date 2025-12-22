#pragma once
#include "Camera.h"
#include "DialogUtilities.h"
#include "Entity.h"
#include "GraphicsBase.h"
#include "ImageBase.h"
#include "ImageTexture.h"
#include "Material.h"
#include "Mesh.h"
#include "Transform.h"
#include "UID.h"
#include "VertexLayoutStorage.h"

namespace jam
{

void Widget_CenteredText(ImVec2 _region, const char* _fmt, ...);
void Widget_CenteredTextV(ImVec2 _region, const char* _fmt, va_list _args);

void Widget_FrameAlignedText(const char* _fmt, ...);
void Widget_FrameAlignedTextV(const char* _fmt, va_list _args);

void Widget_BorderedText(const char* _text, const ImVec2& _size = ImVec2 { 0.f, 0.f }, bool _bDisabled = false);
void Widget_BorderedTextColored(const ImVec4& _color, const char* _text, const ImVec2& _size = ImVec2 { 0.f, 0.f });

bool Widget_DragTranslation(const char* _label, Vec3* _translation, float _speed = 0.01f, float _min = -FLT_MAX, float _max = FLT_MAX);
bool Widget_DragScale(const char* _label, Vec3& _scaleNorm, float _speedPerc = 0.5f, float _minPerc = -FLT_MAX, float _maxPerc = FLT_MAX);
bool Widget_DragScalePerc(const char* _label, Vec3& _scalePerc, float _speedPerc = 0.5f, float _minPerc = -FLT_MAX, float _maxPerc = FLT_MAX);
bool Widget_DragPitchYawRoll(const char* _label, Vec3& _pitchYawRollRad, float _speedDeg = 0.5f, float _minDeg = -359.f, float _maxDeg = 359.f);
bool Widget_DragPitchYaw(const char* _label, Vec2& _pitchYawRad, float _speedDeg = 0.5f, float _minDeg = -359.f, float _maxDeg = 359.f);
bool Widget_DragPitchYawRollDeg(const char* _label, Vec3& _pitchYawRollDeg, float _speedDeg = 0.5f, float _minDeg = -359.f, float _maxDeg = 359.f);
bool Widget_DragPitchYawDeg(const char* _label, Vec2& _pitchYawDeg, float _speedDeg = 0.5f, float _minDeg = -359.f, float _maxDeg = 359.f);
bool Widget_DragAngle(const char* _label, float& _pRad, float _degSpeed = 0.5f, float _minDeg = -359.f, float _maxDeg = 359.f);
bool Widget_DragPercent(const char* _label, float& _pNorm, float _speedPerc = 0.5f, float _minPerc = 0.f, float _maxPerc = 100.f, const char* _format = "%.0f %%");

void Widget_ShadowedText(const char* _fmt, ...);
void Widget_ShadowedTextV(const char* _fmt, va_list _args);

void Widget_HelpMark(const char* _desc);

bool Widget_VertexLayoutEntryCombo(const char* _label, Ref<VertexLayoutEntry>& _pVle);
bool Widget_ProjectionEnumCombo(const char* _label, eProjection& _projection);

bool Widget_MeshSelector(const char* _label, Ref<Mesh>& _pMesh);
bool Widget_MaterialSelector(const char* _label, Ref<Material>& _pMaterial);
bool Widget_ImageTextureSelector(const char* _label, Ref<ImageTexture>& _pImageTexture, const ImVec2& _size = ImVec2 { 128.f, 128.f });

bool Widget_DragScalar(const char* label, ImGuiDataType data_type, void* p_data, float v_speed, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags);
bool Widget_DragScalarN(const char* _label, ImGuiDataType _dataType, void* _pData, int _numComps, float _speed, const void* _pMin, const void* _pMax, const char* _format, ImGuiSliderFlags _flags);

bool Widget_DragFloat(const char* label, float* p_data, float v_speed = 1.f, float v_min = 0.f, float v_max = 0.f, const char* format = "%.3f", ImGuiSliderFlags flags);
bool Widget_DragFloat2(const char* label, float p_data[2], float v_speed = 1.f, float v_min = 0.f, float v_max = 0.f, const char* format = "%.3f", ImGuiSliderFlags flags);
bool Widget_DragFloat3(const char* label, float p_data[3], float v_speed = 1.f, float v_min = 0.f, float v_max = 0.f, const char* format = "%.3f", ImGuiSliderFlags flags);
bool Widget_DragFloat4(const char* label, float p_data[4], float v_speed = 1.f, float v_min = 0.f, float v_max = 0.f, const char* format = "%.3f", ImGuiSliderFlags flags);

bool Widget_DragDistance(const char* label, float& _distance, float _speed = 0.01f, float _min = 0.f, float _max = 10000.f, const char* _format = "%.2fm", ImGuiSliderFlags _flags = ImGuiSliderFlags_AlwaysClamp);
bool Widget_DragDistance2(const char* label, Vec2& _distanceVec2, float _speed = 0.01f, float _min = 0.f, float _max = 10000.f, const char* _format = "%.2fm", ImGuiSliderFlags _flags = ImGuiSliderFlags_AlwaysClamp);
bool Widget_DragDistance3(const char* label, Vec3& _distanceVec3, float _speed = 0.01f, float _min = 0.f, float _max = 10000.f, const char* _format = "%.2fm", ImGuiSliderFlags _flags = ImGuiSliderFlags_AlwaysClamp);

bool Widget_DragLightIntensity(const char* _label, float& _intensity, float _speed = 0.01f, float _min = 0.f, float _max = 10.f, const char* _format = "%.2f", ImGuiSliderFlags _flags = ImGuiSliderFlags_AlwaysClamp);

bool Widget_IconButton(const char* _label, ImTextureRef _icon, ImVec2 _size = { 0, 0 });
bool Widget_Acroball(const char* _label, Transform& _transform, ImVec2 _pos, float _length);

bool Widget_OpenFileDialog(const char* _label, fs::path& _path, std::span<const FileDialogFilter> _filters = {});
bool Widget_TextureChannelCombo(const char* _label, eTextureChannel& _colorChannel, bool _bExcludeAlphaChannel = false);
bool Widget_TextureFormatCombo(const char* _label, eTextureFormat& _format, bool _bExcludeDepthFormat = false);
bool Widget_ImageFormatCombo(const char* _label, eImageFormat& _format);

bool Widget_EnumCombo(const char* _label, Int32& _value, std::span<const char* const> _labels);

}   // namespace jam