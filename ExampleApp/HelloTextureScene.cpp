#include "pch.h"

#include "HelloTextureScene.h"

#include "CameraScript.h"

struct HelloTextureVertex
{
    Vec3 position = Vec3::Zero;
    Vec2 texCoord = Vec2::Zero;

    static VertexLayout s_layout;
    static void         InitLayout()
    {
        VertexLayout layout;
        layout.Add(eVertexAttribute::Position, eAttributeFormat::Float3);
        layout.Add(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
        s_layout = std::move(layout);
    }
};
VertexLayout HelloTextureVertex::s_layout = {};

struct alignas(16) CB_HELLOTEXTURE
{
    Mat4 world;
    Mat4 view;
    Mat4 proj;
};

HelloTextureVertex g_helloTextureVerts[] = {
    // back (0..3)
    {  Vec3 { -1.f, -1.f, 1.f }, Vec2 { 0.f, 1.f } },
    {   Vec3 { 1.f, -1.f, 1.f }, Vec2 { 1.f, 1.f } },
    {    Vec3 { 1.f, 1.f, 1.f }, Vec2 { 1.f, 0.f } },
    {   Vec3 { -1.f, 1.f, 1.f }, Vec2 { 0.f, 0.f } },
    // front (4..7)
    { Vec3 { -1.f, -1.f, -1.f }, Vec2 { 0.f, 1.f } },
    {  Vec3 { 1.f, -1.f, -1.f }, Vec2 { 1.f, 1.f } },
    {   Vec3 { 1.f, 1.f, -1.f }, Vec2 { 1.f, 0.f } },
    {  Vec3 { -1.f, 1.f, -1.f }, Vec2 { 0.f, 0.f } },
    // left (8..11)
    { Vec3 { -1.f, -1.f, -1.f }, Vec2 { 0.f, 1.f } },
    {  Vec3 { -1.f, -1.f, 1.f }, Vec2 { 1.f, 1.f } },
    {   Vec3 { -1.f, 1.f, 1.f }, Vec2 { 1.f, 0.f } },
    {  Vec3 { -1.f, 1.f, -1.f }, Vec2 { 0.f, 0.f } },
    // right (12..15)
    {   Vec3 { 1.f, -1.f, 1.f }, Vec2 { 0.f, 1.f } },
    {  Vec3 { 1.f, -1.f, -1.f }, Vec2 { 1.f, 1.f } },
    {   Vec3 { 1.f, 1.f, -1.f }, Vec2 { 1.f, 0.f } },
    {    Vec3 { 1.f, 1.f, 1.f }, Vec2 { 0.f, 0.f } },
    // top (16..19)
    {   Vec3 { -1.f, 1.f, 1.f }, Vec2 { 0.f, 1.f } },
    {    Vec3 { 1.f, 1.f, 1.f }, Vec2 { 1.f, 1.f } },
    {   Vec3 { 1.f, 1.f, -1.f }, Vec2 { 1.f, 0.f } },
    {  Vec3 { -1.f, 1.f, -1.f }, Vec2 { 0.f, 0.f } },
    // bottom (20..23)
    { Vec3 { -1.f, -1.f, -1.f }, Vec2 { 0.f, 1.f } },
    {  Vec3 { 1.f, -1.f, -1.f }, Vec2 { 1.f, 1.f } },
    {   Vec3 { 1.f, -1.f, 1.f }, Vec2 { 1.f, 0.f } },
    {  Vec3 { -1.f, -1.f, 1.f }, Vec2 { 0.f, 0.f } },
};

// clang-format off
UInt16 g_helloTexIndices[] = {
    // back
    0, 1, 2,
    2, 3, 0,
    // front
    4, 6, 5,
    6, 4, 7,
    // left
    8, 9, 10,
    10, 11, 8,
    // right
    12, 13, 14,
    14, 15, 12,
    // top
    16, 17, 18,
    18, 19, 16,
    // bottom
    20, 21, 22,
    22, 23, 20, 
};
// clang-format on

constexpr const char* g_helloTexSource = R"(
struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
};
struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};
cbuffer CB_HELLOTEXTURE : register(b0)
{
    float4x4 world;
    float4x4 view;
    float4x4 proj;
};

