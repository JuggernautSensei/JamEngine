#pragma once
#include "Camera.h"
#include "EditorRenderer.h"
#include "EditorWidgets.h"
#include "Event.h"
#include "GraphicsBase.h"
#include "JsonWriter.h"
#include "JsonReader.h"
#include "Transform.h"

namespace jam
{

class MouseWheelEvent;
class KeyDownEvent;
class KeyUpEvent;
class MouseUpEvent;
class MouseDownEvent;
class SceneChangedEvent;
class Scene;
class Event;

class Editor_SceneView
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
    void Init();
    void Shutdown();

    void OnUpdate(float _deltaTime);
    void OnRender();
    void OnImGuiRender(bool& _bShow);
    void OnEvent(Event& _event);

    void Serialize(JsonWriter _writer) const;
    void Deserialize(JsonReader _reader);

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
    bool Widget_GizmoButton(eGizmoOp _op, const Ref<ImageTexture>& _pTex);
    bool Widget_GizmoOrientButton(eGizmoOrient _orient);
    void Widget_GizmoSnapButton();
    void Widget_CameraAcroball_();
    bool Widget_RunButton_() const;
    bool Widget_StopButton_() const;
    void Widget_EditorModeSwitchButton();

    // view
    FrameBufferHandle m_fbh        = k_invalidHandle;
    TextureHandle     m_renderTexh = k_invalidHandle;

    FrameBufferHandle m_fbhWithDepth   = k_invalidHandle;
    TextureHandle     m_sceneDepthTexh = k_invalidHandle;

    // resize
    Int32 m_width            = 0;
    Int32 m_height           = 0;
    Int32 m_resizeWidth      = 0;
    Int32 m_resizeHeight     = 0;
    bool  m_bResizeRequested = false;

    // play <-> edit 상태 복구용
    bool m_bRecoverRequested      = false;
    Json m_backupSceneJson        = {};
    UID  m_focusedEntityBackupUid = {};

    // camera
    Camera    m_camera                 = {};
    Transform m_cameraTransform        = {};
    float     m_pitchRad               = 0.f;
    float     m_yawRad                 = 0.f;
    float     m_cameraMoveSpeed        = 5.f;
    float     m_cameraRotSpeed         = 1.2f;
    bool      m_bShowCameraTranslation = true;
    bool      m_bShowCameraForward     = true;

    InfiniteDragHelper m_dragHelper = {};

    // edit camera overlay window
    ImVec2 m_sceneViewLeftTop = ImVec2 { 0.f, 0.f };
    ImVec2 m_sceneViewRes     = ImVec2 { 0.f, 0.f };

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

    // icons
    Ref<ImageTexture> m_cursorIcon     = {};
    Ref<ImageTexture> m_stopIcon       = {};
    Ref<ImageTexture> m_optionsIcon       = {};
    Ref<ImageTexture> m_playIcon       = {};
    Ref<ImageTexture> m_rotatationIcon = {};
    Ref<ImageTexture> m_scaleIcon      = {};
    Ref<ImageTexture> m_tranlationIcon = {};

    static constexpr Vec3   k_translateSnap    = Vec3 { 0.5f };
    static constexpr Vec3   k_rotateSnapDeg    = Vec3 { 15.f };
    static constexpr Vec3   k_scaleSnap        = Vec3 { 0.5f };
    static constexpr ImVec2 k_modeBtnSize      = ImVec2 { 100.f, 0.f };
    static constexpr RGBA   k_boundingBoxCol   = RGBA { 0xFF9D23FF };
    static constexpr float  k_dragSensitivity  = 0.001f;
    static constexpr float  k_wheelSensitivity = 0.05f;

    static constexpr float k_cameraMoveDecelerationFactor = 0.5f;
    static constexpr float k_cameraMoveSpeedAdjustFactor  = 5.f;
    static constexpr float k_cameraMoveSpeedMin           = 0.1f;
    static constexpr float k_cameraRotSpeedMin            = 0.01f;

    static constexpr float    k_acroballLength = 120.f;
    static constexpr ImGuiCol k_acroballBgCol  = IM_COL32(0, 0, 0, 120);
};

}   // namespace jam
