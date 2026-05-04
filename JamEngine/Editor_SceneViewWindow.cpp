#include "pch.h"

#include "Editor_SceneViewWindow.h"

#include "CameraComponent.h"
#include "Editor.h"
#include "EditorWidgets.h"
#include "Event.h"
#include "Events.h"
#include "Graphics.h"
#include "Input.h"
#include "JsonArchive.h"
#include "RenderableComponent.h"
#include "SceneManager.h"
#include "EditorUtilities.h"

namespace Jam
{

Editor_SceneViewWindow::Editor_SceneViewWindow()
    : m_infiniteDragger(eInfiniteDraggerFlags::Right | eInfiniteDraggerFlags::Middle)
{
    // init editor camera
    {
        m_cameraTransform.SetTranslation(-5, 5, -5);
        m_cameraTransform.SetLookAt(Vec3::kZero, Vec3::kUp);
        m_camera.SetFarZ(300.f);
    }
}

Editor_SceneViewWindow::~Editor_SceneViewWindow()
{
    JAM_DESTROY_TEXTURE(m_renderTexh);
    JAM_DESTROY_FRAME_BUFFER(m_fbh);
    JAM_DESTROY_FRAME_BUFFER(m_fbhWithDepth);
}

void Editor_SceneViewWindow::OnUpdate(const float _deltaTime)
{
    if (GetEditor().GetMode() == eEditorMode::Play)
    {
        OnUpdateRuntime_(_deltaTime);
    }
    else if (GetEditor().GetMode() == eEditorMode::Edit)
    {
        OnUpdateEdit_(_deltaTime);
    }
}

void Editor_SceneViewWindow::OnRender()
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
    const eEditorMode mode = GetEditor().GetMode();
    if (mode == eEditorMode::Play)
    {
        OnRenderRuntime();
    }
    else if (mode == eEditorMode::Edit)
    {
        OnRenderEdit();
    }
}

void Editor_SceneViewWindow::OnImGuiRender(
    bool& _bShow)
{
    // begin window
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;
    if (GetEditor().GetMode() == eEditorMode::Play)
    {
        flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    }

    ImGui::SetNextWindowSizeConstraints({ 200.f, 200.f }, { Limits<float>::kMax, Limits<float>::kMax });
    m_bVisible = ImGui::Begin("scene view", &_bShow, flags);
    if (!m_bVisible)
    {
        ImGui::End();
        return;
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
        Widget_IconButton("options", GetEditor().GetIcon(eEditorIcon::Menu));
        if (ImGui::BeginPopupContextItem(0, ImGuiPopupFlags_MouseButtonLeft))
        {
            Menu_Options();
            ImGui::EndPopup();
        }

        // Gizmo m_mode selector
        ImGui::SameLine();
        Widget_GizmoSelector(eGizmoOp::None);
        ImGui::SameLine(0.f, 1.f);
        Widget_GizmoSelector(eGizmoOp::Translation);
        ImGui::SameLine(0.f, 1.f);
        Widget_GizmoSelector(eGizmoOp::Rotation);
        ImGui::SameLine(0.f, 1.f);
        Widget_GizmoSelector(eGizmoOp::Scale);

        // gizmo orient (mtx/local) toggle
        ImGui::SameLine();
        Widget_GizmoOriendToggle_();

        // snap toggle
        ImGui::SameLine();
        Widget_GizmoSnapToggle_();

        // Play/Stop button
        ImGui::SameLine();
        Widget_EditorModeSwitchButton_();
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
            if (m_renderTexh)
            {
                m_sceneViewLeftTop = ImGui::GetCursorScreenPos();
                m_sceneViewRes     = avail;

                // draw scene
                Widget_Image(m_renderTexh, avail);

                // edit m_mode overlay
                if (GetEditor().GetMode() == eEditorMode::Edit)
                {
                    DrawGizmo_();
                    Widget_CameraAcroball_();   // on right top
                }
            }
            else
            {
                Widget_TextFrame("cannot render scene view", avail);
            }
        }
        ImGui::EndChild();
    }

    // short cut in edit m_mode
    if (GetEditor().GetMode() == eEditorMode::Edit)
    {
        if (GetSceneManager().HasActiveScene())
        {
            const Entity entity = GetEditor().GetFocusingEntityOrNull();

            // 생성
            if (ImGui::Shortcut(GetEditor().GetStyleShortCut(eEditorShortCut::CreateEntity)))
            {
                GetEditor().CreateEntity(m_pScene);
            }

            // 복제
            if (ImGui::Shortcut(GetEditor().GetStyleShortCut(eEditorShortCut::CloneEntity)) && GetEditor().CanEntityClone(entity))
            {
                GetEditor().CloneEntity(m_pScene, entity);
            }

            // 삭제
            if (ImGui::Shortcut(GetEditor().GetStyleShortCut(eEditorShortCut::DeleteEntity)) && GetEditor().CanEntityDestroy(entity))
            {
                GetEditor().DestroyEntity(m_pScene, entity);
            }

            // 씬 저장
            if (ImGui::Shortcut(GetEditor().GetStyleShortCut(eEditorShortCut::SaveScene)))
            {
                GetEditor().SaveSceneByFileDialog(m_pScene);
            }
        }

        // 씬 열기
        if (ImGui::Shortcut(GetEditor().GetStyleShortCut(eEditorShortCut::LoadScene)))
        {
            GetEditor().LoadScenByFileDialog();
        }
    }

    ImGui::End();
}

