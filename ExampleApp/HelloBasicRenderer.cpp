#include "pch.h"

#include "HelloBasicRenderer.h"

void HelloBasicRenderer::OnEnter()
{
    BasicRenderer::Init();

    auto [w, h] = SceneManager::GetSceneResoultion();
    CompileGraph_(w, h);

    Entity           entity  = GetRootEntity();
    CameraComponent& cmrComp = entity.CreateOrReplaceComponent<CameraComponent>();
    Camera&          cmr     = cmrComp.camera;
    cmr.SetWidth(static_cast<float>(w));
    cmr.SetHeight(static_cast<float>(h));
}

void HelloBasicRenderer::OnExit()
{
    BasicRenderer::Shutdown();
}

void HelloBasicRenderer::OnUpdate(float _deltaSec)
{
}

void HelloBasicRenderer::OnRender(const RenderView& _view)
{
    BasicRenderer::Render(_view.eye, _view.view, _view.proj, m_env);
}

void HelloBasicRenderer::OnImGuiRender()
{
}

void HelloBasicRenderer::OnResize(const Int32 _width, const Int32 _height)
{
    Resize_(_width, _height);
}

void HelloBasicRenderer::Resize_(const Int32 _width, const Int32 _height)
{
    m_fullScrennVp = Viewport { 0, 0, NarrowCast<UInt16>(_width), NarrowCast<UInt16>(_height) };
    CompileGraph_(_width, _height);

    Entity          entity  = GetRootEntity();
    CameraComponent cmrComp = entity.GetComponent<CameraComponent>();
    Camera&         cmr     = cmrComp.camera;
    cmr.SetWidth(static_cast<float>(_width));
    cmr.SetHeight(static_cast<float>(_height));
}

void HelloBasicRenderer::CompileGraph_(const Int32 _width, const Int32 _height)
{
    JAM_DESTROY_FRAME_BUFFER(m_offscreenFbh);
    JAM_DESTROY_TEXTURE(m_offscreenTh);

    JAM_DESTROY_FRAME_BUFFER(m_gbufferFbh);
    JAM_DESTROY_TEXTURE(m_albedoAoTh);
    JAM_DESTROY_TEXTURE(m_normalRoughnessMetallicTh);
    JAM_DESTROY_TEXTURE(m_additionalLightTh);
    JAM_DESTROY_TEXTURE(m_depthTh);

    BasicRenderer::ClearRenderGraph();

    // gbuffer pass
    {
        GBufferDesc gbufferResult   = BasicRenderer::CreateGBuffer(_width, _height);
        m_albedoAoTh                = gbufferResult.albedoAOTexture;
        m_normalRoughnessMetallicTh = gbufferResult.normalRoughnessMetallicTexture;
        m_additionalLightTh         = gbufferResult.additionalLightTexture;
        m_depthTh                   = gbufferResult.depthTexture;

        TextureHandle handles[] = {
            m_albedoAoTh, m_normalRoughnessMetallicTh, m_additionalLightTh, m_depthTh
        };

        m_gbufferFbh = Graphics::CreateFrameBuffer(handles);

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

        PBRGBufferPassDesc desc {
            m_gbufferFbh,
            m_fullScrennVp,
            PBRGBufferExecutor { this }
        };

        RenderPass pass = BasicRenderer::CreatePBRGBufferPass(desc);
        BasicRenderer::AddPass(pass);
    }

    GBufferDesc gbufferDesc { m_albedoAoTh, m_normalRoughnessMetallicTh, m_additionalLightTh, m_depthTh };

    // gbuffer-light
    {
        m_offscreenTh  = BasicRenderer::CreateTexture2D(_width, _height, eFormat::RGBA16_Float, eAttachmentType::RenderTarget);
        m_offscreenFbh = Graphics::CreateFrameBuffer(m_offscreenTh);

        PBRLightPassDesc desc {
            m_offscreenFbh,
            m_fullScrennVp,
            gbufferDesc
        };

        RenderPass pass = BasicRenderer::CreatePBRLightPass(desc);
        BasicRenderer::AddPass(pass);
    }

    // post process bloom
    {
    }

    // post process 
    {
        
    }

    BasicRenderer::CompileRenderGraph();
}