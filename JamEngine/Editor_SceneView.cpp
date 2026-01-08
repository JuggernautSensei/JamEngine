#include "pch.h"

#include "Editor_SceneView.h"

#include "CameraComponent.h"
#include "Editor.h"
#include "EditorWidgets.h"
#include "Event.h"
#include "Events.h"
#include "Graphics.h"
#include "ImGuiUtilities.h"
#include "Input.h"
#include "RenderableComponent.h"
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
    }

    // load icons
    {
        m_cursorIcon     = Editor::GetIcon(eEditorIcon::Cursor);
        m_stopIcon       = Editor::GetIcon(eEditorIcon::Stop);
        m_optionsIcon    = Editor::GetIcon(eEditorIcon::Menu);
        m_playIcon       = Editor::GetIcon(eEditorIcon::Play);
        m_rotatationIcon = Editor::GetIcon(eEditorIcon::Rotation);
        m_scaleIcon      = Editor::GetIcon(eEditorIcon::Scale);
        m_tranlationIcon = Editor::GetIcon(eEditorIcon::Translation);
    }
}

void Editor_SceneView::Shutdown()
{
    m_renderer.Shutdown();

    JAM_DESTROY_TEXTURE(m_renderTexh);

    JAM_DESTROY_FRAME_BUFFER(m_fbh);
    JAM_DESTROY_FRAME_BUFFER(m_fbhWithDepth);
}

void Editor_SceneView::OnUpdate(const float _deltaTime)
{
    if (Editor::GetMode() == eEditorMode::Play)
    {
        OnUpdateRuntime_(_deltaTime);
    }
    else if (Editor::GetMode() == eEditorMode::Edit)
    {
        OnUpdateEdit_(_deltaTime);
    }
    else
    {
        JAM_ASSERT(0, "unknown editor mode.");
    }
}

void Editor_SceneView::OnRender()
{
    if (!m_bVisible)
    {
        return;
    }

    // resize 처리
    if (m_bResizeRequested)
    {
        Resize_(m_resizeWidth, m_resizeHeight);
        m_bResizeRequested = false;
    }

    // 렌더링
    const eEditorMode mode = Editor::GetMode();
    if (mode == eEditorMode::Play)
    {
        OnRenderRuntime();
    }
    else if (mode == eEditorMode::Edit)
    {
        OnRenderEdit();
    }
    else
    {
        JAM_ASSERT(0, "unknown editor mode.");
    }
}

void Editor_SceneView::Widget_EditorModeSwitchButton()
{
    const eEditorMode mode = Editor::GetMode();
    if (mode == eEditorMode::Edit)
    {
        if (Widget_RunButton_())
        {
            Editor::SetMode(eEditorMode::Play);

            // 현재 씬을 직렬화하여 백업
            SceneSerializer serializer = {};
            serializer.Serialize(SceneManager::GetActiveScene());
            m_backupSceneJson = serializer.GetJson();

            // 현재 포커스된 엔티티 UID 백업
            const Entity entity      = Editor::GetFocusedEntity();
            m_focusedEntityBackupUid = entity.GetUID();

            // 씬이 변경되었을때 복구 처리 필요
            m_bRecoverRequested = true;
        }
    }
    else if (mode == eEditorMode::Play)
    {
        if (Widget_StopButton_())
        {
            JAM_ASSERT(!m_backupSceneJson.empty(), "Temporary scene json is empty on stop!");
            Editor::SetMode(eEditorMode::Edit);

            // 씬 역직렬화
            SceneDeserializer deserializer = {};
            deserializer.LoadJson(m_backupSceneJson);
            if (!deserializer.Deserialize())
            {
                JAM_ASSERT(0, "Failed to deserialize scene on stop: {}", GlobalBuffer::GetLastError());
            }

            // clear backup
            m_backupSceneJson.clear();
            SceneManager::LoadScene(deserializer.GetScene());

            // 복구 로직은 OnSceneChangedEvent_에 의해 처리됨
        }
    }
    else
    {
        JAM_ASSERT(0, "unknown editor mode.");
    }
}