void Editor_SceneViewWindow::OnEvent(Event& _event)
{
    EventDispatcher dispatcher { _event };
    dispatcher.Dispatch<SceneChangedEvent>(this, &Editor_SceneViewWindow::OnSceneChangedEvent_);
    dispatcher.Dispatch<MouseDownEvent>(this, &Editor_SceneViewWindow::OnMouseDownEvent_);
    dispatcher.Dispatch<MouseUpEvent>(this, &Editor_SceneViewWindow::OnMouseUpEvent_);
    dispatcher.Dispatch<KeyDownEvent>(this, &Editor_SceneViewWindow::OnKeyDownEvent_);
    dispatcher.Dispatch<KeyUpEvent>(this, &Editor_SceneViewWindow::OnKeyUpEvent_);
    dispatcher.Dispatch<MouseWheelEvent>(this, &Editor_SceneViewWindow::OnMouseWheelEvent_);
}

void Editor_SceneViewWindow::WriteToArchive(
    ArchiveWriter& _writer) const
{
    _writer.WriteField("cameraMoveSpeed", m_cameraMoveSpeed);
    _writer.WriteField("cameraRotSpeed", m_cameraRotSpeed);
    _writer.WriteField("drawGrid", m_bShowGrid);
    _writer.WriteField("drawAxis", m_bShowAxis);
    _writer.WriteField("drawCameraTranslation", m_bShowCameraTranslation);
    _writer.WriteField("drawCameraForward", m_bShowCameraForward);
}

Error Editor_SceneViewWindow::ReadFromArchive(
    ArchiveReader& _reader)
{
    float fovYRad = 0.f;
    if (const Error err = _reader.ReadField("cameraFovYRad", fovYRad))
    {
        return err;
    }
    m_camera.SetFovYRad(fovYRad);

    eProjection proj = {};
    if (const Error err = _reader.ReadField("cameraProjection", proj))
    {
        return err;
    }
    m_camera.SetProjection(proj);

    if (const Error err = _reader.ReadField("cameraMoveSpeed", m_cameraMoveSpeed))
    {
        return err;
    }

    if (const Error err = _reader.ReadField("cameraRotSpeed", m_cameraRotSpeed))
    {
        return err;
    }

    if (const Error err = _reader.ReadField("drawGrid", m_bShowGrid))
    {
        return err;
    }

    if (const Error err = _reader.ReadField("drawAxis", m_bShowAxis))
    {
        return err;
    }

    if (const Error err = _reader.ReadField("drawCameraTranslation", m_bShowCameraTranslation))
    {
        return err;
    }

    if (const Error err = _reader.ReadField("drawCameraForward", m_bShowCameraForward))
    {
        return err;
    }

    return Error::None;
}

