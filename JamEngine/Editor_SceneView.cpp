#include "pch.h"

#include "Editor_SceneView.h"

#include "CameraComponent.h"
#include "Editor.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "Event.h"
#include "Events.h"
#include "Graphics.h"
#include "ImGuiUtilities.h"
#include "Input.h"
#include "JamEngineEditorEmbededIcons.h"
#include "JsonUtilities.h"
#include "SceneDeserializer.h"
#include "SceneManager.h"
#include "SceneSerializer.h"
#include "Vendor/imgui/ImGuizmo.h"

namespace jam
{

void Editor_SceneView::Init()
{
    // init editor renderer
    {
        m_renderer.Init();
    }

    // init editor camera
    {
        m_cameraTransform.SetTranslation(-5, 5, -5);
        m_cameraTransform.SetLookAt(Vec3::Zero);
        m_camera.SetFarZ(300.f);

        const Vec3 euler = m_cameraTransform.GetPitchYawRoll();
        m_pitchRad       = euler.x;
        m_yawRad         = euler.y;
    }

    // load icons
    {
        bool bSucceeded = false;
        bSucceeded |= m_cursorIcon.LoadFromMemory(ConstMemView { CursorIcon_png, CursorIcon_png_size }, eImageFormat::PNG);
        bSucceeded |= m_editIcon.LoadFromMemory(ConstMemView { EditIcon_png, EditIcon_png_size }, eImageFormat::PNG);
        bSucceeded |= m_menuIcon.LoadFromMemory(ConstMemView { MenuIcon_png, MenuIcon_png_size }, eImageFormat::PNG);
        bSucceeded |= m_playIcon.LoadFromMemory(ConstMemView { PlayIcon_png, PlayIcon_png_size }, eImageFormat::PNG);
        bSucceeded |= m_rotateIcon.LoadFromMemory(ConstMemView { RotationIcon_png, RotationIcon_png_size }, eImageFormat::PNG);
        bSucceeded |= m_scaleIcon.LoadFromMemory(ConstMemView { ScalingIcon_png, ScalingIcon_png_size }, eImageFormat::PNG);
        bSucceeded |= m_tranlateIcon.LoadFromMemory(ConstMemView { TranslateIcon_png, TranslateIcon_png_size }, eImageFormat::PNG);
        JAM_ASSERT(bSucceeded, "Failed to load editor scene view icons");
    }
}

void Editor_SceneView::Shutdown()
{
    m_renderer.Shutdown();

    JAM_DESTROY_TEXTURE(m_renderTexh);

    JAM_DESTROY_FRAME_BUFFER(m_fbh);
    JAM_DESTROY_FRAME_BUFFER(m_fbhWithSceneDepth);
}

void Editor_SceneView::OnUpdate(const float _deltaTime)
{
    if (Editor::IsRuntimeMode())
    {
        OnUpdateRuntime_(_deltaTime);
    }
    else
    {
        OnUpdateEdit_(_deltaTime);
    }
}

void Editor_SceneView::OnRender()
{
    if (!m_bVisible)
    {
        return;
    }

    if (m_bNeedResize)
    {
        Resize_(m_resizeWidth, m_resizeHeight);
        m_bNeedResize = false;
    }

    if (Editor::IsRuntimeMode())
    {
        OnRenderRuntime();
    }
    else
    {
        OnRenderEdit();
    }
}

void Editor_SceneView::Widget_EditorModeButton()
{
    if (Editor::IsEditMode())
    {
        if (Widget_RunButton_())
        {
            OnClickRunButton_();
        }
    }
    else if (Editor::IsRuntimeMode())
    {
        if (Widget_EditButton_())
        {
            OnClickEditButton_();
        }
    }
}

void Editor_SceneView::OnImGuiRender(bool& _bShow)
{
    // begin window
    {
        ImGuiWindowFlags flags = ImGuiWindowFlags_None;
        if (Editor::IsRuntimeMode())
        {
            flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
        }

        ImGui::SetNextWindowSizeConstraints({ 300.f, 200.f }, { k_floatMax, k_floatMax });
        m_bVisible = ImGui::Begin("scene view", &_bShow, flags);
        if (!m_bVisible)
        {
            ImGui::End();
            return;
        }
    }

    // Update window states
    {
        m_bHovering = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows | ImGuiHoveredFlags_NoPopupHierarchy);
        m_bFocusing = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        if (m_bHovering && !ImGui::IsAnyItemActive())
        {
            ImGui::SetWindowFocus();
        }
    }

