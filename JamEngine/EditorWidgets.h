#pragma once
#include "Camera.h"
#include "WindowsUtilities.h"
#include "Entity.h"
#include "GraphicsBase.h"
#include "ImageTexture.h"
#include "Input.h"
#include "Material.h"
#include "Mesh.h"
#include "Transform.h"
#include "ImGuiUtilities.h"

namespace jam
{

class Script;
class VertexLayoutEntry;

class InfiniteDragHelper
{
public:
    void Begin()
    {
        JAM_ASSERT(!m_bCapturing, "Already capturing mouse");

        m_bCapturing  = true;
        m_capturedPos = ImGui::GetMousePos();
        Input::SetMouseVisibility(false);

        if (JAM_DEBUG_ENABLED)
        {
            JAM_ASSERT(!s_bSharedBitForDebug, "Another InfiniteDragHelper is already capturing mouse. Only one can capture at a time.");
            s_bSharedBitForDebug = true;
        }
    }

    void Update() const
    {
        if (!m_bCapturing)
        {
            return;
        }

        // current pos
        const auto [curX, curY] = Input::GetMousePos();

        // os mouse move to captured pos
        const Int32 capturedX = static_cast<Int32>(m_capturedPos.x);
        const Int32 capturedY = static_cast<Int32>(m_capturedPos.y);
        Input::SetMousePos(capturedX, capturedY);

        // for imgui use
        ImGuiIO& io     = ImGui::GetIO();
        io.MousePosPrev = m_capturedPos;
        io.MouseDelta   = io.MousePos - io.MousePosPrev;
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
    ImVec2 m_capturedPos = { 0.f, 0.f };
    ImVec2 m_deltaPos    = { 0.f, 0.f };
    bool   m_bCapturing  = false;

    inline static bool s_bSharedBitForDebug = false;
};

struct ImTextureProxy
{
    ImTextureProxy() = default;
    /* implicit */ ImTextureProxy(const Ref<ImageTexture>& _pTexture)
        : texture(ImGuiGlue::ImTextureRefFrom(_pTexture))
    {
        JAM_ASSERT(_pTexture, "Invalid texture passed to ImTextureProxy");
        JAM_ASSERT(_pTexture->IsValid(), "Invalid texture passed to ImTextureProxy");
    }

    /* implicit */ ImTextureProxy(const ImTextureRef& _texture)
        : texture(_texture)
    {
    }

    /* implicit */ ImTextureProxy(const TextureHandle _texh)
        : texture(ImGuiGlue::ImTextureRefFrom(_texh))
    {
    }

    bool IsValid() const noexcept
    {
        return texture.GetTexID() != ImTextureID_Invalid;
    }

    ImTextureRef texture = nullptr;
};

struct ButtonDesc
{
    const char*    text               = nullptr;         // ImGui 에 전달될 라벨. label이 버튼 텍스트로 사용됨
    ImTextureProxy icon               = {};              // 버튼에 표시될 아이콘 텍스처
    ImVec2         size               = { 0.f, 0.f };    // 버튼 크기 (0,0 이면 자동 크기)
    ImGuiKeyChord  shortcut           = 0;               // 버튼에 대한 단축키
    ImGuiDir       direction          = ImGuiDir_None;   // None이 아닐경우. 화살표 버튼으로 렌더링
    const char*    tooltipDescription = nullptr;         // 툴팁에 표시될 설명 (nullptr이면 툴팁 미표시)

