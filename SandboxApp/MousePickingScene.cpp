#include "pch.h"
#include "MousePickingScene.h"

#include "Gui.h"
#include "ImageFilterManager.h"
#include "RenderStates.h"
#include "Utilities.h"
#include "../JamEngine/Events.h"

#define SCENE_PATH(_filepath) "Resources/MousePickingScene/" _filepath

namespace
{
struct CBEntity
{
    Mtx4  world, invTransWorld;
    Vec3  color;
    Int32 objectId;
};

struct CBSelection
{
    Int32 selectedId, showIds, outlineWidth, padding;
};
static_assert(sizeof(CBEntity) == 144 && sizeof(CBSelection) == 16);
}   // namespace

MousePickingScene::MousePickingScene()
{
    InitObjects_();
    InitRenderGraph_();
    m_camera.camera.SetFarZ(100.f);
    m_camera.transform.SetTranslation({ 0.f, 3.3f, -11.f });
    m_camera.euler.pitch = ToRad(12.f);
    m_camera.transform.SetRotation(m_camera.euler);
    m_camera.dMove = 5.f;
}

void MousePickingScene::RequestPick_(const Vec2i _clientPoint)
{
    m_pickPoint      = _clientPoint;
    m_bPickRequested = true;
}

void MousePickingScene::OnEvent(Event& _event)
{
    EventDispatcher dispatcher { _event };
    dispatcher.Dispatch<MouseDownEvent>(this, &MousePickingScene::OnMouseDown_);
}

void MousePickingScene::OnMouseDown_(const MouseDownEvent& _event)
{
    if (_event.GetVKey() != VK_LBUTTON || ImGui::GetIO().WantCaptureMouse)
        return;
    POINT point = {};
    if (::GetCursorPos(&point) && ::ScreenToClient(GetWindowsPlatform().GetPlatformHandle(), &point))
        RequestPick_({ point.x, point.y });
}

void MousePickingScene::OnUpdate(const float _deltaTime)
{
    m_camera.Update(_deltaTime);
    if (m_bAnimate)
    {
        m_time += _deltaTime;
        for (Int32 i = 0; i < m_objects.size(); ++i)
            m_objects[i].transform.SetRotation({ 0.15f, m_time * 0.35f + i * 0.3f, 0.f });
    }
}

void MousePickingScene::OnRender(const RenderView& _view)
{
    m_viewport          = _view.viewport;
    const Mtx4 viewProj = m_camera.CalcViewProj().Transpose();
    GetGraphics().UpdateBuffer(m_cameraCbh, AsMemoryView(viewProj));
    m_graph.Execute(_view.fbh, _view.viewport);
}

void MousePickingScene::OnImGuiRender()
{
    if (!BeginSceneDemoWindow("MousePickingScene"))
    {
        ImGui::End();
        return;
    }
    if (m_selectedId == 0)
        ImGui::TextUnformatted("Selected: none");
    else
        ImGui::Text("Selected ID: %d", m_selectedId);
    if (ImGui::Button("Clear selection"))
    {
        m_selectedId     = 0;
        m_bPickRequested = false;
    }
    ImGui::Checkbox("Show object IDs", &m_bShowIds);
    ImGui::SliderInt("Outline width", &m_outlineWidth, 1, 5);
    ImGui::Checkbox("Animate", &m_bAnimate);
    ImGui::TextUnformatted("LMB: select, background: clear\nWASD / QE: move, RMB drag: look");
    ImGui::End();
}

void MousePickingScene::OnResize(const Int32 _width, const Int32 _height)
{
    m_camera.SetAspectRatioFromBackBuffer();
}

void MousePickingScene::InitObjects_()
{
    VertexLayout layout = {};
    layout.Add(eVertexAttribute::Normal, eAttributeFormat::Float3);
    const Ref<Mesh> cube   = CreateCubeMesh(Vec3 { 1.6f }, layout);
    const Ref<Mesh> sphere = CreateSphereMesh(0.8f, 4, layout);
    const Ref<Mesh> bunny  = LoadMesh("Resources/Models/StanfordBunny.obj", layout, true);
    m_pFloor               = CreatePlaneMesh({ 24.f, 20.f }, Vec3::kUp, layout);

    const auto add = [&](const Ref<Mesh>& _mesh, Vec3 _position, Vec3 _scale, Vec3 _rotation, Vec3 _color) {
        Object object;
        object.pMesh = _mesh;
        object.transform.SetTranslation(_position);
        object.transform.SetScale(_scale);
        object.transform.SetRotation(_rotation);
        object.color = _color;
        m_objects.push_back(std::move(object));
    };
    add(cube, { -3.f, 1.1f, 1.f }, Vec3::kOne, { 0.2f, 0.4f, 0.f }, { 0.9f, 0.22f, 0.2f });
    add(sphere, { 0.f, 1.2f, 2.4f }, Vec3 { 1.35f }, Vec3::kZero, { 0.2f, 0.5f, 0.95f });
    add(bunny, { 3.f, -0.35f, 1.f }, Vec3 { 12.f }, Vec3::kZero, { 0.95f, 0.8f, 0.55f });
    add(cube, { -2.f, 0.95f, -2.f }, { 0.85f, 1.1f, 0.85f }, { 0.15f, 0.6f, 0.15f }, { 0.2f, 0.8f, 0.55f });
    add(sphere, { 0.f, 0.9f, -1.8f }, Vec3 { 0.9f }, Vec3::kZero, { 1.f, 0.5f, 0.12f });
    add(cube, { 2.5f, 1.f, -1.7f }, Vec3::kOne, { 0.1f, -0.4f, 0.15f }, { 0.65f, 0.3f, 0.9f });
}

