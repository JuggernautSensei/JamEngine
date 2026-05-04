#include "pch.h"
#include "SimpleScene.h"

#include "Gui.h"
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
    Graphics& gfx = GetGraphics();

    ShaderCompilerInfo vsInfo = {};
    vsInfo.entryPoint         = "VSMain";
    vsInfo.stage              = eShader::VertexShader;
    const ShaderCompiler vsc  = ShaderCompiler::CompileHLSLFile("Resources/SimpleScene/SimpleSceneShader.hlsl", vsInfo);
    JAM_ASSERT(vsc.IsSucceeded(), "Vertex shader compilation failed: {}", vsc.GetErrorMessage());
    const ShaderHandle vsh = gfx.CreateShader(vsc.GetBytecode());

    ShaderCompilerInfo psInfo = {};
    psInfo.entryPoint         = "PSMain";
    psInfo.stage              = eShader::PixelShader;
    const ShaderCompiler psc  = ShaderCompiler::CompileHLSLFile("Resources/SimpleScene/SimpleSceneShader.hlsl", psInfo);
    JAM_ASSERT(psc.IsSucceeded(), "Pixel shader compilation failed: {}", psc.GetErrorMessage());
    const ShaderHandle psh = gfx.CreateShader(psc.GetBytecode());

    m_ph = gfx.CreateProgram(vsh, psh, true);

    m_entityCbh = gfx.CreateConstantBuffer(sizeof(CBSimpleSceneEntity));
    m_cmrCbh    = gfx.CreateConstantBuffer(sizeof(CBSimpleSceneCamera));
    m_lightCbh  = gfx.CreateConstantBuffer(sizeof(CBSimpleSceneLight));

    m_camera.trans.SetTranslationZ(-8.f);

    InitMeshes_();
}

SimpleScene::~SimpleScene()
{
    JAM_DESTROY_PROGRAM(m_ph);
    JAM_DESTROY_CONSTANT_BUFFER(m_entityCbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_cmrCbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_lightCbh);
    JAM_DESTROY_FRAME_BUFFER(m_fbh);
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
    gfx.SetFrameBuffer(m_fbh);
    gfx.ClearRenderTarget(m_fbh, RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f));
    gfx.ClearDepthStencil(m_fbh, true, false, 1, 0);
    gfx.SetViewport(_view.vp);
    gfx.SetProgram(m_ph);
    gfx.SetRenderFlags(kBackCullDepthTest);
    gfx.SetBlendFlags(kOpaqueBlend, 0);

    // camera
    CBSimpleSceneCamera cbCamera = {};
    cbCamera.eye                 = m_camera.GetEyePos();
    cbCamera.viewProj            = m_camera.CalcViewProj().Transpose();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(cbCamera));
    gfx.SetConstantBuffer(m_cmrCbh, eShader::VertexShader, CBSimpleSceneCamera::kSlot);
    gfx.SetConstantBuffer(m_cmrCbh, eShader::PixelShader, CBSimpleSceneCamera::kSlot);

    // light
    CBSimpleSceneLight cbLight = {};
    cbLight.lightDir           = m_lightLookAt - m_lightPos;
    cbLight.lightDir.Normalize();
    cbLight.lightCol   = m_lightCol;
    cbLight.ambientCol = m_ambientCol;
    gfx.UpdateBuffer(m_lightCbh, AsMemoryView(cbLight));
    gfx.SetConstantBuffer(m_lightCbh, eShader::PixelShader, CBSimpleSceneLight::kSlot);

    // entity
    gfx.SetConstantBuffer(m_entityCbh, eShader::VertexShader, CBSimpleSceneEntity::kSlot);
    gfx.SetConstantBuffer(m_entityCbh, eShader::PixelShader, CBSimpleSceneEntity::kSlot);

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

    //// blit inner framebuffer texture to output texture
    //// output framebuffer maybe do not have depth texture.
    //// so. draw on screen-off framebuffer and blit the color texture to output framebuffer.
    //// in real case. use post-process pass to do this.
    //gfx.CopyTexture(fbInfo.renderTargetTexhs[0], 0, 0, 0, 0, 0, m_fbTexh, 0, 0, 0, 0, 0);
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
    attribLayout.Push(eVertexAttribute::Normal, eAttributeFormat::Float3);

    m_pCube = CreateCubeMesh(Vec3 { 0.5f }, attribLayout);
    m_cubeTrans.SetTranslationX(-4.f);

    m_pSphere = CreateSphereMesh(0.5f, 5, attribLayout);

    // load bunny
    Expect<AssimpImporter> importer = AssimpImporter::ImportFromFile("Resources/Models/StanfordBunny.obj", eAssimpImporterFlags::LoadMeshes | eAssimpImporterFlags::GenerateNormals);
    JAM_ASSERT(importer, "Failed to load model: {}", importer.GetError().ToErrorMessage());
    Expect<Ref<Model>> pModelOrNot = importer->ConvertToModel(attribLayout, eAssimpConvertToModelFlags::Mesh);
    JAM_ASSERT(pModelOrNot, "Failed to convert imported data to model: {}", pModelOrNot.GetError().ToErrorMessage());
    m_pBunny = pModelOrNot.GetValue()->GetMeshes()[0];
    m_bunnyTrans.SetScale(10.f);
}

void SimpleScene::OnResize(
    const Int32 _width,
    const Int32 _height)
{
    JAM_DESTROY_FRAME_BUFFER(m_fbh);

    // frame buffer
    Graphics& gfx = GetGraphics();
    m_fbh         = gfx.CreateFrameBuffer(_width, _height, eTextureFormat::RGBA8_UNorm, eTextureFormat::D24_UNorm_S8_UInt);

    // camera
    m_camera.cmr.SetAspectRatio(static_cast<float>(_width) / static_cast<float>(_height));

    // for blit
    const FrameBufferInfo info = gfx.GetFrameBufferInfo(m_fbh);
    JAM_ASSERT(info.numRenderTargets > 0, "Frame buffer must have at least one render target");
    m_fbTexh = info.attachments[0].texh;
}