    // Viewport toolbar
    {
        // Options button
        Widget_IconButton("menu", ToImTextureRef(m_menuIcon.GetHandle()));
        if (ImGui::BeginPopupContextItem(0, ImGuiPopupFlags_MouseButtonLeft))
        {
            DrawOptionPopup_();
            ImGui::EndPopup();
        }

        // Gizmo mode selector
        ImGui::SameLine();
        Widget_GizmoButton(eGizmoOp::None, ToImTextureRef(m_cursorIcon.GetHandle()));
        ImGui::SameLine(0.f, 1.f);
        Widget_GizmoButton(eGizmoOp::Translation, ToImTextureRef(m_tranlateIcon.GetHandle()));
        ImGui::SameLine(0.f, 1.f);
        Widget_GizmoButton(eGizmoOp::Rotation, ToImTextureRef(m_rotateIcon.GetHandle()));
        ImGui::SameLine(0.f, 1.f);
        Widget_GizmoButton(eGizmoOp::Scale, ToImTextureRef(m_scaleIcon.GetHandle()));

        // gizmo orient (mtx/local) toggle
        ImGui::SameLine();
        Widget_GizmoOrientButton(eGizmoOrient::World);
        ImGui::SameLine(0.f, 1.f);
        Widget_GizmoOrientButton(eGizmoOrient::Local);

        // snap toggle
        ImGui::SameLine();
        Widget_GizmoSnapButton();

        // Play/Stop button (right side)
        static ImVec2 s_btnLastSize = { 0.f, 0.f };
        ImGui::SameLine();
        ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2 { ImGui::GetContentRegionAvail().x - s_btnLastSize.x, 0.f });
        Widget_EditorModeButton();
        s_btnLastSize = ImGui::GetItemRectSize();   // use next frame
    }

    // Render viewport
    {
        if (ImGui::BeginChild("RenderViewChild", ImGui::GetContentRegionAvail()))
        {
            // check resized
            const ImVec2 avail  = ImGui::GetContentRegionAvail();
            const Int32  width  = static_cast<Int32>(avail.x);
            const Int32  height = static_cast<Int32>(avail.y);

            // not resize when dragging with mouse
            if ((m_width != width || m_height != height) && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                m_resizeWidth  = width;
                m_resizeHeight = height;
                m_bNeedResize  = true;
            }

            // draw scene
            if (JAM_IS_VALID_HANDLE(m_renderTexh))
            {
                m_sceneViewLeftTop = ImGui::GetCursorScreenPos();
                m_sceneViewRes     = avail;

                // draw scene
                ImGui::Image(ToImTextureRef(m_renderTexh), avail);

                // edit mode overlay
                if (Editor::IsEditMode())
                {
                    DrawFocusEntityGizmo_();    // draw gizmo
                    Widget_CameraAcroball_();   // draw camera acroball (right top)
                }
            }
            else
            {
                Widget_BorderedText("cannot render scene view", avail);
            }
        }
        ImGui::EndChild();
    }

    // short cut in edit mode
    if (Editor::IsEditMode())
    {
        if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_D))
            Editor::CloneEntityToActiveScene(Editor::GetFocusEntity());

        if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N))
            Editor::CreateEntityToActiveScene();

        if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S))
            Editor::SaveActiveSceneByDialog();

        if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O))
            Editor::LoadActiveSceneByDialog();

        if (ImGui::Shortcut(ImGuiKey_Delete))
            Editor::DestroyEntityToActiveScene(Editor::GetFocusEntity());
    }

    ImGui::End();
}

void Editor_SceneView::OnEvent(Event& _event)
{
    HandleEvent<SceneChangedEvent>(_event, JAM_BIND_MEMBER_LISTENER(Editor_SceneView::OnSceneChangedEvent_));
    HandleEvent<MouseDownEvent>(_event, JAM_BIND_MEMBER_LISTENER(Editor_SceneView::OnMouseDownEvent_));
    HandleEvent<MouseUpEvent>(_event, JAM_BIND_MEMBER_LISTENER(Editor_SceneView::OnMouseUpEvent_));
    HandleEvent<KeyDownEvent>(_event, JAM_BIND_MEMBER_LISTENER(Editor_SceneView::OnKeyDownEvent_));
    HandleEvent<KeyUpEvent>(_event, JAM_BIND_MEMBER_LISTENER(Editor_SceneView::OnKeyUpEvent_));
    HandleEvent<MouseWheelEvent>(_event, JAM_BIND_MEMBER_LISTENER(Editor_SceneView::OnMouseWheelEvent_));
}