void Editor_SceneView::OnImGuiRender(
    bool& _bShow)
{
    // begin window
    {
        ImGuiWindowFlags flags = ImGuiWindowFlags_None;
        if (Editor::GetMode() == eEditorMode::Play)
        {
            flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
        }

        ImGui::SetNextWindowSizeConstraints({ 200.f, 200.f }, { FLT_MAX, FLT_MAX });
        m_bVisible = ImGui::Begin("scene view", &_bShow, flags);
        if (!m_bVisible)
        {
            ImGui::End();
            return;
        }
    }

    // update window states
    {
        m_bHovering = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows | ImGuiHoveredFlags_NoPopupHierarchy);
        m_bFocusing = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        if (m_bHovering && !ImGui::IsAnyItemActive())
        {
            // 마우스가 hover 중일때는 자동으로 포커스
            // 단, scene view 위에 다른 위젯이 포커스된 경우는 제외
            ImGui::SetWindowFocus();
        }
    }

    // viewport toolbar
    {
        // Options button
        {
            ButtonDesc desc = {};
            desc.text      = "options";
            desc.icon       = m_optionsIcon;
            Widget_Button(desc);

            // option button menu
            if (ImGui::BeginPopupContextItem(0, ImGuiPopupFlags_MouseButtonLeft))
            {
                Menu_Options();
                ImGui::EndPopup();
            }
        }

        // Gizmo mode selector
        ImGui::SameLine();
        Widget_GizmoButton(eGizmoOp::None, m_cursorIcon);
        ImGui::SameLine(0.f, 1.f);
        Widget_GizmoButton(eGizmoOp::Translation, m_tranlationIcon);
        ImGui::SameLine(0.f, 1.f);
        Widget_GizmoButton(eGizmoOp::Rotation, m_rotatationIcon);
        ImGui::SameLine(0.f, 1.f);
        Widget_GizmoButton(eGizmoOp::Scale, m_scaleIcon);

        // gizmo orient (mtx/local) toggle
        ImGui::SameLine();
        Widget_GizmoOrientButton(eGizmoOrient::World);
        ImGui::SameLine(0.f, 1.f);
        Widget_GizmoOrientButton(eGizmoOrient::Local);

        // snap toggle
        ImGui::SameLine();
        Widget_GizmoSnapButton();

        // Play/Stop button
        ImGui::SameLine();
        Widget_EditorModeSwitchButton();
    }

    // Render viewport
    {
        if (ImGui::BeginChild("RenderViewPanel", ImGui::GetContentRegionAvail()))
        {
            // check resized
            const ImVec2 avail  = ImGui::GetContentRegionAvail();
            const Int32  width  = static_cast<Int32>(avail.x);
            const Int32  height = static_cast<Int32>(avail.y);

            // not resize when dragging with mouse
            if ((m_width != width || m_height != height) && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                m_resizeWidth      = width;
                m_resizeHeight     = height;
                m_bResizeRequested = true;
            }

            // draw scene
            if (JAM_IS_VALID_HANDLE(m_renderTexh))
            {
                m_sceneViewLeftTop = ImGui::GetCursorScreenPos();
                m_sceneViewRes     = avail;

                // draw scene
                Widget_Image(m_renderTexh, avail);

                // edit mode overlay
                if (Editor::GetMode() == eEditorMode::Edit)
                {
                    DrawGizmo_();
                    Widget_CameraAcroball_();   // on right top
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
    if (Editor::GetMode() == eEditorMode::Edit)
    {
        if (SceneManager::HasActiveScene())
        {
            const Ref<Scene>& pScene = SceneManager::GetActiveScene();
            const Entity      entity = Editor::GetFocusedEntity();

            // 생성
            if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N))
            {
                Editor::CreateEntity(pScene);
            }

            // 복제
            if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_D) && Editor::IsClonableEntity(pScene, entity))
            {
                Editor::CloneEntity(pScene, entity);
            }

            // 삭제
            if (ImGui::Shortcut(ImGuiKey_Delete) && Editor::IsDestroyableEntity(pScene, entity))
            {
                Editor::DestroyEntity(pScene, entity);
            }

            // 씬 저장
            if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S))
            {
                Editor::SaveSceneBySaveDialog(pScene);
            }
        }

        // 씬 열기
        if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O))
        {
            Editor::LoadSceneByOpenDialog();
        }
    }

    ImGui::End();
}

