#include "pch.h"
#include "DebugNormalScene.h"

#include "../JamEngine/AssimpImporter.h"

struct CBDebugNormalScene_NoWIT
{
    static constexpr Int32 kSlot = 0;

    Mtx4 world = Mtx4::kIdentity;
};

struct CBDebugNormalScene_WithWIT
{
    static constexpr Int32 kSlot = 0;

    Mtx4 world         = Mtx4::kIdentity;
    Mtx4 invTransWorld = Mtx4::kIdentity;
};

struct CBDebugNormalSceneCamera
{
    static constexpr Int32 kSlot    = 1;
    Mtx4                   viewProj = Mtx4::kIdentity;
    Vec3                   eyePos   = Vec3::kZero;
    float                  padding  = 0.f;
};

void DebugNormalScene::InitMeshes_()
{
    // mesh
    VertexLayout attribLayout = {};
    attribLayout.Push(eVertexAttribute::Normal, eAttributeFormat::Float3);

    // sphere mesh
    m_pSphere = CreateSphereMesh(0.5f, 5, attribLayout);
    m_sphereTrans.SetTranslationX(-1.f);

    // bunny mesh
    Expect<AssimpImporter> importer = AssimpImporter::ImportFromFile("Resources/Models/StanfordBunnyPBR/StanfordBunnyPBR.gltf", eAssimpImporterFlags::GenerateNormals);
    JAM_ASSERT(importer.HasValue(), "Failed to load bunny model: {}", importer.GetError().ToErrorMessage());
    m_pBunny = importer->ConvertToModel(attribLayout, eAssimpConvertToModelFlags::Mesh).GetValue()->GetMeshes()[0];
    m_bunnyTrans.SetTranslationX(1.f);
    m_bunnyTrans.SetScale(0.01f);

    // full screen tri
    {
        Vec3 positions[] = {
            { -1.f, -1.f, 0.f },
            { -1.f,  3.f, 0.f },
            {  3.f, -1.f, 0.f }
        };

        UInt16 indices[] = { 0, 1, 2 };

        m_pFsTri = Mesh::Load(positions, indices);
    }
}