Json Editor_SceneView::OnSerialize()
{
    // serialize viewport settings
    Json json;
    json["cameraMoveSpeed"]       = m_cameraMoveSpeed;
    json["cameraRotSpeed"]        = m_cameraRotSpeed;
    json["cameraFovYRad"]         = m_camera.GetFovYRad();
    json["cameraProjection"]      = m_camera.GetProjection();
    json["drawGrid"]              = m_bShowGrid;
    json["drawAxis"]              = m_bShowAxis;
    json["drawCameraTranslation"] = m_bShowCameraTranslation;
    json["drawCameraForward"]     = m_bShowCameraForward;
    return json;
}

void Editor_SceneView::OnDeserialize(const Json& _json)
{
    try
    {
        m_camera.SetFovYRad(_json.value("cameraFovYRad", m_camera.GetFovYRad()));
        m_camera.SetProjection(ValueEnumFromJson(_json, "cameraProjection", m_camera.GetProjection()));
        m_cameraMoveSpeed        = _json.value("cameraMoveSpeed", m_cameraMoveSpeed);
        m_cameraRotSpeed         = _json.value("cameraRotSpeed", m_cameraRotSpeed);
        m_bShowGrid              = _json.value("drawGrid", m_bShowGrid);
        m_bShowAxis              = _json.value("drawAxis", m_bShowAxis);
        m_bShowCameraTranslation = _json.value("drawCameraTranslation", m_bShowCameraTranslation);
        m_bShowCameraForward     = _json.value("drawCameraForward", m_bShowCameraForward);
    }
    catch (const std::exception& e)
    {
        Log::Warn(JAM_PRETTY_FUNCTION " failed to deserialize: {}", e.what());
    }
}

void Editor_SceneView::DrawFocusEntityGizmo_() const
{
    JAM_ASSERT(Editor::IsEditMode(), "Guizmo is only available in edit mode.");

    if (m_gizmoOp == eGizmoOp::None)
    {
        return;
    }

    Entity entity = Editor::GetFocusEntity();
    if (!entity)
    {
        return;
    }

    TransformHierarchy& th   = entity.GetTransformHierarchy();
    Mat4                mtx  = th.GetWorldMatrix();
    const Mat4          view = m_cameraTransform.GetViewMatrix();
    const Mat4          proj = m_camera.CreateProjMatrix();

    const float x        = m_sceneViewLeftTop.x;
    const float y        = m_sceneViewLeftTop.y;
    const float width    = m_sceneViewRes.x;
    const float height   = m_sceneViewRes.y;
    const bool  bIsOrtho = m_camera.GetProjection() == eProjection::Orthographic;

    const ImGuizmo::OPERATION gizmoOp = m_gizmoOp == eGizmoOp::Translation ? ImGuizmo::TRANSLATE
                                      : m_gizmoOp == eGizmoOp::Rotation    ? ImGuizmo::ROTATE
                                                                           : ImGuizmo::SCALE;

    const ImGuizmo::MODE gizmoMode = m_gizmoOrient == eGizmoOrient::World ? ImGuizmo::WORLD
                                                                          : ImGuizmo::LOCAL;

    const float* pSnap = nullptr;
    if (m_bGizmoSnap || Input::IsKeyDown(eKey::Shift))
    {
        pSnap = m_gizmoOp == eGizmoOp::Translation ? &k_translateSnap.x
              : m_gizmoOp == eGizmoOp::Rotation    ? &k_rotateSnapDeg.x
                                                   : &k_scaleSnap.x;
    }

    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
    ImGuizmo::SetOrthographic(bIsOrtho);
    ImGuizmo::SetRect(x, y, width, height);
    if (ImGuizmo::Manipulate(&view._11, &proj._11, gizmoOp, gizmoMode, &mtx._11, nullptr, pSnap))
    {
        const Transform trans { mtx };
        th.SetWorldTransform(trans);
    }
}

