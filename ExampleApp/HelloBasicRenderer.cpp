#include "pch.h"

#include "HelloBasicRenderer.h"

#include "CameraScript.h"

void HelloBasicRenderer::OnEnter()
{
    BasicRenderer::Init();

    // setup camera
    {
        Entity           entity  = GetRootEntity();
        CameraComponent& cmrComp = entity.CreateComponent<CameraComponent>();
        cmrComp.bMain            = true;

        TransformHierarchy& t = entity.GetTransform();
        t.SetWorldTranslation(0.f, 0.f, -5.f);

        ScriptComponent& srtComp = entity.CreateComponent<ScriptComponent>();
        srtComp.BindScript<CameraScript>(entity);
    }

    // create entity
    std::shared_ptr<Mesh>     pMesh = std::make_shared<Mesh>(CreateBoxMesh({ 1.f, 1.f, 1.f }, VertexStaticMesh::s_layout));
    std::shared_ptr<Material> pMat  = std::make_shared<Material>();

    {
        Entity        entity = CreateEntity();
        MeshRenderer& mr     = entity.CreateComponent<MeshRenderer>();
        mr.pMesh             = pMesh;
        mr.pMaterial         = pMat;
    }

    // setup environment
    m_env.skyboxType      = eSkyboxType::Cubemap;
    m_env.toneMappingType = eToneMappingType::Linear;

    const ProjectDirectories& projDirs        = Application::GetInstance().GetProjectDirectories();
    const fs::path&           imageTextureDir = projDirs.imageTexturesDir;

    eErrorCode ec;
    m_env.specularMap = std::make_shared<ImageTexture>();
    ec                = m_env.specularMap->Load(imageTextureDir / "skySpecularHDR.dds");
    if (IsError(ec))
    {
        Log::Error("Failed to load specular map texture from path: {}. Error code: {}, Message: {}", (imageTextureDir / "skySpecularHDR.dds").string(), static_cast<Int32>(ec), GetErrorMessage());
        JAM_ASSERT(0, "Failed to load specular map texture!");
    }

    m_env.irradianceMap = std::make_shared<ImageTexture>();
    ec                  = m_env.irradianceMap->Load(imageTextureDir / "skyDiffuseHDR.dds");
    if (IsError(ec))
    {
        Log::Error("Failed to load irradiance map texture from path: {}. Error code: {}, Message: {}", (imageTextureDir / "skyDiffuseHDR.dds").string(), static_cast<Int32>(ec), GetErrorMessage());
        JAM_ASSERT(0, "Failed to load irradiance map texture!");
    }

    m_env.brdfLUT = std::make_shared<ImageTexture>();
    ec            = m_env.brdfLUT->Load(imageTextureDir / "skyBrdf.dds");
    if (IsError(ec))
    {
        Log::Error("Failed to load brdf LUT texture from path: {}. Error code: {}, Message: {}", (imageTextureDir / "skyBrdf.dds").string(), static_cast<Int32>(ec), GetErrorMessage());
        JAM_ASSERT(0, "Failed to load brdf LUT texture!");
    }

    m_env.skyboxTexture = std::make_shared<ImageTexture>();
    ec                  = m_env.skyboxTexture->Load(imageTextureDir / "skyEnvHDR.dds");
    if (IsError(ec))
    {
        Log::Error("Failed to load brdf LUT texture from path: {}. Error code: {}, Message: {}", (imageTextureDir / "skyBrdf.dds").string(), static_cast<Int32>(ec), GetErrorMessage());
        JAM_ASSERT(0, "Failed to load brdf LUT texture!");
    }

    // call resize
    auto [w, h] = SceneManager::GetSceneResoultion();
    Resize_(w, h);
}