void Editor_SceneViewWindow::DrawGizmo_() const
{
    JAM_ASSERT(GetEditor().GetMode() == eEditorMode::Edit, "Gizmo can be drawn only in edit m_mode.");

    if (m_gizmoOp == eGizmoOp::None)
    {
        return;
    }

    Entity entity = GetEditor().GetFocusingEntityOrNull();
    if (!entity)
    {
        return;
    }

    Mtx4       mtx  = entity.GetWorldTransform().GetSRT();
    const Mtx4 view = mtx.Inverse();
    const Mtx4 proj = m_camera.GetProj();

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
    if (m_bGizmoSnap || GetInput().IsKeyDown(eKey::Shift))
    {
        pSnap = m_gizmoOp == eGizmoOp::Translation ? &kTranslateSnap.x
              : m_gizmoOp == eGizmoOp::Rotation    ? &kRotateSnapDeg.x
                                                   : &kScaleSnap.x;
    }

    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
    ImGuizmo::SetOrthographic(bIsOrtho);
    ImGuizmo::SetRect(x, y, width, height);
    if (ImGuizmo::Manipulate(&view.m[0][0], &proj.m[0][0], gizmoOp, gizmoMode, &mtx.m[0][0], nullptr, pSnap))
    {
        entity.ModifyWorldTransform().SetSRT(mtx);
    }
}

void Editor_SceneViewWindow::Menu_Options()
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
            m_cameraEulerRad = Vec3::kZero;
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
            Widget_DragFloat("##CameraMoveSpeed", m_cameraMoveSpeed, 0.1f, kCameraMoveSpeedMin);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("rotation speed"))
        {
            Widget_DragFloat("##CameraRotationSpeed", m_cameraRotSpeed, 0.01f, kCameraRotSpeedMin);
            ImGui::EndMenu();
        }

        Widget_MenuItem("show translation", 0, &m_bShowCameraTranslation);
        Widget_MenuItem("show forward", 0, &m_bShowCameraForward);
    }

    ImGui::SeparatorText("gizmo");
    {
        if (Widget_MenuItem("cursor", GetEditor().GetStyleShortCut(eEditorShortCut::GizmoCursorMode), m_gizmoOp == eGizmoOp::None))
        {
            m_gizmoOp = eGizmoOp::None;
        }
        else if (Widget_MenuItem("translation", GetEditor().GetStyleShortCut(eEditorShortCut::GizmoTranslationMode), m_gizmoOp == eGizmoOp::Translation))
        {
            m_gizmoOp = eGizmoOp::Translation;
        }
        else if (Widget_MenuItem("rotation", GetEditor().GetStyleShortCut(eEditorShortCut::GizmoRotationMode), m_gizmoOp == eGizmoOp::Rotation))
        {
            m_gizmoOp = eGizmoOp::Rotation;
        }
        else if (Widget_MenuItem("scale", GetEditor().GetStyleShortCut(eEditorShortCut::GizmoScaleMode), m_gizmoOp == eGizmoOp::Scale))
        {
            m_gizmoOp = eGizmoOp::Scale;
        }

        // orient
        Widget_GizmoOriendToggle_();

        // snap
        Widget_MenuItem("snap", GetEditor().GetStyleShortCut(eEditorShortCut::GizmoSnapToggle), &m_bGizmoSnap);
    }

    ImGui::SeparatorText("view options");
    {
        Widget_MenuItem("show grid", 0, &m_bShowGrid);
        Widget_MenuItem("show axis", 0, &m_bShowAxis);
    }
}