void Editor_SceneView::DrawOptionPopup_()
{
    ImGui::SeparatorText("editor camera");
    {
        const eProjection proj    = m_camera.GetProjection();
        float             fovYRad = m_camera.GetFovYRad();

        if (ImGui::MenuItem("reset position"))
        {
            m_cameraTransform.SetTranslation(0.f, 0.f, 0.f);
        }

        if (ImGui::MenuItem("reset rotation"))
        {
            m_pitchRad = m_yawRad = 0.f;
            m_cameraTransform.SetRotation(0.f, 0.f, 0.f);
        }

        if (ImGui::MenuItem("perspective projection", 0, proj == eProjection::Perspective))
        {
            m_camera.SetProjection(eProjection::Perspective);
        }

        if (ImGui::MenuItem("orthographic projection", 0, proj == eProjection::Orthographic))
        {
            m_camera.SetProjection(eProjection::Orthographic);
        }

        if (ImGui::BeginMenu("field of view"))
        {
            if (Widget_DragAngle("##CameraFov", &fovYRad, 0.5f, 1.f, 179.f))
                m_camera.SetFovYRad(fovYRad);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("move speed"))
        {
            Widget_DragFloat("##CameraMoveSpeed", &m_cameraMoveSpeed, 0.1f, k_cameraMoveSpeedMin, k_cameraMoveSpeedMax, "%.1f");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("rotation speed"))
        {
            Widget_DragFloat("##CameraRotationSpeed", &m_cameraRotSpeed, 0.01f, k_cameraRotSpeedMin, k_cameraRotSpeedMax, "%.1f");
            ImGui::EndMenu();
        }

        ImGui::MenuItem("show translation", 0, &m_bShowCameraTranslation);
        ImGui::MenuItem("show forward", 0, &m_bShowCameraForward);
    }

    ImGui::SeparatorText("gizmo");
    {
        if (ImGui::MenuItem("cursor", 0, m_gizmoOp == eGizmoOp::None))
            m_gizmoOp = eGizmoOp::None;
        else if (ImGui::MenuItem("translation", 0, m_gizmoOp == eGizmoOp::Translation))
            m_gizmoOp = eGizmoOp::Translation;
        else if (ImGui::MenuItem("rotation", 0, m_gizmoOp == eGizmoOp::Rotation))
            m_gizmoOp = eGizmoOp::Rotation;
        else if (ImGui::MenuItem("scale", 0, m_gizmoOp == eGizmoOp::Scale))
            m_gizmoOp = eGizmoOp::Scale;

        if (ImGui::MenuItem("mtx orient", 0, m_gizmoOrient == eGizmoOrient::World))
            m_gizmoOrient = eGizmoOrient::World;
        else if (ImGui::MenuItem("local orient", 0, m_gizmoOrient == eGizmoOrient::Local))
            m_gizmoOrient = eGizmoOrient::Local;
    }

    ImGui::SeparatorText("view options");
    {
        ImGui::MenuItem("show grid", 0, &m_bShowGrid);
        ImGui::MenuItem("show axis", 0, &m_bShowAxis);
    }
}