void Editor_SceneView::OnEvent(Event& _event)
{
    HandleEvent<SceneChangedEvent>(_event, JAM_BIND_LISTENER_METHOD(this, Editor_SceneView::OnSceneChangedEvent_));
    HandleEvent<MouseDownEvent>(_event, JAM_BIND_LISTENER_METHOD(this, Editor_SceneView::OnMouseDownEvent_));
    HandleEvent<MouseUpEvent>(_event, JAM_BIND_LISTENER_METHOD(this, Editor_SceneView::OnMouseUpEvent_));
    HandleEvent<KeyDownEvent>(_event, JAM_BIND_LISTENER_METHOD(this, Editor_SceneView::OnKeyDownEvent_));
    HandleEvent<KeyUpEvent>(_event, JAM_BIND_LISTENER_METHOD(this, Editor_SceneView::OnKeyUpEvent_));
    HandleEvent<MouseWheelEvent>(_event, JAM_BIND_LISTENER_METHOD(this, Editor_SceneView::OnMouseWheelEvent_));
}

void Editor_SceneView::Serialize(
    JsonWriter _writer) const
{
    _writer.WriteField("cameraMoveSpeed", m_cameraMoveSpeed);
    _writer.WriteField("cameraRotSpeed", m_cameraRotSpeed);
    _writer.WriteField("cameraFovYRad", m_camera.GetFovYRad());
    _writer.WriteField("cameraProjection", m_camera.GetProjection());
    _writer.WriteField("drawGrid", m_bShowGrid);
    _writer.WriteField("drawAxis", m_bShowAxis);
    _writer.WriteField("drawCameraTranslation", m_bShowCameraTranslation);
    _writer.WriteField("drawCameraForward", m_bShowCameraForward);
}

void Editor_SceneView::Deserialize(
    JsonReader _reader)
{
    float cameraFovYRad = 0.f;
    if (_reader.ReadField("cameraFovYRad", cameraFovYRad))
    {
        m_camera.SetFovYRad(cameraFovYRad);
    }

    eProjection cameraProjection = eProjection::Perspective;
    if (_reader.ReadField("cameraProjection", cameraProjection))
    {
        m_camera.SetProjection(cameraProjection);
    }

    _reader.ReadField("cameraMoveSpeed", m_cameraMoveSpeed);
    _reader.ReadField("cameraRotSpeed", m_cameraRotSpeed);
    _reader.ReadField("drawGrid", m_bShowGrid);
    _reader.ReadField("drawAxis", m_bShowAxis);
    _reader.ReadField("drawCameraTranslation", m_bShowCameraTranslation);
    _reader.ReadField("drawCameraForward", m_bShowCameraForward);
}

void Editor_SceneView::DrawGizmo_() const
{
    JAM_ASSERT(Editor::GetMode() == eEditorMode::Edit, "Gizmo can be drawn only in edit mode.");

    if (m_gizmoOp == eGizmoOp::None)
    {
        return;
    }

    Entity entity = Editor::GetFocusedEntity();
    if (!entity)
    {
        return;
    }

    Mat4       mtx  = entity.GetWorldTransform().GetMatrix();
    const Mat4 view = m_cameraTransform.ToViewMatrix();
    const Mat4 proj = m_camera.ToProjMatrix();

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
        entity.ModifyWorldTransform().SetMatrix(mtx);
    }
}