void DebugNormalScene::InitRenderGraph_()
{
    // frame buffer
    m_graph.AddTexture2D("T_SceneColor", 1.f, eTextureFormat::RGBA8_UNorm, 1, eTextureCreateFlags::RenderTargetAttachment);
    m_graph.AddTexture2D("T_SceneDepth", 1.f, eTextureFormat::D24_UNorm_S8_UInt, 1, eTextureCreateFlags::DepthStencilAttachment);

    StringView attachments[] = { "T_SceneColor", "T_SceneDepth" };
    m_graph.AddFrameBuffer("FB_Scene", attachments);

    // cbuffer
    m_graph.AddConstantBuffer("CB_NoWIT", sizeof(CBDebugNormalScene_NoWIT));
    m_graph.AddConstantBuffer("CB_WithWIT", sizeof(CBDebugNormalScene_WithWIT));
    m_graph.AddConstantBuffer("CB_Camera", sizeof(CBDebugNormalSceneCamera));
    m_noWitCbh   = m_graph.GetConstantBuffer("CB_NoWIT");
    m_withWitCbh = m_graph.GetConstantBuffer("CB_WithWIT");
    m_cmrCbh     = m_graph.GetConstantBuffer("CB_Camera");

    // program
    AddProgram_VP_(
        "P_NoWIT",
        "Resources/Misc/DebugNormalScene/DebugNormalSceneNoWITShader.hlsl");

    AddProgram_VP_(
        "P_WithWIT",
        "Resources/Misc/DebugNormalScene/DebugNormalSceneWithWITShader.hlsl");

    AddProgram_VPG_(
        "P_DrawNormalNoWIT",
        "Resources/Misc/DebugNormalScene/DebugNormalSceneDrawNormalNoWITShader.hlsl");

    AddProgram_VPG_(
        "P_DrawNormalWithWIT",
        "Resources/Misc/DebugNormalScene/DebugNormalSceneDrawNormalWithWITShader.hlsl");

    AddProgram_VP_(
        "P_PassThrough",
        "Resources/Misc/DebugNormalScene/DebugNormalScenePassThroughShader.hlsl");

    // pass
    m_graph.AddPass("Pass_NoWIT")
        .SetFrameBuffer("FB_Scene")
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f), 0)
        .SetDepthClearCommand(1.f)
        .SetViewportFromFrameBufferResolution()
        .SetProgram("P_NoWIT")
        .SetRenderFlags(eRenderFlags::DepthFunc_Less | eRenderFlags::DepthWrite)
        .SetBlendFlags(eBlendFlags::WriteMask_All, 0)
        .SetConstantBuffer("CB_NoWIT", eShader::VertexShader, CBDebugNormalScene_NoWIT::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, CBDebugNormalSceneCamera::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, CBDebugNormalSceneCamera::kSlot)
        .SetExecutor(Bind<&DebugNormalScene::RenderMeshes_>(this))
        .SetEnabled(!m_bUseWit);

    m_graph.AddPass("Pass_WithWIT")
        .SetFrameBuffer("FB_Scene")
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f), 0)
        .SetDepthClearCommand(1.f)
        .SetViewportFromFrameBufferResolution()
        .SetProgram("P_WithWIT")
        .SetRenderFlags(eRenderFlags::DepthFunc_Less | eRenderFlags::DepthWrite)
        .SetBlendFlags(eBlendFlags::WriteMask_All, 0)
        .SetConstantBuffer("CB_WithWIT", eShader::VertexShader, CBDebugNormalScene_WithWIT::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, CBDebugNormalSceneCamera::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, CBDebugNormalSceneCamera::kSlot)
        .SetExecutor(Bind<&DebugNormalScene::RenderMeshes_>(this))
        .SetEnabled(m_bUseWit);

    m_graph.AddPass("Pass_DrawNormalNoWIT")
        .SetFrameBuffer("FB_Scene")
        .SetViewportFromFrameBufferResolution()
        .SetProgram("P_DrawNormalNoWIT")
        .SetRenderFlags(eRenderFlags::Primitive_PointList | eRenderFlags::DepthFunc_LessEqual | eRenderFlags::DepthWrite)
        .SetBlendFlags(eBlendFlags::WriteMask_All, 0)
        .SetConstantBuffer("CB_NoWIT", eShader::GeometryShader, CBDebugNormalScene_NoWIT::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::GeometryShader, CBDebugNormalSceneCamera::kSlot)
        .SetExecutor(Bind<&DebugNormalScene::RenderMeshes_>(this))
        .SetEnabled(!m_bUseWit && m_bDrawNormal);

    m_graph.AddPass("Pass_DrawNormalWithWIT")
        .SetFrameBuffer("FB_Scene")
        .SetViewportFromFrameBufferResolution()
        .SetProgram("P_DrawNormalWithWIT")
        .SetRenderFlags(eRenderFlags::Primitive_PointList | eRenderFlags::DepthFunc_LessEqual | eRenderFlags::DepthWrite)
        .SetBlendFlags(eBlendFlags::WriteMask_All, 0)
        .SetConstantBuffer("CB_WithWIT", eShader::GeometryShader, CBDebugNormalScene_WithWIT::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::GeometryShader, CBDebugNormalSceneCamera::kSlot)
        .SetExecutor(Bind<&DebugNormalScene::RenderMeshes_>(this))
        .SetEnabled(m_bUseWit && m_bDrawNormal);

    m_graph.AddPass("Pass_PassThrough")
        .SetAsFinalPass()
        .SetUseExternalOutput()
        .SetProgram("P_PassThrough")
        .SetBlendFlags(eBlendFlags::WriteMask_All, 0)
        .SetTexture("T_SceneColor", eShader::PixelShader, 0)
        .SetSampler(eSamplerFlags::Filter_MinLinear_MagLinear_MipLinear | eSamplerFlags::AddressU_Clamp | eSamplerFlags::AddressV_Clamp, eShader::PixelShader, 0)
        .SetExecutor(Bind<&DebugNormalScene::RenderFullScreenTri_>(this));
}