    bool bRenderDescriptionTooltip = true;   // 설명 툴팁 렌더 여부
    bool bRenderText               = true;   // 라벨 텍스트 렌더 여부
};

// base
void Widget_CenteredText(const char* _text, ImVec2 _region = ImVec2 { 0.f, 0.f });
void Widget_FrameAlignedText(const char* _text);
void Widget_BorderedText(const char* _text, ImVec2 _size = ImVec2 { 0.f, 0.f }, bool _bDisabled = false);

bool Widget_DragScalar(const char* _label, ImGuiDataType _dataType, void* _pData, float _vSpeed, const void* _pMin, const void* _pMax, const char* _format, ImGuiSliderFlags _flags);
bool Widget_DragScalarN(const char* _label, ImGuiDataType _dataType, void* _pData, int _numComps, float _speed, const void* _pMin, const void* _pMax, const char* _format, ImGuiSliderFlags _flags);
bool Widget_DragFloat(const char* label, float* p_data, float v_speed = 0.01f, float v_min = -FLT_MAX, float v_max = FLT_MAX, const char* format = "%.3f", ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp);
bool Widget_DragFloat2(const char* label, float p_data[2], float v_speed = 0.01f, float v_min = -FLT_MAX, float v_max = FLT_MAX, const char* format = "%.3f", ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp);
bool Widget_DragFloat3(const char* label, float p_data[3], float v_speed = 0.01f, float v_min = -FLT_MAX, float v_max = FLT_MAX, const char* format = "%.3f", ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp);
bool Widget_DragFloat4(const char* label, float p_data[4], float v_speed = 0.01f, float v_min = -FLT_MAX, float v_max = FLT_MAX, const char* format = "%.3f", ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp);
bool Widget_DragInt(const char* label, int* p_data, float v_speed = 0.05f, int v_min = -INT32_MIN, int v_max = INT32_MAX, const char* format = "%d", ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp);
bool Widget_DragInt2(const char* label, int p_data[2], float v_speed = 0.05f, int v_min = -INT32_MIN, int v_max = INT32_MAX, const char* format = "%d", ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp);

void Widget_HelpMark(const char* _desc);
bool Widget_MenuItem(const char* _label, ImGuiKeyChord _shortcut, bool* _pSelected = nullptr, bool _bEnabled = true);
bool Widget_MenuItem(const char* _label, ImGuiKeyChord _shortcut = 0, bool _bSelected = false, bool _bEnabled = true);

bool Widget_DragTranslation(const char* _label, Vec3& _translation, float _speed = 0.01f, float _min = -FLT_MAX, float _max = FLT_MAX, const char* _fmt = "%.3fm", ImGuiSliderFlags _flag = ImGuiSliderFlags_AlwaysClamp);
bool Widget_DragScale(const char* _label, Vec3& _scaleNorm, float _speedPerc = 1.f, float _minPerc = 0.f, float _maxPerc = FLT_MAX, const char* _fmt = "%.1f%", ImGuiSliderFlags _flag = ImGuiSliderFlags_AlwaysClamp);
bool Widget_DragPercent(const char* _label, float& _pNorm, float _speedPerc = 1.f, float _minPerc = 0.f, float _maxPerc = FLT_MAX, const char* _format = "%.1f%%", ImGuiSliderFlags _flag = ImGuiSliderFlags_AlwaysClamp);
bool Widget_DragPitchYawRoll(const char* _label, Vec3& _pitchYawRollRad, float _speedDeg = 1.f, float _minDeg = -359.f, float _maxDeg = 359.f, const char* _fmt = "%.1fdeg", ImGuiSliderFlags _flag = ImGuiSliderFlags_WrapAround);
bool Widget_DragPitchYaw(const char* _label, Vec2& _pitchYawRad, float _speedDeg = 1.f, float _minDeg = -359.f, float _maxDeg = 359.f, const char* _fmt = "%.1fdeg", ImGuiSliderFlags _flag = ImGuiSliderFlags_WrapAround);
bool Widget_DragAngle(const char* _label, float& _pRad, float _speedDeg = 1.f, float _minDeg = -359.f, float _maxDeg = 359.f, const char* _fmt = "%.1fdeg", ImGuiSliderFlags _flag = ImGuiSliderFlags_WrapAround);

void Widget_ImageFrame(ImTextureProxy _tex, ImVec2 _texSize, ImVec2 _frameSize, ImVec2 _uv0 = { 0.f, 0.f }, ImVec2 _uv1 = { 1.f, 1.f }, ImVec4 _bgCol = { 0.f, 0.f, 0.f, 0.f }, bool _bBorder = false);
void Widget_Image(ImTextureProxy _tex, ImVec2 _size, ImVec2 _uv0 = { 0.f, 0.f }, ImVec2 _uv1 = { 1.f, 1.f });

// sugar
bool Widget_VertexLayoutEntryCombo(const char* _label, Ref<VertexLayoutEntry>& _pVle);
bool Widget_ScriptCombo(const char* _label, Ref<Script>& _pScript, Entity _owner);   // owner for creating script instance
bool Widget_ProjectionCombo(const char* _label, eProjection& _projection);
bool Widget_TextureChannelCombo(const char* _label, eTextureChannel& _colorChannel, bool _bExcludeAlphaChannel = false);
bool Widget_TextureFormatCombo(const char* _label, eTextureFormat& _format, bool _bExcludeDepthFormat = false);

// asset
bool Widget_MeshSelector(const char* _label, Ref<Mesh>& _pMesh);
bool Widget_MaterialSelector(const char* _label, Ref<Material>& _pMaterial);
bool Widget_ImageTextureSelector(const char* _label, Ref<ImageTexture>& _pImageTexture, const ImVec2& _size = ImVec2 { 128.f, 128.f });

bool Widget_Acroball(const char* _label, Transform& _transform, ImVec2 _pos, float _length);
bool Widget_Combo(const char* _label, Int32& _curItem, Span<const char* const> _items);

bool Widget_Button(const ButtonDesc& _desc);

void Widget_WrappedText(const char* _text, float _wrapWidth);
bool Widget_PathSelector(const char* _label, fs::path& _path, FileDialog& _fd);

}   // namespace jam