void Editor_SceneViewWindow::OnUpdateEdit_(
    const float _deltaSec)
{
    // update infinite dragger
    m_infiniteDragger.Update();

    // right button. camera move m_mode. WASDQE -> move like flight. mouse drag -> rotate
    if (GetInput().IsMouseDown(eMouse::RightButton))
    {
        // translation
        {
            const float accel = GetInput().IsKeyDown(eKey::Shift) ? kCameraMoveDecelerationFactor : 1.f;
            const float dMov  = m_cameraMoveSpeed * accel * _deltaSec;

            if (GetInput().IsKeyDown(eKey::A))
            {
                m_cameraTransform.AddTranslation(m_cameraTransform.GetLeft() * dMov);
            }

            if (GetInput().IsKeyDown(eKey::D))
            {
                m_cameraTransform.AddTranslation(m_cameraTransform.GetRight() * dMov);
            }

            if (GetInput().IsKeyDown(eKey::W))
            {
                m_cameraTransform.AddTranslation(m_cameraTransform.GetForward() * dMov);
            }

            if (GetInput().IsKeyDown(eKey::S))
            {
                m_cameraTransform.AddTranslation(m_cameraTransform.GetBackward() * dMov);
            }

            if (GetInput().IsKeyDown(eKey::Q))
            {
                m_cameraTransform.AddTranslation(m_cameraTransform.GetDown() * dMov);
            }

            if (GetInput().IsKeyDown(eKey::E))
            {
                m_cameraTransform.AddTranslation(m_cameraTransform.GetUp() * dMov);
            }
        }

        // rotation
        {
            const Vec2i dd = m_infiniteDragger.GetDragDelta();
            if (dd.x != 0 || dd.y != 0)
            {
                const float dRot   = m_cameraRotSpeed * kDragSensitivity;
                m_cameraEulerRad.x = Clamp(m_cameraEulerRad.x - static_cast<float>(dd.y) * dRot, ToRad(-89.f), ToRad(89.f));
                m_cameraEulerRad.y = Wrap(m_cameraEulerRad.y - static_cast<float>(dd.x) * dRot, -kPi, kPi);
                m_cameraTransform.SetRotation(m_cameraEulerRad);
            }
        }
    }

    // middle button action. move camera parallel to view plane
    if (GetInput().IsMouseDown(eMouse::MiddleButton))
    {
        const Vec2i dd = m_infiniteDragger.GetDragDelta();
        if (dd.x != 0 || dd.y != 0)
        {
            const float dMov = m_cameraMoveSpeed * kDragSensitivity;
            const Vec3  r    = m_cameraTransform.GetRight();
            const Vec3  u    = m_cameraTransform.GetUp();
            const Vec3  dr   = r * static_cast<float>(dd.x) * dMov;
            const Vec3  du   = u * static_cast<float>(-dd.y) * dMov;
            m_cameraTransform.AddTranslation(dr + du);
        }
    }

    // mouse wheel action. move forward/backward or adjust move speed
    const float dWheel = GetInput().GetMouseDeltaWheel();
    if (!IsZeroApprox(dWheel))
    {
        if (GetInput().IsMouseDown(eMouse::RightButton))
        {
            // adjust move speed
            const float dMov  = dWheel * kWheelSensitivity * kCameraMoveSpeedAdjustFactor;
            m_cameraMoveSpeed = Max(m_cameraMoveSpeed + dMov, kCameraMoveSpeedMin);
        }
        else
        {
            // move forward/backward (like zoom)
            const float dMov = m_cameraMoveSpeed * dWheel * kWheelSensitivity;
            m_cameraTransform.AddTranslation(m_cameraTransform.GetForward() * dMov);
        }
    }

    // esc to release focus
    if (GetInput().IsKeyPressed(eKey::Escape))
    {
        GetEditor().SetFocusingEntity(Entity::kNull);
    }
}

void Editor_SceneViewWindow::OnUpdateRuntime_(const float _deltaSec) const
{
    GetSceneManager().Update(_deltaSec);
}

