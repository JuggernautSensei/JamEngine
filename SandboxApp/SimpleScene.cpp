#include "pch.h"
#include "SimpleScene.h"

#include "Gui.h"
#include "PostProcessManager.h"
#include "Utils.h"
#include "RenderStates.h"
#include "../JamEngine/AssimpImporter.h"

namespace
{

struct CBSimpleSceneEntity
{
    static constexpr Int32 kSlot = 0;

    Mtx4  world;
    Mtx4  invTransWorld;
    Vec3  baseColor;
    float padding1;
};

struct CBSimpleSceneCamera
{
    static constexpr Int32 kSlot = 1;

    Vec3  eye;
    float padding1;
    Mtx4  viewProj;
};

struct CBSimpleSceneLight
{
    static constexpr Int32 kSlot = 2;

    Vec3  lightDir;
    float padding2;
    Vec3  lightCol;
    float padding3;
    Vec3  ambientCol;
    float padding4;
};

}   // namespace

SimpleScene::SimpleScene()
{
    InitMeshes_();
    InitRenderGraph_();
    m_camera.trans.SetTranslationZ(-8.f);
}

SimpleScene::~SimpleScene()
{
}

void SimpleScene::OnUpdate(
    const float _deltaTime)
{
    m_camera.Update(_deltaTime);

    if (m_bAnimate)
    {
        m_counter += _deltaTime;

        // cube. just rotate
        Vec3 cubeR  = m_cubeTrans.ToEuler();
        cubeR.pitch = m_dRot * m_counter;
        cubeR.yaw   = m_dRot * m_counter;
        m_cubeTrans.SetRotation(cubeR);

        // sphere. move round
        Vec3 sphereT = m_sphereTrans.GetTranslation();
        sphereT.x    = Cos(m_dMove * m_counter) + 4.f;
        sphereT.y    = Sin(m_dMove * m_counter);
        m_sphereTrans.SetTranslation(sphereT);

        // bunny. move up and down and rotate
        Vec3 bunnyT = m_bunnyTrans.GetTranslation();
        bunnyT.x    = 0.f;
        bunnyT.y    = Sin(m_dMove * m_counter) * 2 - 1.f;
        m_bunnyTrans.SetTranslation(bunnyT);

        Vec3 bunnyR = m_bunnyTrans.ToEuler();
        bunnyR.yaw  = m_dRot * m_counter;
        m_bunnyTrans.SetRotation(bunnyR);
    }
}

void SimpleScene::OnRender(
    const RenderView& _view)
{
    Graphics& gfx = GetGraphics();

    CBSimpleSceneCamera cbCamera = {};
    cbCamera.eye                 = m_camera.GetEyePos();
    cbCamera.viewProj            = m_camera.CalcViewProj().Transpose();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(cbCamera));

    CBSimpleSceneLight cbLight = {};
    cbLight.lightDir           = (m_lightLookAt - m_lightPos).Normalized();
    cbLight.lightCol           = m_lightCol;
    cbLight.ambientCol         = m_ambientCol;
    gfx.UpdateBuffer(m_lightCbh, AsMemoryView(cbLight));

    m_graph.Execute(_view.fbh, _view.vp);
}

void SimpleScene::OnImGuiRender()
{
    if (!BeginSceneInspectorWindows("SimpleScene"))
    {
        ImGui::End();
        return;
    }

    ImGui::End();
}

void SimpleScene::InitMeshes_()
{
    VertexLayout attribLayout = {};
    attribLayout.Add(eVertexAttribute::Normal, eAttributeFormat::Float3);

    m_pCube = CreateCubeMesh(Vec3 { 0.5f }, attribLayout);
    m_cubeTrans.SetTranslationX(-4.f);

    m_pSphere = CreateSphereMesh(0.5f, 5, attribLayout);

    // load bunny
    m_pBunny = LoadMesh("Resources/Models/StanfordBunny.obj", attribLayout);
    m_bunnyTrans.SetScale(10.f);
}

void SimpleScene::InitRenderGraph_()
{
    ProgramHandle ph = LoadProgram("Resources/SimpleScene/SimpleSceneShader.hlsl");
    m_graph.InsertProgram("P_Simple", ph, true);
    m_graph.EmplaceConstantBuffer("CB_Entity", sizeof(CBSimpleSceneEntity));
    m_graph.EmplaceConstantBuffer("CB_Camera", sizeof(CBSimpleSceneCamera));
    m_graph.EmplaceConstantBuffer("CB_Light", sizeof(CBSimpleSceneLight));
    m_graph.EmplaceTexture2D("T_SceneColor", 1.f, eTextureFormat::RGBA8_UNorm, 1, eTextureCreateFlags::RenderTargetAttachment);
    m_graph.EmplaceTexture2D("T_SceneDepth", 1.f, eTextureFormat::D32_Float, 1, eTextureCreateFlags::DepthStencilAttachment);

    AttachmentRG attchments[2];
    attchments[0].name = "T_SceneColor";
    attchments[1].name = "T_SceneDepth";
    m_graph.EmplaceFrameBuffer("FB_Scene", attchments);

    m_entityCbh = m_graph.GetConstantBuffer("CB_Entity");
    m_cmrCbh    = m_graph.GetConstantBuffer("CB_Camera");
    m_lightCbh  = m_graph.GetConstantBuffer("CB_Light");

    m_graph.AddRenderPass("Pass_Simple")
        .SetProgram("P_Simple")
        .SetFrameBuffer("FB_Scene")
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f), 0)
        .SetDepthClearCommand(1.f)
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, 1)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, 1)
        .SetConstantBuffer("CB_Light", eShader::PixelShader, 2)
        .SetRenderFlags(kBackCullDepthTest)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetExecutor(Bind<&SimpleScene::Submit_>(this));

    m_graph.AddUtilityPass("Pass_Blit")
        .SetExecutor(Bind<&SimpleScene::Submit_>(this));
}

void SimpleScene::Submit_() const
{
    Graphics&           gfx      = GetGraphics();
    CBSimpleSceneEntity cbEntity = {};

    // cube
    cbEntity.world         = m_cubeTrans.GetSRT().Transpose();
    cbEntity.invTransWorld = m_cubeTrans.GetSRT().Inverse();
    cbEntity.baseColor     = m_baseCol;
    gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cbEntity));
    m_pCube->Submit();

    // sphere
    cbEntity.world         = m_sphereTrans.GetSRT().Transpose();
    cbEntity.invTransWorld = m_sphereTrans.GetSRT().Inverse();
    cbEntity.baseColor     = m_baseCol;
    gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cbEntity));
    m_pSphere->Submit();

    // bunny
    cbEntity.world         = m_bunnyTrans.GetSRT().Transpose();
    cbEntity.invTransWorld = m_bunnyTrans.GetSRT().Inverse();
    cbEntity.baseColor     = m_baseCol;
    gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cbEntity));
    m_pBunny->Submit();
}

void SimpleScene::OnResize(
    const Int32 _width,
    const Int32 _height)
{
    m_camera.cmr.SetAspectRatio(static_cast<float>(_width) / static_cast<float>(_height));
}