void Editor_SceneView::Menu_Options()
{
    ImGui::SeparatorText("editor camera");
    {
        const eProjection proj    = m_camera.GetProjection();
        float             fovYRad = m_camera.GetFovYRad();

        if (Widget_MenuItem("reset position"))
        {
            m_cameraTransform.SetTranslation(0.f, 0.f, 0.f);
        }

        if (Widget_MenuItem("reset rotation"))
        {
            m_pitchRad = m_yawRad = 0.f;
            m_cameraTransform.SetRotation(0.f, 0.f, 0.f);
        }

        if (Widget_MenuItem("perspective projection", 0, proj == eProjection::Perspective))
        {
            m_camera.SetProjection(eProjection::Perspective);
        }

        if (Widget_MenuItem("orthographic projection", 0, proj == eProjection::Orthographic))
        {
            m_camera.SetProjection(eProjection::Orthographic);
        }

        if (ImGui::BeginMenu("field of view"))
        {
            if (Widget_DragAngle("##CameraFov", fovYRad, 0.5f, 1.f, 179.f))
            {
                m_camera.SetFovYRad(fovYRad);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("move speed"))
        {
            Widget_DragFloat("##CameraMoveSpeed", &m_cameraMoveSpeed, 0.1f, k_cameraMoveSpeedMin);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("rotation speed"))
        {
            Widget_DragFloat("##CameraRotationSpeed", &m_cameraRotSpeed, 0.01f, k_cameraRotSpeedMin);
            ImGui::EndMenu();
        }

        Widget_MenuItem("show translation", 0, &m_bShowCameraTranslation);
        Widget_MenuItem("show forward", 0, &m_bShowCameraForward);
    }

    ImGui::SeparatorText("gizmo");
    {
        if (Widget_MenuItem("cursor", ImGuiKey_Q, m_gizmoOp == eGizmoOp::None))
        {
            m_gizmoOp = eGizmoOp::None;
        }
        else if (Widget_MenuItem("translation", ImGuiKey_W, m_gizmoOp == eGizmoOp::Translation))
        {
            m_gizmoOp = eGizmoOp::Translation;
        }
        else if (Widget_MenuItem("rotation", ImGuiKey_E, m_gizmoOp == eGizmoOp::Rotation))
        {
            m_gizmoOp = eGizmoOp::Rotation;
        }
        else if (Widget_MenuItem("scale", ImGuiKey_R, m_gizmoOp == eGizmoOp::Scale))
        {
            m_gizmoOp = eGizmoOp::Scale;
        }

        // orient
        const bool bWorldOrient = m_gizmoOrient == eGizmoOrient::World;
        if (Widget_MenuItem("world orient", bWorldOrient ? ImGuiKey_T : 0, bWorldOrient))
        {
            m_gizmoOrient = eGizmoOrient::World;
        }
        else if (Widget_MenuItem("local orient", bWorldOrient ? 0 : ImGuiKey_T, !bWorldOrient))
        {
            m_gizmoOrient = eGizmoOrient::Local;
        }

        // snap
        Widget_MenuItem("snap", ImGuiKey_Y, &m_bGizmoSnap);
    }

    ImGui::SeparatorText("view options");
    {
        Widget_MenuItem("show grid", 0, &m_bShowGrid);
        Widget_MenuItem("show axis", 0, &m_bShowAxis);
    }
}

void Editor_SceneView::OnUpdateEdit_(
    const float _deltaSec)
{
    // on mouse press
    if (Input::IsMousePressed(eMouse::RightButton) || Input::IsMousePressed(eMouse::MiddleButton))
    {
        m_dragHelper.Begin();
    }

    // on mouse release
    if (Input::IsMouseReleased(eMouse::RightButton) || Input::IsMouseReleased(eMouse::MiddleButton))
    {
        // release both buttons to cancel capture
        if (Input::IsMouseUp(eMouse::RightButton) && Input::IsMouseUp(eMouse::MiddleButton))
        {
            m_dragHelper.End();
        }
    }

    // update drag helper
    m_dragHelper.Update();

    // right button. camera move mode. WASDQE -> move like flight. mouse drag -> rotate
    if (Input::IsMouseDown(eMouse::RightButton))
    {
        // translation
        {
            const float accel = Input::IsKeyDown(eKey::Shift) ? k_cameraMoveDecelerationFactor : 1.f;
            const float dMov  = m_cameraMoveSpeed * accel * _deltaSec;

            if (Input::IsKeyDown(eKey::A))
            {
                m_cameraTransform.AddTranslation(m_cameraTransform.GetLeft() * dMov);
            }

            if (Input::IsKeyDown(eKey::D))
            {
                m_cameraTransform.AddTranslation(m_cameraTransform.GetRight() * dMov);
            }

            if (Input::IsKeyDown(eKey::W))
            {
                m_cameraTransform.AddTranslation(m_cameraTransform.GetForward() * dMov);
            }

            if (Input::IsKeyDown(eKey::S))
            {
                m_cameraTransform.AddTranslation(m_cameraTransform.GetBackward() * dMov);
            }

            if (Input::IsKeyDown(eKey::Q))
            {
                m_cameraTransform.AddTranslation(m_cameraTransform.GetDown() * dMov);
            }

            if (Input::IsKeyDown(eKey::E))
            {
                m_cameraTransform.AddTranslation(m_cameraTransform.GetUp() * dMov);
            }
        }

        // rotation
        {
            const auto [dx, dy] = m_dragHelper.GetDragDelta();
            if (!IsZeroApprox({ dx, dy }))
            {
                const float dRot = m_cameraRotSpeed * k_dragSensitivity;
                m_yawRad         = Wrap(m_yawRad - dx * dRot, -k_pi, k_pi);
                m_pitchRad       = Clamp(m_pitchRad - dy * dRot, ToRad(-89.f), ToRad(89.f));
                m_cameraTransform.SetRotation(m_pitchRad, m_yawRad, 0.f);
            }
        }
    }

    // middle button action. move camera parallel to view plane
    if (Input::IsMouseDown(eMouse::MiddleButton))
    {
        const auto [dx, dy] = m_dragHelper.GetDragDelta();
        if (!IsZeroApprox({ dx, dy }))
        {
            const float dMov   = m_cameraMoveSpeed * k_dragSensitivity;
            const Vec3  right  = m_cameraTransform.GetRight();
            const Vec3  up     = m_cameraTransform.GetUp();
            const Vec3  dRight = right * dx * dMov;
            const Vec3  dUp    = up * -dy * dMov;
            m_cameraTransform.AddTranslation(dRight + dUp);
        }
    }

    // mouse wheel action. move forward/backward or adjust move speed
    const float dWheel = Input::GetMouseDeltaWheel();
    if (!IsZeroApprox(dWheel))
    {
        if (Input::IsMouseDown(eMouse::RightButton))
        {
            // adjust move speed
            const float dMov  = dWheel * k_wheelSensitivity * k_cameraMoveSpeedAdjustFactor;
            m_cameraMoveSpeed = Max(m_cameraMoveSpeed + dMov, k_cameraMoveSpeedMin);
        }
        else
        {
            // move forward/backward (like zoom)
            const float dMov = m_cameraMoveSpeed * dWheel * k_wheelSensitivity;
            m_cameraTransform.AddTranslation(m_cameraTransform.GetForward() * dMov);
        }
    }

    // esc to release focus
    if (Input::IsKeyPressed(eKey::Escape))
    {
        Editor::SetFocusedEntity(Entity::s_null);
    }
}

void Editor_SceneView::OnUpdateRuntime_(const float _deltaSec) const
{
    SceneManager::Update(_deltaSec);
}

void Editor_SceneView::OnRenderEdit()
{
    if (SceneManager::HasActiveScene())
    {
        const UInt16   w  = NarrowCast<UInt16>(m_width);
        const UInt16   h  = NarrowCast<UInt16>(m_height);
        const Viewport vp = { w, h };

        // setup render view
        RenderView renderView = {};
        renderView.fbh        = m_fbh;
        renderView.vp         = vp;
        renderView.eye        = m_cameraTransform.GetTranslation();
        renderView.view       = m_cameraTransform.ToViewMatrix();
        renderView.proj       = m_camera.ToProjMatrix();

        // render scene
        SceneManager::Render(renderView);

        // overlays
        {
            // lazy create (because scene depth texture may change on scene load)
            {
                const Ref<Scene>&   pScene    = SceneManager::GetActiveScene();
                const TextureHandle depthTexh = pScene->GetDepthTexture();
                if (m_sceneDepthTexh != depthTexh)
                {
                    JAM_DESTROY_FRAME_BUFFER(m_fbhWithDepth);
                    const TextureHandle attachment[] = { m_renderTexh, depthTexh };
                    m_fbhWithDepth                   = Graphics::CreateFrameBuffer(attachment);
                }
            }

            // render overlays (need depth)
            if (JAM_IS_VALID_HANDLE(m_fbhWithDepth))
            {
                renderView.fbh = m_fbhWithDepth;
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
                if (const Entity entity = Editor::GetFocusedEntity())
                {
                    // bounding box
                    if (entity.HasComponent<RenderableComponent>())
                    {
                        const RenderableComponent& rc = entity.GetComponent<RenderableComponent>();
                        if (const Ref<Mesh>& pMesh = rc.renderable.pMesh)
                        {
                            const Mat4& mtx = entity.GetWorldTransform().GetMatrix();
                            m_renderer.DrawBoundingBox(pMesh->GetBoundingBox(), mtx, k_boundingBoxCol);
                        }
                    }

                    // camera frustum
                    if (entity.HasComponent<CameraComponent>())
                    {
                        const Mat4 view = entity.GetWorldTransform().ToViewMatrix();

                        const CameraComponent& cc   = entity.GetComponent<CameraComponent>();
                        const Camera&          cmr  = cc.camera;
                        const Mat4             proj = cmr.ToProjMatrix();

                        m_renderer.DrawFrustum(view * proj, k_boundingBoxCol);
                    }
                }

                m_renderer.End();
            }
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

void Editor_SceneView::Resize_(
    const Int32 _width,
    const Int32 _height)
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

void Editor_SceneView::OnSceneChangedEvent_(
    const SceneChangedEvent& _event)
{
    if (m_bRecoverRequested && m_focusedEntityBackupUid != k_invalidUid)
    {
        const Ref<Scene>& pScene = SceneManager::GetActiveScene();

        // uid 기반 복구
        const Entity entity = pScene->GetEntity(m_focusedEntityBackupUid);
        Editor::SetFocusedEntity(entity);

        m_focusedEntityBackupUid = k_invalidUid;
        m_bRecoverRequested      = false;
    }

    m_bResizeRequested = true;
}

void Editor_SceneView::OnMouseDownEvent_(
    MouseDownEvent& _event) const
{
    if (!m_bHovering)
    {
        _event.SetHandled(true);
    }
}

void Editor_SceneView::OnMouseUpEvent_(
    MouseUpEvent& _event) const
{
    if (!m_bHovering)
    {
        _event.SetHandled(true);
    }
}

void Editor_SceneView::OnKeyDownEvent_(
    KeyDownEvent& _event) const
{
    if (!m_bFocusing)
    {
        _event.SetHandled(true);
    }
}

void Editor_SceneView::OnKeyUpEvent_(
    KeyUpEvent& _event) const
{
    if (!m_bFocusing)
    {
        _event.SetHandled(true);
    }
}

void Editor_SceneView::OnMouseWheelEvent_(
    MouseWheelEvent& _event) const
{
    if (!m_bHovering)
    {
        _event.SetHandled(true);
    }
}

bool Editor_SceneView::Widget_GizmoButton(
    const eGizmoOp           _op,
    const Ref<ImageTexture>& _pTex)
{
    constexpr const char* const k_names[]     = { "cursor", "translation", "rotation", "scale" };
    constexpr ImGuiKey          k_shortcuts[] = { ImGuiKey_Q, ImGuiKey_W, ImGuiKey_E, ImGuiKey_R };

    const bool bSelected = (m_gizmoOp == _op);
    bool       bChanged  = false;

    // disabled style
    if (bSelected)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }

    // button
    {
        ButtonDesc desc = {};
        desc.text      = k_names[IntFrom(_op)];
        desc.icon       = _pTex;
        desc.shortcut   = k_shortcuts[IntFrom(_op)];
        if (Widget_Button(desc))
        {
            m_gizmoOp = _op;
            bChanged  = true;
        }
    }

    // pop style
    if (bSelected)
    {
        ImGui::PopStyleColor();
    }

    return bChanged;
}

bool Editor_SceneView::Widget_GizmoOrientButton(
    const eGizmoOrient _orient)
{
    constexpr const char* const k_names[] = { "world", "local" };

    const bool bSelected = (m_gizmoOrient == _orient);
    bool       bChanged  = false;

    // disabled style
    if (bSelected)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }

    // button
    {
        ButtonDesc desc = {};
        desc.text      = k_names[IntFrom(_orient)];
        desc.shortcut   = ImGuiKey_T;
        if (Widget_Button(desc))
        {
            m_gizmoOrient = _orient;
            bChanged      = true;
        }
    }

    // pop style
    if (bSelected)
    {
        ImGui::PopStyleColor();
    }

    return bChanged;
}

void Editor_SceneView::Widget_GizmoSnapButton()
{
    const bool bSelected = m_bGizmoSnap;

    // button
    if (bSelected)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
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
        ImGui::TextUnformatted("gizmo snap mode (toggle with y)\n holding left shift while dragging also enables snap mode.");
        ImGui::EndTooltip();
    }
}

void Editor_SceneView::Widget_CameraAcroball_()
{
    const ImGuiStyle& style = ImGui::GetStyle();

    // draw acroball
    {
        const ImVec2 pos = {
            m_sceneViewLeftTop.x + m_sceneViewRes.x - style.WindowPadding.x,
            m_sceneViewLeftTop.y + style.WindowPadding.y
        };

        if (Widget_Acroball("Acroball", m_cameraTransform, pos, k_acroballLength))
        {
            const Vec3 euler = m_cameraTransform.ToPitchYawRoll();
            m_pitchRad       = euler.x;
            m_yawRad         = euler.y;
        }
    }

    // draw camera info
    if (m_bShowCameraTranslation || m_bShowCameraForward)
    {
        const float  rectW          = ImGui::CalcTextSize("X").x * 10.f;
        const ImVec2 p0             = { m_sceneViewLeftTop.x + m_sceneViewRes.x - style.WindowPadding.x - rectW, m_sceneViewLeftTop.y + style.WindowPadding.y * 2.f + k_acroballLength };
        ImVec2       p1             = { p0.x + rectW, p0.y + style.WindowPadding.y * 2.f };
        const float  textItemHeight = ImGui::GetTextLineHeight() * 4 + style.ItemSpacing.y;
        if (m_bShowCameraTranslation)
        {
            p1.y += textItemHeight;
        }

        if (m_bShowCameraForward)
        {
            p1.y += textItemHeight;
        }

        // background
        ImGui::GetWindowDrawList()->AddRectFilled(p0, p1, k_acroballBgCol);

        // render text
        ImVec2 textPos = p0 + style.WindowPadding;

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
    ImGui::PushStyleColor(ImGuiCol_Button, Editor::GetStyleColorU32(eEditorStyleColor::RunButtonBg));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Editor::GetStyleColorU32(eEditorStyleColor::RunButtonBgActive));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Editor::GetStyleColorU32(eEditorStyleColor::RunButtonBgHovered));

    ButtonDesc desc     = {};
    desc.text          = "play";
    desc.icon           = m_playIcon;
    desc.size           = k_modeBtnSize;
    desc.shortcut       = ImGuiKey_F5;
    const bool bClicked = Widget_Button(desc);

    ImGui::PopStyleColor(3);
    return bClicked;
}

bool Editor_SceneView::Widget_StopButton_() const
{
    ImGui::PushStyleColor(ImGuiCol_Button, Editor::GetStyleColorU32(eEditorStyleColor::EditButtonBg));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Editor::GetStyleColorU32(eEditorStyleColor::EditButtonBgActive));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Editor::GetStyleColorU32(eEditorStyleColor::EditButtonBgHovered));

    ButtonDesc desc     = {};
    desc.text          = "stop";
    desc.icon           = m_stopIcon;
    desc.size           = k_modeBtnSize;
    desc.shortcut       = ImGuiKey_F5;
    const bool bClicked = Widget_Button(desc);

    ImGui::PopStyleColor(3);
    return bClicked;
}

}   // namespace jam