void Editor_SceneView::OnUpdateEdit_(const float _deltaSec)
{
    // on mouse press
    if (Input::IsMousePressed(eMouse::RightButton) || Input::IsMousePressed(eMouse::MiddleButton))
    {
        const auto [x, y] = Input::GetMousePos();
        m_moveStartMouseX = x;
        m_moveStartMouseY = y;
        Input::SetMouseVisibility(false);
        m_bMouseCaptured = true;
    }

    // on mouse release
    if (Input::IsMouseReleased(eMouse::RightButton) || Input::IsMouseReleased(eMouse::MiddleButton))
    {
        // release both buttons to cancel capture
        if (Input::IsMouseUp(eMouse::RightButton) && Input::IsMouseUp(eMouse::MiddleButton))
        {
            Input::SetMousePos(m_moveStartMouseX, m_moveStartMouseY);
            Input::SetMouseVisibility(true);
            m_bMouseCaptured = false;
        }
    }

    // right button action
    if (Input::IsMouseDown(eMouse::RightButton))
    {
        // translation
        {
            const float factor = Input::IsKeyDown(eKey::Shift) ? k_cameraMoveDecelerationFactor : 1.f;
            const float dMove  = m_cameraMoveSpeed * factor * _deltaSec;

            if (Input::IsKeyDown(eKey::A))
                m_cameraTransform.AddTranslation(m_cameraTransform.GetLeft() * dMove);

            if (Input::IsKeyDown(eKey::D))
                m_cameraTransform.AddTranslation(m_cameraTransform.GetRight() * dMove);

            if (Input::IsKeyDown(eKey::W))
                m_cameraTransform.AddTranslation(m_cameraTransform.GetForward() * dMove);

            if (Input::IsKeyDown(eKey::S))
                m_cameraTransform.AddTranslation(m_cameraTransform.GetBackward() * dMove);

            if (Input::IsKeyDown(eKey::Q))
                m_cameraTransform.AddTranslation(m_cameraTransform.GetDown() * dMove);

            if (Input::IsKeyDown(eKey::E))
                m_cameraTransform.AddTranslation(m_cameraTransform.GetUp() * dMove);
        }

        // rotation
        {
            const auto [x, y] = Input::GetMousePos();
            const Int32 dx    = m_moveStartMouseX - x;
            const Int32 dy    = m_moveStartMouseY - y;
            if (dx != 0 || dy != 0)
            {
                const float dragSpeed = m_cameraRotSpeed * k_dragSensitivity;

                m_yawRad -= static_cast<float>(dx) * dragSpeed;
                m_yawRad = Wrap(m_yawRad, -k_pi, k_pi);

                m_pitchRad -= static_cast<float>(dy) * dragSpeed;
                m_pitchRad = Clamp(m_pitchRad, ToRad(-89.f), ToRad(89.f));

                m_cameraTransform.SetRotation(m_pitchRad, m_yawRad, 0.f);
            }
        }

        // infinite mouse drag
        Input::SetMousePos(m_moveStartMouseX, m_moveStartMouseY);
    }

    // middle button action
    if (Input::IsMouseDown(eMouse::MiddleButton))
    {
        const auto [x, y] = Input::GetMousePos();
        const Int32 dx    = m_moveStartMouseX - x;
        const Int32 dy    = m_moveStartMouseY - y;

        // translation
        if (dx != 0 || dy != 0)
        {
            const float dragSpeed = m_cameraMoveSpeed * k_dragSensitivity;
            const Vec3  right     = m_cameraTransform.GetRight();
            const Vec3  up        = m_cameraTransform.GetUp();
            const Vec3  dRight    = right * static_cast<float>(dx) * dragSpeed;
            const Vec3  dUp       = up * static_cast<float>(-dy) * dragSpeed;
            m_cameraTransform.AddTranslation(dRight + dUp);
        }

        Input::SetMousePos(m_moveStartMouseX, m_moveStartMouseY);
    }

    // mouse wheel action
    const float dWheel = Input::GetMouseDeltaWheel();
    if (!IsZeroApprox(dWheel))
    {
        if (Input::IsMouseDown(eMouse::RightButton))
        {
            // adjust move speed
            const float delta = dWheel * k_wheelSensitivity * k_cameraMoveSpeedAdjustFactor;
            m_cameraMoveSpeed = Clamp(m_cameraMoveSpeed + delta, k_cameraMoveSpeedMin, k_cameraMoveSpeedMax);
        }
        else
        {
            // move forward/backward (like zoom)
            const float delta = m_cameraMoveSpeed * dWheel * k_wheelSensitivity;
            m_cameraTransform.AddTranslation(m_cameraTransform.GetForward() * delta);
        }
    }

    // esc to release focus
    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        Editor::SetFocusEntity(Entity::s_null);
    }
}

void Editor_SceneView::OnUpdateRuntime_(const float _deltaSec) const
{
    SceneManager::Update(_deltaSec);
}

