#include "pch.h"

#include "HelloCubeScene.h"

#include "CameraScript.h"

struct HelloCubeVertex
{
    Vec3 position = Vec3::Zero;
    RGBA color    = RGBA::k_white;

    static VertexLayout s_layout;
    static void         InitLayout()
    {
        VertexLayout layout;
        layout.Add(eVertexAttribute::Position, eAttributeFormat::Float3);
        layout.Add(eVertexAttribute::Color0, eAttributeFormat::UInt8x4);
        s_layout = std::move(layout);
    }
};
VertexLayout HelloCubeVertex::s_layout = {};

struct alignas(16) CB_HELLOCUBE
{
    Mat4 world;
    Mat4 view;
    Mat4 proj;
};

HelloCubeVertex g_helloCubeVerts[] = {
    // back (0..3)
    {  Vec3 { -1.f, -1.f, 1.f },   RGBA { 255, 0, 0 } },
    {   Vec3 { 1.f, -1.f, 1.f },   RGBA { 255, 0, 0 } },
    {    Vec3 { 1.f, 1.f, 1.f },   RGBA { 255, 0, 0 } },
    {   Vec3 { -1.f, 1.f, 1.f },   RGBA { 255, 0, 0 } },
    // front (4..7)
    { Vec3 { -1.f, -1.f, -1.f },   RGBA { 0, 255, 0 } },
    {  Vec3 { 1.f, -1.f, -1.f },   RGBA { 0, 255, 0 } },
    {   Vec3 { 1.f, 1.f, -1.f },   RGBA { 0, 255, 0 } },
    {  Vec3 { -1.f, 1.f, -1.f },   RGBA { 0, 255, 0 } },
    // left (8..11)
    { Vec3 { -1.f, -1.f, -1.f },   RGBA { 0, 0, 255 } },
    {  Vec3 { -1.f, -1.f, 1.f },   RGBA { 0, 0, 255 } },
    {   Vec3 { -1.f, 1.f, 1.f },   RGBA { 0, 0, 255 } },
    {  Vec3 { -1.f, 1.f, -1.f },   RGBA { 0, 0, 255 } },
    // right (12..15)
    {   Vec3 { 1.f, -1.f, 1.f }, RGBA { 255, 255, 0 } },
    {  Vec3 { 1.f, -1.f, -1.f }, RGBA { 255, 255, 0 } },
    {   Vec3 { 1.f, 1.f, -1.f }, RGBA { 255, 255, 0 } },
    {    Vec3 { 1.f, 1.f, 1.f }, RGBA { 255, 255, 0 } },
    // top (16..19)
    {   Vec3 { -1.f, 1.f, 1.f }, RGBA { 255, 0, 255 } },
    {    Vec3 { 1.f, 1.f, 1.f }, RGBA { 255, 0, 255 } },
    {   Vec3 { 1.f, 1.f, -1.f }, RGBA { 255, 0, 255 } },
    {  Vec3 { -1.f, 1.f, -1.f }, RGBA { 255, 0, 255 } },
    // bottom (20..23)
    { Vec3 { -1.f, -1.f, -1.f }, RGBA { 0, 255, 255 } },
    {  Vec3 { 1.f, -1.f, -1.f }, RGBA { 0, 255, 255 } },
    {   Vec3 { 1.f, -1.f, 1.f }, RGBA { 0, 255, 255 } },
    {  Vec3 { -1.f, -1.f, 1.f }, RGBA { 0, 255, 255 } }
};

// clang-format off
UInt16 g_helloCubeIndices[] = {
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

constexpr const char* g_helloCubeSource = R"(
struct VSInput
{
    float3 position : POSITION;
    uint4  color    : COLOR0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color    : COLOR0;
};

cbuffer CB_HELLOCUBE : register(b0)
{
    float4x4 world;
    float4x4 view;
    float4x4 proj;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    float4x4 wvp = mul(mul(world, view), proj);
    output.position = mul(float4(input.position, 1.0f), wvp);
    output.color = float4(input.color) / 255.0f;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
)";

void HelloCubeScene::OnEnter()
{
    // buffer create
    {
        HelloCubeVertex::InitLayout();
        m_cubeVbh = Graphics::CreateVertexBuffer(MemoryView { g_helloCubeVerts }, HelloCubeVertex::s_layout);
        m_cubeIbh = Graphics::CreateIndexBuffer(MemoryView { g_helloCubeIndices });
        m_cbh     = Graphics::CreateConstantBuffer(sizeof(CB_HELLOCUBE));
    }

    // shader compile
    {
        eErrorCode     ec;
        ShaderCompiler compiler;

        ec = compiler.CompileFromSource(g_helloCubeSource, eShader::VertexShader, "VSMain");
        JAM_ASSERT(IsOk(ec), "vertex shader compile failed");
        ShaderHandle vsh = Graphics::CreateShader(compiler.GetBytecode());

        ec = compiler.CompileFromSource(g_helloCubeSource, eShader::PixelShader, "PSMain");
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
}

void HelloCubeScene::OnExit()
{
    JAM_DESTROY_FRAME_BUFFER(m_sceneFbh);
    JAM_DESTROY_TEXTURE(m_sceneRenderTexh);
    JAM_DESTROY_TEXTURE(m_sceneDepthTexh);

    JAM_DESTROY_VERTEX_BUFFER(m_cubeVbh);
    JAM_DESTROY_INDEX_BUFFER(m_cubeIbh);
    JAM_DESTROY_PROGRAM(m_program);
    JAM_DESTROY_CONSTANT_BUFFER(m_cbh);
}

void HelloCubeScene::OnUpdate(const float _deltaSec)
{
    m_dt = _deltaSec;
    m_cubeTransform.rotate *= Quat::CreateFromYawPitchRoll(m_rotSpeed * _deltaSec, 0, 0);
}

void HelloCubeScene::OnRender(const SceneView& _view)
{
    Graphics::ClearRenderTarget(m_sceneFbh, RGBA { 0x9188EDFF });
    Graphics::ClearDepthStencil(m_sceneFbh, true, false);
    Graphics::SetFrameBuffer(m_sceneFbh);
    Graphics::SetViewport(_view.viewport);

    CB_HELLOCUBE cbData;
    cbData.world = m_cubeTransform.CreateWorld().Transpose();
    cbData.view  = _view.view.Transpose();
    cbData.proj  = _view.proj.Transpose();
    Graphics::UploadBuffer(m_cbh, MemoryView { cbData });

    Graphics::SetGfxState(GfxState::k_depthTest);
    Graphics::SetConstantBuffer(m_cbh, eShader::VertexShader, 0);
    Graphics::SetProgram(m_program);

    // draw cube
    Graphics::SetVertexBuffer(m_cubeVbh);
    Graphics::SetIndexBuffer(m_cubeIbh);
    Graphics::Submit();

    // blit to scene buffer
    TextureHandle sceneTexh = Graphics::GetFrameBufferInfo(_view.frameBufferHandle).renderTargetTextureHandles[0];
    Graphics::Blit(m_sceneRenderTexh, sceneTexh);
}

void HelloCubeScene::OnImGuiRender()
{
    ImGui::Begin("hello cube");
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

void HelloCubeScene::OnResize(const Int32 _width, const Int32 _height)
{
    Resize(_width, _height);
}

void HelloCubeScene::Resize(const Int32 _width, const Int32 _height)
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