void Editor_SceneViewWindow::OnRenderEdit()
{
    if (GetSceneManager().HasActiveScene())
    {
        const UInt16   w  = static_cast<UInt16>(m_width);
        const UInt16   h  = static_cast<UInt16>(m_height);
        const Viewport vp = { w, h };

        // setup render view
        RenderView renderView = {};
        renderView.fbh        = m_fbh;
        renderView.vp         = vp;
        renderView.eye        = m_cameraTransform.GetTranslation();
        renderView.view       = m_cameraTransform.CalcInvSRT();
        renderView.proj       = m_camera.GetProj();

        // render scene
        GetSceneManager().Render(renderView);

        // overlays
        {
            // lazy create (because scene depth texture may change on scene load)
            {
                const TextureHandle depthTexh = m_pScene->GetDepthTexture();
                if (m_sceneDepthTexh != depthTexh)
                {
                    JAM_DESTROY_FRAME_BUFFER(m_fbhWithDepth);

                    Graphics& gfx = GetGraphics();

                    Attachment attachment[2] = {};
                    attachment[0].texh       = m_renderTexh;
                    attachment[1].texh       = depthTexh;
                    m_fbhWithDepth           = gfx.CreateFrameBuffer(attachment);
                }
            }

            // render overlays (need depth)
            if (m_fbhWithDepth)
            {
                renderView.fbh = m_fbhWithDepth;
                m_renderer.Begin(renderView);

                // draw grid and axis
                {
                    Flags<eDrawGridFlags> flags = eDrawGridFlags::None;

                    if (m_bShowGrid)
                    {
                        flags |= eDrawGridFlags::Grid;
                    }

                    if (m_bShowAxis)
                    {
                        flags |= eDrawGridFlags::Axis;
                    }

                    if (flags != eDrawGridFlags::None)
                    {
                        m_renderer.DrawGrid(flags);
                    }
                }

                // draw focused entity
                if (const Entity entity = GetEditor().GetFocusingEntityOrNull())
                {
                    // bounding box
                    if (entity.HasComponent<RenderableComponent>())
                    {
                        const RenderableComponent& rc = entity.GetComponent<RenderableComponent>();
                        if (rc.pMesh)
                        {
                            const Mtx4& mtx = entity.GetWorldTransform().GetSRT();
                            m_renderer.DrawBoundingBox(rc.pMesh->GetBoundingBox(), mtx, kBoundingBoxCol);
                        }
                    }

                    // camera frustum
                    if (entity.HasComponent<CameraComponent>())
                    {
                        const CameraComponent& cc   = entity.GetComponent<CameraComponent>();
                        const Camera&          cmr  = cc.camera;
                        const Mtx4             view = entity.GetWorldTransform().CalcInvSRT();
                        const Mtx4             proj = cmr.GetProj();

                        m_renderer.DrawFrustum(view * proj, kBoundingBoxCol);
                    }
                }

                m_renderer.End();
            }
        }
    }
}

void Editor_SceneViewWindow::OnRenderRuntime() const
{
    const UInt16   w  = static_cast<UInt16>(m_width);
    const UInt16   h  = static_cast<UInt16>(m_height);
    const Viewport vp = { w, h };
    GetSceneManager().RenderRuntime(m_fbh, vp);
}

void Editor_SceneViewWindow::Resize_(
    const Int32 _width,
    const Int32 _height)
{
    JAM_DESTROY_TEXTURE(m_renderTexh);
    JAM_DESTROY_FRAME_BUFFER(m_fbh);

    // resize current resolution
    m_width  = _width;
    m_height = _height;

    // render target texture
    m_renderTexh = GetGraphics().CreateTexture2D(_width, _height, eTextureFormat::RGBA8_UNorm, false, 1, eTextureCreateFlags::RenderTargetAttachment);
    GetGraphics().SetTextureName(m_renderTexh, "scene view texture");

    // frame buffer
    m_fbh = GetGraphics().CreateFrameBuffer(m_renderTexh);
    GetGraphics().SetFrameBufferName(m_fbh, "scene view frame buffer");

    // camera
    m_camera.SetWidth(static_cast<float>(_width));
    m_camera.SetHeight(static_cast<float>(_height));

    // resize scene manager
    GetSceneManager().Resize(_width, _height);
}

void Editor_SceneViewWindow::OnSceneChangedEvent_(
    const SceneChangedEvent&)
{
    const SceneManager& mgr = GetSceneManager();
    if (!mgr.HasActiveScene())
    {
        return;
    }

    m_pScene = mgr.GetActiveScene();

    // edit <-> play 상태 전환 시, 편집중이던 씬과 엔티티 복구
    if (m_bRecoverRequested && m_focusedEntityBackupUid)
    {
        const Entity entity = m_pScene->GetEntity(m_focusedEntityBackupUid);
        GetEditor().SetFocusingEntity(entity);

        m_focusedEntityBackupUid = UID::kNull;
        m_bRecoverRequested      = false;
    }

    m_bResizeRequested = true;
}

void Editor_SceneViewWindow::OnMouseDownEvent_(
    MouseDownEvent& _event) const
{
    if (!m_bHovering)
    {
        _event.SetHandled(true);
    }
}

void Editor_SceneViewWindow::OnMouseUpEvent_(
    MouseUpEvent& _event) const
{
    if (!m_bHovering)
    {
        _event.SetHandled(true);
    }
}

void Editor_SceneViewWindow::OnKeyDownEvent_(
    KeyDownEvent& _event) const
{
    if (!m_bFocusing)
    {
        _event.SetHandled(true);
    }
}

void Editor_SceneViewWindow::OnKeyUpEvent_(
    KeyUpEvent& _event) const
{
    if (!m_bFocusing)
    {
        _event.SetHandled(true);
    }
}