void MousePickingScene::InitRenderGraph_()
{
    LoadProgram(m_graph, "P_Picking", SCENE_PATH("PickingShader.hlsl"));
    LoadProgram(m_graph, "P_Composite", SCENE_PATH("CompositeShader.hlsl"));
    m_graph.EmplaceConstantBuffer("CB_Entity", sizeof(CBEntity));
    m_graph.EmplaceConstantBuffer("CB_Camera", sizeof(Mtx4));
    m_graph.EmplaceConstantBuffer("CB_Selection", sizeof(CBSelection));
    m_entityCbh    = m_graph.GetConstantBuffer("CB_Entity");
    m_cameraCbh    = m_graph.GetConstantBuffer("CB_Camera");
    m_selectionCbh = m_graph.GetConstantBuffer("CB_Selection");

    m_graph.EmplaceTexture2D("T_Color", 1.f, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_Id", 1.f, eTextureFormat::R32_UInt, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_Depth", 1.f, eTextureFormat::D32_Float, false, 1, eTextureFlags::DepthStencil);
    m_graph.EmplaceTexture2D("T_Readback", 1, 1, eTextureFormat::R32_UInt, false, 1, eTextureFlags::Readback);
    AttachmentRG attachments[3] = {};
    attachments[0].name         = "T_Color";
    attachments[1].name         = "T_Id";
    attachments[2].name         = "T_Depth";
    m_graph.EmplaceFrameBuffer("FB_Picking", attachments);

    m_graph.AddRenderPass("Pass_Objects")
        .SetFrameBuffer("FB_Picking")
        .SetProgram("P_Picking")
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.055f, 0.075f, 0.11f, 1.f), 0)
        .SetRenderTargetClearCommand(RGBA::kTransparent, 1)
        .SetDepthClearCommand(1.f)
        .SetRenderFlags(kBackCullDepthTest)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetBlendFlags(kOpaqueBlend, 1)
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, 0)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, 1)
        .SetExecutor(Bind<&MousePickingScene::SubmitObjects_>(this));

    m_graph.AddRenderPass("Pass_Composite")
        .AsFinalPass()
        .SetProgram("P_Composite")
        .SetRenderFlags(eRenderFlags::None)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetConstantBuffer("CB_Selection", eShader::PixelShader, 0)
        .SetTexture("T_Color", eShader::PixelShader, 0)
        .SetTexture("T_Id", eShader::PixelShader, 1)
        .SetExecutor(Bind<&MousePickingScene::SubmitComposite_>(this));
}

void MousePickingScene::DrawObject_(const Mesh& _mesh, const Mtx4& _world, const Vec3 _color, const Int32 _id) const
{
    CBEntity entity      = {};
    entity.world         = _world.Transpose();
    entity.invTransWorld = _world.Inverse();
    entity.color         = _color;
    entity.objectId      = _id;
    GetGraphics().UpdateBuffer(m_entityCbh, AsMemoryView(entity));
    _mesh.Submit();
}

void MousePickingScene::SubmitObjects_() const
{
    DrawObject_(*m_pFloor, Mtx4::kIdentity, { 0.17f, 0.2f, 0.24f }, 0);
    for (Int32 i = 0; i < m_objects.size(); ++i)
    {
        const Object& object = m_objects[i];
        DrawObject_(*object.pMesh, object.transform.GetSRT(), object.color, i + 1);
    }
}

void MousePickingScene::SubmitComposite_()
{
    if (m_bPickRequested)
        ReadPick_();
    const CBSelection selection = { m_selectedId, m_bShowIds ? 1 : 0, m_outlineWidth, 0 };
    GetGraphics().UpdateBuffer(m_selectionCbh, AsMemoryView(selection));
    GetImageFilterManager().SubmitScreenTriangle();
}

void MousePickingScene::ReadPick_()
{
    m_bPickRequested = false;
    if (m_pickPoint.x < m_viewport.x || m_pickPoint.y < m_viewport.y
        || m_pickPoint.x >= m_viewport.x + m_viewport.width || m_pickPoint.y >= m_viewport.y + m_viewport.height)
        return;
    const Int32 x = m_pickPoint.x - m_viewport.x;
    const Int32 y = m_pickPoint.y - m_viewport.y;

    Graphics&           gfx       = GetGraphics();
    const TextureHandle idTexture = m_graph.GetTexture("T_Id");
    const TextureInfo   info      = gfx.GetTextureInfo(idTexture);
    const Int32         pixelX    = static_cast<Int32>(static_cast<Int64>(x) * info.width / m_viewport.width);
    const Int32         pixelY    = static_cast<Int32>(static_cast<Int64>(y) * info.height / m_viewport.height);
    const TextureHandle readback  = m_graph.GetTexture("T_Readback");
    gfx.CopyTexture(readback, 0, 0, 0, 0, 0, idTexture, 0, 0, pixelX, pixelY, 0, 1, 1, 1);
    Int32       selectedId = 0;
    const Int32 bytes      = gfx.ReadTexture(readback, 0, 0, AsMutableMemoryView(selectedId));
    if (bytes == sizeof(selectedId) && selectedId >= 0 && selectedId <= m_objects.size())
    {
        m_selectedId = selectedId;
    }
}