void DebugNormalScene::OnResize(
    const Int32 _width,
    const Int32 _height)
{
    m_camera.cmr.SetAspectRatio(static_cast<float>(_width) / static_cast<float>(_height));
}

DebugNormalScene::DebugNormalScene()
{
    // camera
    m_camera.trans.SetTranslationZ(-5.f);

    InitMeshes_();
    InitRenderGraph_();
}

void DebugNormalScene::OnUpdate(
    const float _dt)
{
    if (m_bAnimate)
    {
        m_sphereTrans.AddRotationYaw(_dt * ToRad(45.f) * m_dAnim);
        m_bunnyTrans.AddRotationYaw(_dt * ToRad(45.f) * m_dAnim);
    }

    m_camera.Update(_dt);
}

void DebugNormalScene::OnRender(
    const RenderView& _view)
{
    Graphics& gfx = GetGraphics();

    // update camera
    CBDebugNormalSceneCamera cb = {};
    cb.viewProj                 = (m_camera.CalcView() * m_camera.GetProj()).Transpose();
    cb.eyePos                   = m_camera.GetEyePos();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(cb));

    m_graph.Execute(_view.fbh, _view.vp);
}
void DebugNormalScene::OnImGuiRender()
{
    if (ImGui::Begin("Debug Normal Scene"))
    {
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Separator();

        // 1. 애니메이션 설정
        ImGui::Checkbox("Animate", &m_bAnimate);
        ImGui::SliderFloat("Animation Speed", &m_dAnim, 0.f, 5.f);

        ImGui::Separator();

        // 2. 렌더 그래프 패스 설정
        bool bWitChanged    = ImGui::Checkbox("Use World Inverse Transpose", &m_bUseWit);
        bool bNormalChanged = ImGui::Checkbox("Draw Normals", &m_bDrawNormal);

        if (bWitChanged || bNormalChanged)
        {
            m_graph.ModifyPass("Pass_NoWIT")
                .SetEnabled(!m_bUseWit);

            m_graph.ModifyPass("Pass_WithWIT")
                .SetEnabled(m_bUseWit);

            m_graph.ModifyPass("Pass_DrawNormalNoWIT")
                .SetEnabled(!m_bUseWit && m_bDrawNormal);

            m_graph.ModifyPass("Pass_DrawNormalWithWIT")
                .SetEnabled(m_bUseWit && m_bDrawNormal);
        }

        ImGui::Separator();

        // 3. 메쉬 트랜스폼 제어 (람다를 사용하여 중복 로직 처리)
        auto editTransform = [](const char* _label, Transform& _trans) {
            if (ImGui::TreeNode(_label))
            {
                // Translation
                Vec3 pos = _trans.GetTranslation();
                if (ImGui::DragFloat3("Position", &pos.x, 0.1f))
                {
                    _trans.SetTranslation(pos);
                }

                // Rotation (Euler Degree로 변환하여 조작)
                Vec3 eulerRad = _trans.ToEuler();
                Vec3 eulerDeg = { ToDeg(eulerRad.x), ToDeg(eulerRad.y), ToDeg(eulerRad.z) };
                if (ImGui::DragFloat3("Rotation", &eulerDeg.x, 1.f, -180.f, 180.f))
                {
                    _trans.SetRotation(ToRad(eulerDeg.x), ToRad(eulerDeg.y), ToRad(eulerDeg.z));
                }

                // Scale
                Vec3 scale = _trans.GetScale();
                if (ImGui::DragFloat3("Scale", &scale.x, 0.01f, 0.001f, 10.f))
                {
                    _trans.SetScale(scale);
                }

                ImGui::TreePop();
            }
        };

        editTransform("Sphere Transform", m_sphereTrans);
        editTransform("Bunny Transform", m_bunnyTrans);

        ImGui::Separator();

        // 4. 카메라 정보
        if (ImGui::TreeNode("Camera Info"))
        {
            Vec3 pos = m_camera.trans.GetTranslation();
            ImGui::Text("Pos: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

void DebugNormalScene::AddProgram_VP_(
    const StringView _name,
    const StringView _path)
{
    constexpr ShaderCompilerInfo kVsInfo = { eShader::VertexShader, "VSMain", {}, eShaderCompileFlags::Default };
    constexpr ShaderCompilerInfo kPsInfo = { eShader::PixelShader, "PSMain", {}, eShaderCompileFlags::Default };

    const ShaderCompiler vsc = ShaderCompiler::CompileHLSLFile(_path, kVsInfo);
    const ShaderCompiler psc = ShaderCompiler::CompileHLSLFile(_path, kPsInfo);
    JAM_ASSERT(vsc.IsSucceeded(), "Vertex shader compilation failed: {}", vsc.GetErrorMessage());
    JAM_ASSERT(psc.IsSucceeded(), "Pixel shader compilation failed: {}", psc.GetErrorMessage());

    const String vsName = String::Formatted("VS_{}", _name);
    const String psName = String::Formatted("PS_{}", _name);

    m_graph.AddShader(vsName, vsc.GetBytecode());
    m_graph.AddShader(psName, psc.GetBytecode());
    m_graph.AddProgram(_name, vsName, psName);
}

void DebugNormalScene::AddProgram_VPG_(
    StringView _name,
    StringView _path)
{
    constexpr ShaderCompilerInfo kVsInfo = { eShader::VertexShader, "VSMain", {}, eShaderCompileFlags::Default };
    constexpr ShaderCompilerInfo kPsInfo = { eShader::PixelShader, "PSMain", {}, eShaderCompileFlags::Default };
    constexpr ShaderCompilerInfo kGsInfo = { eShader::GeometryShader, "GSMain", {}, eShaderCompileFlags::Default };

    const ShaderCompiler vsc = ShaderCompiler::CompileHLSLFile(_path, kVsInfo);
    const ShaderCompiler psc = ShaderCompiler::CompileHLSLFile(_path, kPsInfo);
    const ShaderCompiler gsc = ShaderCompiler::CompileHLSLFile(_path, kGsInfo);
    JAM_ASSERT(vsc.IsSucceeded(), "Vertex shader compilation failed: {}", vsc.GetErrorMessage());
    JAM_ASSERT(psc.IsSucceeded(), "Pixel shader compilation failed: {}", psc.GetErrorMessage());
    JAM_ASSERT(gsc.IsSucceeded(), "Geometry shader compilation failed: {}", gsc.GetErrorMessage());

    const String vsName = String::Formatted("VS_{}", _name);
    const String psName = String::Formatted("PS_{}", _name);
    const String gsName = String::Formatted("GS_{}", _name);

    m_graph.AddShader(vsName, vsc.GetBytecode());
    m_graph.AddShader(psName, psc.GetBytecode());
    m_graph.AddShader(gsName, gsc.GetBytecode());
    m_graph.AddProgram(_name, vsName, psName, gsName, {}, {});
}

void DebugNormalScene::RenderMeshes_() const
{
    UpdateEntityConstants_(m_sphereTrans);
    m_pSphere->Submit();

    UpdateEntityConstants_(m_bunnyTrans);
    m_pBunny->Submit();
}

void DebugNormalScene::RenderFullScreenTri_() const
{
    m_pFsTri->Submit();
}

void DebugNormalScene::UpdateEntityConstants_(
    const Transform& _trans) const
{
    Graphics& gfx = GetGraphics();

    if (m_bUseWit)
    {
        CBDebugNormalScene_WithWIT cb = {};
        cb.world                      = _trans.GetSRT().Transpose();
        cb.invTransWorld              = _trans.CalcInvSRT();
        gfx.UpdateBuffer(m_withWitCbh, AsMemoryView(cb));
    }
    else
    {
        CBDebugNormalScene_NoWIT cb = {};
        cb.world                    = _trans.GetSRT().Transpose();
        gfx.UpdateBuffer(m_noWitCbh, AsMemoryView(cb));
    }
}