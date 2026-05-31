#include "pch.h"
#include "SimpleScene.h"

#include "Gui.h"
#include "ImageFilterManager.h"
#include "Utils.h"
#include "RenderStates.h"
#include "../JamEngine/AssimpImporter.h"

#define SCENE_PATH(_filepath) "Resources/SimpleScene/" _filepath

namespace
{

struct CBEntity
{
    static constexpr Int32 kSlot = 0;

    Mtx4  world;
    Mtx4  invTransWorld;
    Vec3  baseColor;
    float padding1;
};

struct CBCamera
{
    static constexpr Int32 kSlot = 1;

    Vec3  eye;
    float padding1;
    Mtx4  viewProj;
};

struct CBLight
{
    static constexpr Int32 kSlot = 2;

    Vec3  lightDir;
    float padding2;
    Vec3  lightColor;
    float padding3;
    Vec3  ambientColor;
    float padding4;
};

}   // namespace

SimpleScene::SimpleScene()
{
    InitMeshes_();
    InitRenderGraph_();
    m_camera.transform.SetTranslationZ(-8.f);
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

    CBCamera camera;
    camera.eye      = m_camera.GetEyePos();
    camera.viewProj = m_camera.CalcViewProj().Transpose();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(camera));

    CBLight light;
    light.lightDir     = (m_lightLookAt - m_lightPos).Normalized();
    light.lightColor   = m_lightCol;
    light.ambientColor = m_ambientCol;
    gfx.UpdateBuffer(m_lightCbh, AsMemoryView(light));

    m_graph.Execute(_view.fbh, _view.vp);
}

void SimpleScene::OnImGuiRender()
{
    if (!BeginSceneDemoWindow("SimpleScene"))
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

    // cube
    m_pCube = CreateCubeMesh(Vec3 { 0.5f }, attribLayout);
    m_cubeTrans.SetTranslationX(-4.f);

    // sphere
    m_pSphere = CreateSphereMesh(0.5f, 5, attribLayout);

    // bunny
    m_pBunny = LoadMesh("Resources/Models/StanfordBunny.obj", attribLayout);
    m_bunnyTrans.SetScale(10.f);
}

void SimpleScene::InitRenderGraph_()
{
    LoadProgram(m_graph, "P_Simple", SCENE_PATH("SimpleShader.hlsl"));
    m_graph.EmplaceConstantBuffer("CB_Entity", sizeof(CBEntity));
    m_graph.EmplaceConstantBuffer("CB_Camera", sizeof(CBCamera));
    m_graph.EmplaceConstantBuffer("CB_Light", sizeof(CBLight));
    m_graph.EmplaceTexture2D("T_SceneColor", 1.f, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_SceneDepth", 1.f, eTextureFormat::D32_Float, false, 1, eTextureFlags::DepthStencil);

    AttachmentRG attchments[2] = {};
    attchments[0].name         = "T_SceneColor";
    attchments[1].name         = "T_SceneDepth";
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

    ImageFilterManager& ifm = GetImageFilterManager();

    ImageFilterDesc blitDesc   = {};
    blitDesc.filter            = eImageFilter::Blit;
    blitDesc.output            = eImageFilterOutput::Final;
    blitDesc.input0Texture = "T_SceneColor";
    ifm.AddImageFilter(m_graph, "Pass_Blit", blitDesc);
}

void SimpleScene::Submit_() const
{
    Graphics& gfx = GetGraphics();

    // cube
    CBEntity entity;
    entity.world         = m_cubeTrans.GetSRT().Transpose();
    entity.invTransWorld = m_cubeTrans.GetSRT().Inverse();
    entity.baseColor     = m_baseCol;
    gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));
    m_pCube->Submit();

    // sphere
    entity.world         = m_sphereTrans.GetSRT().Transpose();
    entity.invTransWorld = m_sphereTrans.GetSRT().Inverse();
    entity.baseColor     = m_baseCol;
    gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));
    m_pSphere->Submit();

    // bunny
    entity.world         = m_bunnyTrans.GetSRT().Transpose();
    entity.invTransWorld = m_bunnyTrans.GetSRT().Inverse();
    entity.baseColor     = m_baseCol;
    gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));
    m_pBunny->Submit();
}

void SimpleScene::OnResize(
    const Int32 _width,
    const Int32 _height)
{
    m_camera.camera.SetAspectRatio(static_cast<float>(_width) / static_cast<float>(_height));
}