void Editor_SceneViewWindow::OnMouseWheelEvent_(
    MouseWheelEvent& _event) const
{
    if (!m_bHovering)
    {
        _event.SetHandled(true);
    }
}

void Editor_SceneViewWindow::Widget_GizmoSelector(
    const eGizmoOp _op)
{
    const eEditorShortCut editorShortcut = _op == eGizmoOp::None        ? eEditorShortCut::GizmoCursorMode
                                         : _op == eGizmoOp::Translation ? eEditorShortCut::GizmoTranslationMode
                                         : _op == eGizmoOp::Rotation    ? eEditorShortCut::GizmoRotationMode
                                                                        : eEditorShortCut::GizmoScaleMode;

    const Ref<Texture> pIcon = _op == eGizmoOp::None        ? GetEditor().GetIcon(eEditorIcon::Cursor)
                             : _op == eGizmoOp::Translation ? GetEditor().GetIcon(eEditorIcon::Translation)
                             : _op == eGizmoOp::Rotation    ? GetEditor().GetIcon(eEditorIcon::Rotation)
                                                            : GetEditor().GetIcon(eEditorIcon::Scale);

    const char* summryText = _op == eGizmoOp::None        ? "select tool"
                           : _op == eGizmoOp::Translation ? "tranlation tool"
                           : _op == eGizmoOp::Rotation    ? "rotation tool"
                                                          : "scale tool";

    const char* descText = _op == eGizmoOp::None        ? "select and move around objects"
                         : _op == eGizmoOp::Translation ? "move objects along x, y, z axis"
                         : _op == eGizmoOp::Rotation    ? "rotate objects around x, y, z axis"
                                                        : "scale objects along x, y, z axis";

    const bool          bSelected = (m_gizmoOp == _op);
    const ImGuiKeyChord shortcut  = GetEditor().GetStyleShortCut(editorShortcut);
    ImGui::SetNextItemShortcut(shortcut);

    // button
    if (!bSelected)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_FrameBg));
    }

    if (Widget_IconButton(NameOf(_op).GetPtr(), pIcon))
    {
        m_gizmoOp = _op;
    }

    if (!bSelected)
    {
        ImGui::PopStyleColor();
    }

    // desc tooltip
    Widget_ItemDescriptionTooltip(summryText, descText, shortcut);
}

void Editor_SceneViewWindow::Widget_GizmoOriendToggle_()
{
    const char* summryText = m_gizmoOrient == eGizmoOrient::World ? "world orient"
                                                                  : "local orient";

    const char* descText = m_gizmoOrient == eGizmoOrient::World ? "gizmo aligned to world axes. can toggle with t"
                                                                : "gizmo aligned to object local axes. can toggle with t";

    const ImGuiKeyChord shortcut = GetEditor().GetStyleShortCut(eEditorShortCut::GizmoOrientToggle);

    ImGui::SetNextItemShortcut(shortcut);
    Widget_EnumToggle("##GizmoOrient", m_gizmoOrient);
    Widget_ItemDescriptionTooltip(summryText, descText, shortcut);
}

void Editor_SceneViewWindow::Widget_GizmoSnapToggle_()
{
    constexpr EnumWidgetItem kItems[] = {
        {  "snap on",  true },
        { "snap off", false }
    };

    const ImGuiKeyChord shortcut = GetEditor().GetStyleShortCut(eEditorShortCut::GizmoSnapToggle);
    ImGui::SetNextItemShortcut(shortcut);

    EnumWidgetItem cur = m_bGizmoSnap ? kItems[0] : kItems[1];
    if (Widget_EnumToggle("##gizmo snap", cur, kItems))
    {
        m_bGizmoSnap = cur.value;
    }

    Widget_ItemDescriptionTooltip(
        "snap",
        "enable/disable gizmo snapping. can toggle with y",
        shortcut);
}