Texture2D t_texture : register(t0);
SamplerState s_sampler : register(s0);

PSInput VSMain(VSInput input)
{
    PSInput output;
    float4x4 wvp = mul(mul(world, view), proj);
    output.position = mul(float4(input.position, 1.0f), wvp);
    output.texCoord = input.texCoord;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 color = t_texture.Sample(s_sampler, input.texCoord);

    // simple tonemapping (texture is in sRGB space)
    float exposure = 1.f;
    float gamma = 2.2f;
    color.rgb = pow(abs(color.rgb * exposure), 1.f / gamma);
    return color;
}

)";

void HelloTextureScene::OnEnter()
{
    // buffer create
    {
        HelloTextureVertex::InitLayout();
        m_cubeVbh = Graphics::CreateVertexBuffer(MemoryView { g_helloTextureVerts }, HelloTextureVertex::s_layout);
        m_cubeIbh = Graphics::CreateIndexBuffer(MemoryView { g_helloTexIndices });
        m_cbh     = Graphics::CreateConstantBuffer(sizeof(CB_HELLOTEXTURE));
    }

    // shader compile
    {
        eErrorCode     ec;
        ShaderCompiler compiler;

        ec = compiler.CompileFromSource(g_helloTexSource, eShader::VertexShader, "VSMain");
        JAM_ASSERT(IsOk(ec), "vertex shader compile failed");
        ShaderHandle vsh = Graphics::CreateShader(compiler.GetBytecode());

        ec = compiler.CompileFromSource(g_helloTexSource, eShader::PixelShader, "PSMain");
        JAM_ASSERT(IsOk(ec), "pixel shader compile failed");
        ShaderHandle psh = Graphics::CreateShader(compiler.GetBytecode());
        m_program        = Graphics::CreateProgram(vsh, psh, true);
    }

    // get scene resolution
    auto [w, h] = SceneManager::GetSceneResoultion();

    // camera entity
    {
        m_cmrEntity = CreateEntity("camera");

        CameraComponent& cmr = m_cmrEntity.CreateComponent<CameraComponent>();
        cmr.bMain            = true;
        cmr.width            = static_cast<float>(w);
        cmr.height           = static_cast<float>(h);

        ScriptComponent& sc = m_cmrEntity.CreateComponent<ScriptComponent>();
        sc.BindScript<CameraScript>(m_cmrEntity);

        TransformHierarchy& t = m_cmrEntity.GetTransform();
        t.SetWorldTranslateZ(-10.f);
    }

    // resize (create frame buffer)
    Resize(w, h);

    // cube initial transform
    m_cubeTransform.rotate = Quat::CreateFromYawPitchRoll(0.f, ToRad(25.f), 0.f);

    // load texture
    Application&              app     = Application::GetInstance();
    const ProjectDirectories& projDir = app.GetProjectDirectories();
    auto [ec, tex]                    = Graphics::LoadTexture(projDir.imageTexturesDir / "naginagi.png");
    JAM_ASSERT(IsOk(ec), "load texture failed!.");
    m_texh = tex;
}

void HelloTextureScene::OnExit()
{
    JAM_DESTROY_FRAME_BUFFER(m_sceneFbh);
    JAM_DESTROY_TEXTURE(m_sceneRenderTexh);
    JAM_DESTROY_TEXTURE(m_sceneDepthTexh);

    JAM_DESTROY_VERTEX_BUFFER(m_cubeVbh);
    JAM_DESTROY_INDEX_BUFFER(m_cubeIbh);
    JAM_DESTROY_PROGRAM(m_program);
    JAM_DESTROY_CONSTANT_BUFFER(m_cbh);

    JAM_DESTROY_TEXTURE(m_texh);
}