void Editor_SceneView::OnRenderEdit()
{
    const UInt16   w  = NarrowCast<UInt16>(m_width);
    const UInt16   h  = NarrowCast<UInt16>(m_height);
    const Viewport vp = { w, h };

    // setup render view
    RenderView renderView;
    renderView.fbh    = m_fbh;
    renderView.vp     = vp;
    renderView.eyePos = m_cameraTransform.GetTranslation();
    renderView.view   = m_cameraTransform.GetViewMatrix();
    renderView.proj   = m_camera.CreateProjMatrix();

    // render scene
    SceneManager::Render(renderView);

    // render editor overlays
    if (SceneManager::HasActiveScene())
    {
        // lazy create (because scene depth texture may change on scene load)
        {
            const Ref<Scene>&   pScene         = SceneManager::GetActiveScene();
            const TextureHandle sceneDepthTexh = pScene->GetDepthTexture();
            if (m_sceneDepthTexh != sceneDepthTexh)
            {
                JAM_DESTROY_FRAME_BUFFER(m_fbhWithSceneDepth);
                const TextureHandle attachment[] = { m_renderTexh, sceneDepthTexh };
                m_fbhWithSceneDepth              = Graphics::CreateFrameBuffer(attachment);
            }
        }

        // render overlays (need scene depth)
        if (JAM_IS_VALID_HANDLE(m_fbhWithSceneDepth))
        {
            renderView.fbh = m_fbhWithSceneDepth;
            m_renderer.Begin(renderView);

            // draw grid and axis
            {
                eDrawGridFlags flags = eDrawGridFlags_None;

                if (m_bShowGrid)
                    flags |= eDrawGridFlags_Grid;

                if (m_bShowAxis)
                    flags |= eDrawGridFlags_Axis;

                if (flags != eDrawGridFlags_None)
                    m_renderer.DrawGrid(flags);
            }

            // draw focused entity
            {
                const Entity entity = Editor::GetFocusEntity();
                if (entity)
                {
                //{
                //    // bounding box
                //    if (entity.HasComponent<RenderableComponent>())
                //    {
                //        const RenderableComponent& rc = entity.GetComponent<RenderableComponent>();
                //        if (const Ref<Mesh>& pMesh = rc.pMesh)
                //        {
                //            const Mat4& mtx = entity.GetTransformHierarchy().GetWorldMatrix();
                //            m_renderer.DrawBoundingBox(pMesh->GetBoundingBox(), mtx, k_boundingBoxCol);
                //        }
                //    }

                //    // camera frustum
                //    if (entity.HasComponent<CameraComponent>())
                //    {
                //        const TransformHierarchy& th   = entity.GetTransformHierarchy();
                //        const Mat4                view = th.GetWorldViewMatrix();

                //        const CameraComponent& cc   = entity.GetComponent<CameraComponent>();
                //        const Camera&          cmr  = cc.camera;
                //        const Mat4             proj = cmr.CreateProjMatrix();

                //        m_renderer.DrawFrustum(view * proj, k_boundingBoxCol);
                //    }
                }
            }

            m_renderer.End();
        }
    }
}

void Editor_SceneView::OnRenderRuntime() const
{
    const UInt16   w  = NarrowCast<UInt16>(m_width);
    const UInt16   h  = NarrowCast<UInt16>(m_height);
    const Viewport vp = { w, h };
    SceneManager::RenderRuntime(m_fbh, vp);
}

void Editor_SceneView::Resize_(const Int32 _width, const Int32 _height)
{
    JAM_DESTROY_TEXTURE(m_renderTexh);
    JAM_DESTROY_FRAME_BUFFER(m_fbh);

    // resize current resolution
    m_width  = _width;
    m_height = _height;

    // render target texture
    m_renderTexh = Graphics::CreateTexture2D(_width, _height, eTextureFormat::RGBA8_UNorm, 1, eTextureCreateFlags_RenderTarget);
    Graphics::SetTextureName(m_renderTexh, "scene view texture");

    // frame buffer
    m_fbh = Graphics::CreateFrameBuffer(m_renderTexh);
    Graphics::SetFrameBufferName(m_fbh, "scene view frame buffer");

    // camera
    m_camera.SetWidth(static_cast<float>(_width));
    m_camera.SetHeight(static_cast<float>(_height));

    // resize scene manager
    SceneManager::Resize(_width, _height);
}

void Editor_SceneView::OnSceneChangedEvent_(const SceneChangedEvent& _event)
{
    if (m_bNeedRecover && m_backupEntityUID != k_invalidUID)
    {
        const Ref<Scene>& pScene = SceneManager::GetActiveScene();
        const Entity      entity = pScene->GetEntity(m_backupEntityUID);
        Editor::SetFocusEntity(entity);

        m_backupEntityUID = k_invalidUID;
        m_bNeedRecover    = false;
    }

    m_bNeedResize = true;
}

void Editor_SceneView::OnMouseDownEvent_(MouseDownEvent& _event) const
{
    if (!m_bHovering)
    {
        _event.SetHandled(true);
    }
}

