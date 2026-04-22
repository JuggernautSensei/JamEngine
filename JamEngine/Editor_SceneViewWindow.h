#pragma once
#include "Camera.h"
#include "EditorRenderer.h"
#include "EditorWidgets.h"
#include "GraphicsBase.h"
#include "InfiniteDragger.h"
#include "Transform.h"

namespace Jam
{

class MouseWheelEvent;
class KeyDownEvent;
class KeyUpEvent;
class MouseUpEvent;
class MouseDownEvent;
class SceneChangedEvent;
class Scene;
class Event;

class Editor_SceneViewWindow
{

    enum class eGizmoOp
    {
        None,
        Translation,
        Rotation,
        Scale,
    };

    enum class eGizmoOrient
    {
        World,
        Local,
    };

public:
    Editor_SceneViewWindow();
    ~Editor_SceneViewWindow();

    Editor_SceneViewWindow(const Editor_SceneViewWindow&)                = delete;
    Editor_SceneViewWindow& operator=(const Editor_SceneViewWindow&)     = delete;
    Editor_SceneViewWindow(Editor_SceneViewWindow&&) noexcept            = delete;
    Editor_SceneViewWindow& operator=(Editor_SceneViewWindow&&) noexcept = delete;

    void OnUpdate(float _deltaTime);
    void OnRender();
    void OnImGuiRender(bool& _bShow);
    void OnEvent(Event& _event);

    void  WriteToArchive(ArchiveWriter& _writer) const;
    Error ReadFromArchive(ArchiveReader& _reader);

private:
    // draws
    void DrawGizmo_() const;
    void Menu_Options();

    // update
    void OnUpdateEdit_(float _deltaSec);
    void OnUpdateRuntime_(float _deltaSec) const;

    // render
    void OnRenderEdit();
    void OnRenderRuntime() const;
    void Resize_(Int32 _width, Int32 _height);

    // event
    void OnSceneChangedEvent_(const SceneChangedEvent& _event);
    void OnMouseDownEvent_(MouseDownEvent& _event) const;
    void OnMouseUpEvent_(MouseUpEvent& _event) const;
    void OnKeyDownEvent_(KeyDownEvent& _event) const;
    void OnKeyUpEvent_(KeyUpEvent& _event) const;
    void OnMouseWheelEvent_(MouseWheelEvent& _event) const;

    // widget
    void Widget_GizmoSelector(eGizmoOp _op);
    void Widget_GizmoOriendToggle_();
    void Widget_GizmoSnapToggle_();
    void Widget_CameraAcroball_();
    void Widget_RunButton_();
    void Widget_StopButton_() const;
    void Widget_EditorModeSwitchButton_();

    // view
    FrameBufferHandle m_fbh        = kInvalidHandle;
    TextureHandle     m_renderTexh = kInvalidHandle;

    FrameBufferHandle m_fbhWithDepth   = kInvalidHandle;
    TextureHandle     m_sceneDepthTexh = kInvalidHandle;

    // resize
    Int32 m_width            = 0;
    Int32 m_height           = 0;
    Int32 m_resizeWidth      = 0;
    Int32 m_resizeHeight     = 0;
    bool  m_bResizeRequested = false;

    // play <-> edit 상태 복구용
    bool   m_bRecoverRequested      = false;
    String m_backUpSceneData        = {};
    UID    m_focusedEntityBackupUid = {};

    // camera
    Camera    m_camera                 = {};
    Transform m_cameraTransform        = {};
    Vec3      m_cameraEulerRad         = Vec3::kZero;
    float     m_cameraMoveSpeed        = 5.f;
    float     m_cameraRotSpeed         = 1.2f;
    bool      m_bShowCameraTranslation = true;
    bool      m_bShowCameraForward     = true;

    // edit camera overlay window
    InfiniteDragger m_infiniteDragger;   // init in constructor
    ImVec2          m_sceneViewLeftTop = { 0.f, 0.f };
    ImVec2          m_sceneViewRes     = { 0.f, 0.f };

    bool m_bFocusing = false;
    bool m_bHovering = false;
    bool m_bVisible  = false;

    // gizmo
    eGizmoOp     m_gizmoOp     = eGizmoOp::Translation;
    eGizmoOrient m_gizmoOrient = eGizmoOrient::World;
    bool         m_bGizmoSnap  = false;

    // renderer
    EditorRenderer m_renderer;
    bool           m_bShowGrid = true;
    bool           m_bShowAxis = true;

    // scene cache
    Scene* m_pScene = nullptr;

    static constexpr Vec3   kTranslateSnap    = Vec3 { 0.5f };
    static constexpr Vec3   kRotateSnapDeg    = Vec3 { 15.f };
    static constexpr Vec3   kScaleSnap        = Vec3 { 0.5f };
    static constexpr ImVec2 kModeBtnSize      = ImVec2 { 100.f, 0.f };
    static constexpr RGBA   kBoundingBoxCol   = RGBA { 0xFF9D23FF };
    static constexpr float  kDragSensitivity  = 0.001f;
    static constexpr float  kWheelSensitivity = 0.05f;

    static constexpr float kCameraMoveDecelerationFactor = 0.5f;
    static constexpr float kCameraMoveSpeedAdjustFactor  = 5.f;
    static constexpr float kCameraMoveSpeedMin           = 0.1f;
    static constexpr float kCameraRotSpeedMin            = 0.01f;

    static constexpr float    kAcroballLength = 120.f;
    static constexpr ImGuiCol kAcroballBgCol  = IM_COL32(0, 0, 0, 120);
};

JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Editor_SceneViewWindow);

}   // namespace Jam