void HelloTextureScene::OnUpdate(const float _deltaSec)
{
    m_dt = _deltaSec;
    m_cubeTransform.rotate *= Quat::CreateFromYawPitchRoll(m_rotSpeed * _deltaSec, 0, 0);
}

void HelloTextureScene::OnRender(const SceneView& _view)
{
    Graphics::ClearRenderTarget(m_sceneFbh, RGBA { 0x9188EDFF });
    Graphics::ClearDepthStencil(m_sceneFbh, true, false);
    Graphics::SetFrameBuffer(m_sceneFbh);
    Graphics::SetViewport(_view.viewport);

    CB_HELLOTEXTURE cbData;
    cbData.world = m_cubeTransform.CreateWorld().Transpose();
    cbData.view  = _view.view.Transpose();
    cbData.proj  = _view.proj.Transpose();
    Graphics::UploadBuffer(m_cbh, MemoryView { cbData });

    Graphics::SetGfxState(GfxState::k_depthTest);
    Graphics::SetConstantBuffer(m_cbh, eShader::VertexShader, 0);
    Graphics::SetProgram(m_program);

    // draw cube
    Graphics::SetTexture(m_texh, eShader::PixelShader, 0);
    Graphics::SetSampler(SamplerState::k_linearWrap, eShader::PixelShader, 0);
    Graphics::SetVertexBuffer(m_cubeVbh);
    Graphics::SetIndexBuffer(m_cubeIbh);
    Graphics::Submit();

    // blit to scene buffer
    TextureHandle sceneTexh = Graphics::GetFrameBufferInfo(_view.frameBufferHandle).renderTargetTextureHandles[0];
    Graphics::Blit(m_sceneRenderTexh, sceneTexh);
}

void HelloTextureScene::OnImGuiRender()
{
    ImGui::Begin("hello texture");
    ImGui::Text("delta time: %.3f ms", m_dt * 1000.f);
    ImGui::Text("fps: %.1f", 1.f / m_dt);

    ImGui::Separator();
    ImGui::SliderFloat3("cube position", &m_cubeTransform.translate.x, -5.f, 5.f);
    ImGui::SliderFloat3("cube scale", &m_cubeTransform.scale.x, 0.1f, 5.f);
    ImGui::SliderAngle("rotation speed", &m_rotSpeed, -360.f, 360.f);

    ImGui::Separator();
    EditCameraScriptDataComponentUI(m_cmrEntity.GetComponent<CameraScriptDataComponent>());

    ImGui::Separator();
    if (ImGui::Button("exit"))
    {
        Application& app = Application::GetInstance();
        app.Quit();
    }
    ImGui::End();
}

void HelloTextureScene::OnResize(const Int32 _width, const Int32 _height)
{
    Resize(_width, _height);
}

void HelloTextureScene::Resize(const Int32 _width, const Int32 _height)
{
    JAM_DESTROY_FRAME_BUFFER(m_sceneFbh);
    JAM_DESTROY_TEXTURE(m_sceneRenderTexh);
    JAM_DESTROY_TEXTURE(m_sceneDepthTexh);

    m_sceneRenderTexh = Graphics::CreateTexture2D(_width, _height, eFormat::RGBA8_UNorm, 1, eTextureCreateFlags_RenderTarget | eTextureCreateFlags_WriteOnly);
    m_sceneDepthTexh  = Graphics::CreateTexture2D(_width, _height, eFormat::D32_Float, 1, eTextureCreateFlags_DepthStencil | eTextureCreateFlags_WriteOnly);

    TextureHandle attachment[] = { m_sceneRenderTexh, m_sceneDepthTexh };
    m_sceneFbh                 = Graphics::CreateFrameBuffer(attachment);

    CameraComponent& cmr = m_cmrEntity.GetComponent<CameraComponent>();
    cmr.width            = static_cast<float>(_width);
    cmr.height           = static_cast<float>(_height);
}