void HelloBasicRenderer::OnExit()
{
    JAM_DESTROY_TEXTURE(m_offscreenTh);
    JAM_DESTROY_FRAME_BUFFER(m_offscreenFbh);
    JAM_DESTROY_FRAME_BUFFER(m_offscreenWithDepthFbh);

    JAM_DESTROY_FRAME_BUFFER(m_gbufferFbh);
    JAM_DESTROY_TEXTURE(m_albedoAoTh);
    JAM_DESTROY_TEXTURE(m_normalRoughnessTh);
    JAM_DESTROY_TEXTURE(m_emissiveMetallicTh);
    JAM_DESTROY_TEXTURE(m_depthTh);

    BasicRenderer::Shutdown();
}

void HelloBasicRenderer::OnUpdate(const float _deltaSec)
{
    m_dt = _deltaSec;
}

void HelloBasicRenderer::OnRender(const RenderView& _view)
{
    if (m_bNeedEnvironment)
    {
        BasicRenderer::UploadEnvironmentData(m_env);
        m_bNeedEnvironment = false;
    }

    BasicRenderer::Render(_view);
}

void HelloBasicRenderer::OnImGuiRender()
{
    if (!ImGui::Begin("hello basic renderer"))
    {
        ImGui::End();
        return;
    }

    ImGui::Text("delta time: %.3f ms", m_dt * 1000.f);
    ImGui::Text("fps: %.1f", 1.f / m_dt);

    ImGui::Separator();
    Entity              root = GetRootEntity();
    TransformHierarchy& th   = root.GetTransform();

    Vec3 cmrPos = th.GetWorldTranslation();
    ImGui::Text("camera position: (%.3f, %.3f, %.3f)", cmrPos.x, cmrPos.y, cmrPos.z);

    Vec3 euler = th.GetWorldPitchYawRoll();
    euler      = ToDeg(euler);
    ImGui::Text("camera rotation: (pitch: %.3f, yaw: %.3f, roll: %.3f)", euler.x, euler.y, euler.z);

    ImGui::SeparatorText("environmnet");

    constexpr const char* toneMappingTypes[] = {
        "Linear ",
        "Reinhard",
        "LumaBasedReinhard",
        "WhitePreservingReinhard",
        "RomBinDaHouse",
        "Uncharted2",
        "Filmic",
        "None"
    };

    Int32 itemCount = EnumCount<eToneMappingType>();
    if (ImGui::Combo("tone mapping", reinterpret_cast<int*>(&m_env.toneMappingType), toneMappingTypes, itemCount))
        m_bNeedEnvironment = true;

    if (ImGui::DragFloat("exposure", &m_env.exposure, 0.01f))
        m_bNeedEnvironment = true;

    if (ImGui::DragFloat("gamma", &m_env.gamma, 0.01f))
        m_bNeedEnvironment = true;

    ImGui::End();
}

void HelloBasicRenderer::OnResize(const Int32 _width, const Int32 _height)
{
    Resize_(_width, _height);
}

void HelloBasicRenderer::Resize_(const Int32 _width, const Int32 _height)
{
    m_fullScrennVp = Viewport { 0, 0, NarrowCast<UInt16>(_width), NarrowCast<UInt16>(_height) };
    CompileGraph_(_width, _height);

    Entity           entity  = GetRootEntity();
    CameraComponent& cmrComp = entity.GetComponent<CameraComponent>();
    Camera&          cmr     = cmrComp.camera;
    cmr.SetWidth(static_cast<float>(_width));
    cmr.SetHeight(static_cast<float>(_height));

    m_env.screenWidth  = static_cast<float>(_width);
    m_env.screenHeight = static_cast<float>(_height);
    m_bNeedEnvironment = true;
}