void Editor_SceneView::OnMouseUpEvent_(MouseUpEvent& _event) const
{
    if (!m_bHovering)
    {
        _event.SetHandled(true);
    }
}

void Editor_SceneView::OnKeyDownEvent_(KeyDownEvent& _event) const
{
    if (!m_bFocusing)
    {
        _event.SetHandled(true);
    }
}

void Editor_SceneView::OnKeyUpEvent_(KeyUpEvent& _event) const
{
    if (!m_bFocusing)
    {
        _event.SetHandled(true);
    }
}

void Editor_SceneView::OnMouseWheelEvent_(MouseWheelEvent& _event) const
{
    if (!m_bHovering)
    {
        _event.SetHandled(true);
    }
}

void Editor_SceneView::OnClickRunButton_()
{
    Editor::SetEditorMode(eEditorMode::Runtime);

    // current scene serialize -> backup
    SceneSerializer serializer;
    serializer.Serialize(SceneManager::GetActiveScene());
    m_backupSceneJson = serializer.GetJson();

    // backup focused entity UID
    const Entity entity = Editor::GetFocusEntity();
    m_backupEntityUID   = entity.GetUID();

    // mark need recover flag
    m_bNeedRecover = true;
}

void Editor_SceneView::OnClickEditButton_()
{
    JAM_ASSERT(!m_backupSceneJson.empty(), "Temporary scene json is empty on stop!");

    Editor::SetEditorMode(eEditorMode::Edit);

    // recover scene from backup json
    SceneDeserializer deserializer;
    deserializer.LoadFromJson(m_backupSceneJson);

    // deserialize
    if (!deserializer.Deserialize())
    {
        JAM_ASSERT(0, "Failed to deserialize scene on stop: {}", GetErrorString());
    }

    // clear temporary json
    m_backupSceneJson.clear();
    SceneManager::LoadScene(deserializer.GetScene());

    // recover focused entity is handled in OnSceneChangedEvent_
}

bool Editor_SceneView::Widget_GizmoButton(const eGizmoOp _op, const ImTextureRef _tex)
{
    constexpr const char* const k_names[] = {
        "cursor",
        "translation",
        "rotation",
        "scale"
    };

    constexpr const char* const k_tooltips[] = {
        "cursor (q)",
        "tranlate mode (w)",
        "rotate mode (e)",
        "scale mode (r)"
    };

    constexpr ImGuiKey k_shortcuts[] = {
        ImGuiKey_Q,
        ImGuiKey_W,
        ImGuiKey_E,
        ImGuiKey_R
    };

    const bool bSelected = (m_gizmoOp == _op);
    bool       bChanged  = false;

    // disabled style
    if (bSelected)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }

    // short cut
    if (!m_bMouseCaptured)
    {
        ImGui::SetNextItemShortcut(k_shortcuts[EnumToInt(_op)]);
    }

    // button
    if (Widget_IconButton(k_names[EnumToInt(_op)], _tex))
    {
        m_gizmoOp = _op;
        bChanged  = true;
    }

    // pop style
    if (bSelected)
    {
        ImGui::PopStyleColor();
    }

    // tooltip
    if (ImGui::BeginItemTooltip())
    {
        ImGui::TextUnformatted(k_tooltips[EnumToInt(_op)]);
        ImGui::EndTooltip();
    }

    return bChanged;
}

bool Editor_SceneView::Widget_GizmoOrientButton(const eGizmoOrient _orient)
{
    constexpr const char* const k_names[] = {
        "mtx",
        "local",
    };

    constexpr const char* const k_tooltips[] = {
        "mtx orient (toggle to t)",
        "local orient (toggle to t)",
    };

    const bool bSelected = (m_gizmoOrient == _orient);
    bool       bChanged  = false;

    // disabled style
    if (bSelected)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }

    // short cut (toggle)
    if (!m_bMouseCaptured && _orient != m_gizmoOrient)
    {
        ImGui::SetNextItemShortcut(ImGuiKey_T);
    }

    // button
    if (ImGui::Button(k_names[EnumToInt(_orient)]))
    {
        m_gizmoOrient = _orient;
        bChanged      = true;
    }

    // pop style
    if (bSelected)
    {
        ImGui::PopStyleColor();
    }

    // tooltip
    if (ImGui::BeginItemTooltip())
    {
        ImGui::TextUnformatted(k_tooltips[EnumToInt(_orient)]);
        ImGui::EndTooltip();
    }

    return bChanged;
}