void Editor_SceneViewWindow::Widget_CameraAcroball_()
{
    const ImGuiStyle& style = ImGui::GetStyle();

    // draw acroball
    {
        const ImVec2 pos = {
            m_sceneViewLeftTop.x + m_sceneViewRes.x - style.WindowPadding.x,
            m_sceneViewLeftTop.y + style.WindowPadding.y
        };

        if (Widget_Acroball("Acroball", m_cameraTransform, pos, kAcroballLength))
        {
            m_cameraEulerRad = m_cameraTransform.ToEuler();
        }
    }

    // draw camera info
    if (m_bShowCameraTranslation || m_bShowCameraForward)
    {
        const float  rectW          = ImGui::CalcTextSize("X").x * 10.f;
        const ImVec2 p0             = { m_sceneViewLeftTop.x + m_sceneViewRes.x - style.WindowPadding.x - rectW, m_sceneViewLeftTop.y + style.WindowPadding.y * 2.f + kAcroballLength };
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
        ImGui::GetWindowDrawList()->AddRectFilled(p0, p1, kAcroballBgCol);

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

void Editor_SceneViewWindow::Widget_RunButton_()
{
    // hotkey
    const ImGuiKeyChord shortcut = GetEditor().GetStyleShortCut(eEditorShortCut::RunGameToggle);
    ImGui::SetNextItemShortcut(shortcut);

    // button
    {
        ImGui::PushStyleColor(ImGuiCol_Button, GetEditor().GetStyleColorU32(eEditorStyleColor::RunButtonBg));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, GetEditor().GetStyleColorU32(eEditorStyleColor::RunButtonBgActive));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GetEditor().GetStyleColorU32(eEditorStyleColor::RunButtonBgHovered));

        if (Widget_IconButton("play", GetEditor().GetIcon(eEditorIcon::Play), kModeBtnSize))
        {
            GetEditor().SetMode(eEditorMode::Play);

            // 현재 씬을 직렬화하여 백업
            JsonWriter writer = {};
            GetSceneManager().GetActiveScene()->Serialize(writer);
            Expect<String> sceneData = writer.Stringify();
            JAM_ASSERT(sceneData, "failed to serialize scene on run!");
            m_backUpSceneData = sceneData.Take();

            // 현재 포커스된 엔티티 UID 백업
            const Entity entity      = GetEditor().GetFocusingEntityOrNull();
            m_focusedEntityBackupUid = entity.GetUID();

            // 씬이 변경되었을때 복구 처리 필요
            m_bRecoverRequested = true;
        }

        ImGui::PopStyleColor(3);
    }

    // tooltip
    Widget_ItemDescriptionTooltip(
        "run scene",
        "switch to play m_mode and run the scene",
        shortcut);
}

void Editor_SceneViewWindow::Widget_StopButton_() const
{
    // hotkey
    const ImGuiKeyChord shortcut = GetEditor().GetStyleShortCut(eEditorShortCut::RunGameToggle);
    ImGui::SetNextItemShortcut(shortcut);

    // button
    {
        ImGui::PushStyleColor(ImGuiCol_Button, GetEditor().GetStyleColorU32(eEditorStyleColor::EditButtonBg));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, GetEditor().GetStyleColorU32(eEditorStyleColor::EditButtonBgActive));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GetEditor().GetStyleColorU32(eEditorStyleColor::EditButtonBgHovered));

        if (ImGui::Button("stop", kModeBtnSize))
        {
            JAM_ASSERT(!m_backUpSceneData.IsEmpty(), "backup scene data is not empty on stop!");
            GetEditor().SetMode(eEditorMode::Edit);

            // 씬 역직렬화
            Expect<JsonReader> reader = JsonReader::Parse(m_backUpSceneData);
            JAM_ASSERT(reader, "failed to parse backup scene data on stop!");

            /*Expect<Ref<Scene>> m_pScene = DeserializeScene(reader.GetValue());
            JAM_ASSERT(m_pScene, "failed to deserialize scene on stop!");
            GetSceneManager().LoadSceneFromFile(m_pScene.GetValue());*/
        }

        ImGui::PopStyleColor(3);
    }

    ImGui::PopStyleColor(3);

    // tooltip
    Widget_ItemDescriptionTooltip(
        "stop scene",
        "stop the scene and switch to edit m_mode",
        shortcut);
}

void Editor_SceneViewWindow::Widget_EditorModeSwitchButton_()
{
    if (GetEditor().GetMode() == eEditorMode::Edit)
    {
        Widget_RunButton_();
    }
    else if (GetEditor().GetMode() == eEditorMode::Play)
    {
        Widget_StopButton_();
    }
}

}   // namespace Jam