void HelloBasicRenderer::CompileGraph_(const Int32 _width, const Int32 _height)
{
    JAM_PROFILE_SCOPE_SEC("HelloBasicRenderer::CompileGraph_");

    JAM_DESTROY_TEXTURE(m_offscreenTh);
    JAM_DESTROY_FRAME_BUFFER(m_offscreenFbh);
    JAM_DESTROY_FRAME_BUFFER(m_offscreenWithDepthFbh);

    JAM_DESTROY_FRAME_BUFFER(m_gbufferFbh);
    JAM_DESTROY_TEXTURE(m_albedoAoTh);
    JAM_DESTROY_TEXTURE(m_normalRoughnessTh);
    JAM_DESTROY_TEXTURE(m_emissiveMetallicTh);
    JAM_DESTROY_TEXTURE(m_depthTh);

    BasicRenderer::ClearRenderGraph();

    // create offscreen target
    {
        m_offscreenTh  = BasicRenderer::CreateTexture2D(_width, _height, eFormat::RGBA16_Float, eAttachmentType::RenderTarget);
        m_offscreenFbh = Graphics::CreateFrameBuffer(m_offscreenTh);
    }

    // gbuffer pass
    {
        GBufferDesc gbufferResult   = BasicRenderer::CreateGBuffer(_width, _height);
        m_albedoAoTh                = gbufferResult.albedoAoTexture;
        m_normalRoughnessTh = gbufferResult.normalRoughnessTexture;
        m_emissiveMetallicTh         = gbufferResult.emissiveMetallicTexture;
        m_depthTh                   = gbufferResult.depthTexture;

        TextureHandle handles[] = { m_albedoAoTh, m_normalRoughnessTh, m_emissiveMetallicTh, m_depthTh };
        m_gbufferFbh            = Graphics::CreateFrameBuffer(handles);

        struct PBRGBufferExecutor
        {
            void operator()() const
            {
                for (const auto&& [_, tc, mr]: pScene->Each<TransformComponent, MeshRenderer>())
                {
                    const TransformHierarchy& th   = tc.transform;
                    const Transform&          wt   = th.GetWorld();
                    const Mesh&               mesh = *mr.pMesh.get();
                    const Material&           mat  = *mr.pMaterial.get();
                    BasicRenderer::Submit(mesh, mat, wt);
                }
            }

            Scene* pScene = nullptr;
        };

        PBRGBufferPassDesc desc { m_gbufferFbh, m_fullScrennVp, PBRGBufferExecutor { this } };
        RenderPass         pass = BasicRenderer::CreatePBRGBufferPass(desc);

        constexpr RGBA k_zero = RGBA {0,0,0,0};
        pass.AddClearRenderTargetCommand(k_zero, 0);
        pass.AddClearRenderTargetCommand(k_zero, 1);
        pass.AddClearRenderTargetCommand(k_zero, 2);
        pass.AddClearDepthStencilCommand(true, true, 1.f, 0);
        BasicRenderer::AddPass(std::move(pass));
    }

    GBufferDesc gbufferDesc { m_albedoAoTh, m_normalRoughnessTh, m_emissiveMetallicTh, m_depthTh };

    // gbuffer-light
    if (m_env.irradianceMap && m_env.specularMap && m_env.brdfLUT)
    {
        PBRLightPassDesc desc {
            m_offscreenFbh,
            m_fullScrennVp,
            gbufferDesc,
            m_env.irradianceMap->GetHandle(),
            m_env.specularMap->GetHandle(),
            m_env.brdfLUT->GetHandle(),
        };
        RenderPass pass = BasicRenderer::CreatePBRLightPass(desc);
        BasicRenderer::AddPass(std::move(pass));
    }

    // skybox pass
    {
        // offscreen texture + depth
        TextureHandle handles[] = { m_offscreenTh, m_depthTh };
        m_offscreenWithDepthFbh = Graphics::CreateFrameBuffer(handles);

        SkyboxPassDesc desc {
            m_offscreenWithDepthFbh,
            m_fullScrennVp,
            m_env.skyboxTexture->GetHandle()
        };

        RenderPass pass = BasicRenderer::CreateSkyboxPass(desc);
        BasicRenderer::AddPass(std::move(pass));
    }

    // post process
    {
        ToneMappingPassDesc desc;
        desc.width        = _width;
        desc.height       = _height;
        desc.inputTexture = m_offscreenTh;

        auto [pass, _] = BasicRenderer::CreatePostProcessToneMappingPass(desc);
        BasicRenderer::AddPass(std::move(pass));
    }

    BasicRenderer::CompileRenderGraph();
}