void Editor_SceneView::Widget_GizmoSnapButton()
{
    bool bSelected = m_bGizmoSnap;

    // button
    if (bSelected)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }

    // short cut (toggle)
    if (!m_bMouseCaptured)
    {
        ImGui::SetNextItemShortcut(ImGuiKey_Y);
    }

    const char* label = bSelected ? "snap on" : "snap off";
    if (ImGui::Button(label))
    {
        m_bGizmoSnap = !m_bGizmoSnap;
    }

    if (bSelected)
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::BeginItemTooltip())
    {
        ImGui::TextUnformatted("gizmo snap mode (toggle with y)\n"
                               "holding left shift while dragging also enables snap mode.");
        ImGui::EndTooltip();
    }
}

void Editor_SceneView::Widget_CameraAcroball_()
{
    const ImGuiStyle& style = ImGui::GetStyle();

    // draw acroball
    ImVec2 pos;
    pos.x = m_sceneViewLeftTop.x + m_sceneViewRes.x - k_acroballLength - style.WindowPadding.x,
    pos.y = m_sceneViewLeftTop.y + style.WindowPadding.y;

    if (Widget_Acroball("Acroball", m_cameraTransform, pos, k_acroballLength))
    {
        const Vec3 euler = m_cameraTransform.GetPitchYawRoll();
        m_pitchRad       = euler.x;
        m_yawRad         = euler.y;
    }

    // draw camera info
    if (m_bShowCameraTranslation || m_bShowCameraForward)
    {
        const ImVec2 rectMin        = { pos.x, pos.y + k_acroballLength + style.WindowPadding.y /* for consistency use window padding */ };
        ImVec2       rectMax        = { pos.x + k_acroballLength, rectMin.y + style.WindowPadding.y * 2.f };
        const float  textItemHeight = ImGui::GetTextLineHeight() * 4 + style.ItemSpacing.y;

        if (m_bShowCameraTranslation)
            rectMax.y += textItemHeight;

        if (m_bShowCameraForward)
            rectMax.y += textItemHeight;

        ImVec2 textPos = rectMin + style.WindowPadding;

        // background
        ImGui::GetWindowDrawList()->AddRectFilled(rectMin, rectMax, k_acroballBgCol);

        // translation vector
        if (m_bShowCameraTranslation)
        {
            const Vec3 translation = m_cameraTransform.GetTranslation();
            ImGui::SetCursorScreenPos(textPos);
            ImGui::Text("translation\nx: %.2f\ny: %.2f\nz: %.2f", translation.x, translation.y, translation.z);
            textPos.y += textItemHeight;
        }

        // forward vector
        if (m_bShowCameraForward)
        {
            const Vec3 forward = m_cameraTransform.GetForward();
            ImGui::SetCursorScreenPos(textPos);
            ImGui::Text("forward\nfx: %.2f\nfy: %.2f\nfz: %.2f", forward.x, forward.y, forward.z);
        }
    }
}

bool Editor_SceneView::Widget_RunButton_() const
{
    ImGui::PushStyleColor(ImGuiCol_Button, Editor::GetStyleColorU32(eEditorStyleColor_RunButtonBg));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Editor::GetStyleColorU32(eEditorStyleColor_RunButtonBgActive));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Editor::GetStyleColorU32(eEditorStyleColor_RunButtonBgHovered));
    const bool bClicked = Widget_IconButton("run", ToImTextureRef(m_playIcon.GetHandle()), k_editModeBtnSize);
    ImGui::PopStyleColor(3);
    return bClicked;
}

bool Editor_SceneView::Widget_EditButton_() const
{
    ImGui::PushStyleColor(ImGuiCol_Button, Editor::GetStyleColorU32(eEditorStyleColor_EditButtonBg));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Editor::GetStyleColorU32(eEditorStyleColor_EditButtonBgActive));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Editor::GetStyleColorU32(eEditorStyleColor_EditButtonBgHovered));
    const bool bClicked = Widget_IconButton("edit", ToImTextureRef(m_editIcon.GetHandle()), k_editModeBtnSize);
    ImGui::PopStyleColor(3);
    return bClicked;
}

}   // namespace jam