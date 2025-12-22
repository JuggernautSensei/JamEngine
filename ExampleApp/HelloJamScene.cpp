#include "pch.h"

#include "HelloJamScene.h"

#include "CameraScript.h"

struct HelloJamVertex
{
    Vec3 pos;
    RGBA color;
    Vec2 texCoord;

    static VertexLayout s_layout;
    static void         InitLayout()
    {
        VertexLayout layout;
        layout.Add(eVertexAttribute::Position, eAttributeFormat::Float3);
        layout.Add(eVertexAttribute::Color0, eAttributeFormat::UInt8x4);
        layout.Add(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
        s_layout = std::move(layout);
    }
};
VertexLayout HelloJamVertex::s_layout;

constexpr RGBA k_crimson    = RGBA { 0xDC143CFF };
constexpr RGBA k_orange     = RGBA { 0xFF8C00FF };
constexpr RGBA k_gold       = RGBA { 0xFFD700FF };
constexpr RGBA k_chartreuse = RGBA { 0x7FFF00FF };
constexpr RGBA k_lime       = RGBA { 0x00FF7FFF };
constexpr RGBA k_turquoise  = RGBA { 0x40E0D0FF };
constexpr RGBA k_cyan       = RGBA { 0x00FFFFFF };
constexpr RGBA k_sky        = RGBA { 0x87CEFAFF };
constexpr RGBA k_dodger     = RGBA { 0x1E90FFFF };
constexpr RGBA k_indigo     = RGBA { 0x4B0082FF };
constexpr RGBA k_violet     = RGBA { 0xEE82EEFF };
constexpr RGBA k_fuchsia    = RGBA { 0xFF00FFFF };
constexpr RGBA k_hotPink    = RGBA { 0xFF69B4FF };
constexpr RGBA k_background = RGBA::FromLinear(0.16f, 0.18f, 0.21f);

constexpr HelloJamVertex k_helloJamCubeVerts[] = {
    { { -1.0f, -1.0f, -1.0f },    k_crimson, { 0.f, 1.f } },
    {  { -1.0f, 1.0f, -1.0f },     k_orange, { 0.f, 0.f } },
    {   { 1.0f, 1.0f, -1.0f },       k_gold, { 1.f, 0.f } },
    {  { 1.0f, -1.0f, -1.0f }, k_chartreuse, { 1.f, 1.f } },

    {   { 1.0f, -1.0f, 1.0f },       k_lime, { 0.f, 1.f } },
    {    { 1.0f, 1.0f, 1.0f },  k_turquoise, { 0.f, 0.f } },
    {   { -1.0f, 1.0f, 1.0f },       k_cyan, { 1.f, 0.f } },
    {  { -1.0f, -1.0f, 1.0f },        k_sky, { 1.f, 1.f } },

    {  { -1.0f, -1.0f, 1.0f },     k_indigo, { 0.f, 1.f } },
    {   { -1.0f, 1.0f, 1.0f },     k_violet, { 0.f, 0.f } },
    {  { -1.0f, 1.0f, -1.0f },    k_fuchsia, { 1.f, 0.f } },
    { { -1.0f, -1.0f, -1.0f },    k_hotPink, { 1.f, 1.f } },

    {  { 1.0f, -1.0f, -1.0f },    k_hotPink, { 0.f, 1.f } },
    {   { 1.0f, 1.0f, -1.0f },    k_fuchsia, { 0.f, 0.f } },
    {    { 1.0f, 1.0f, 1.0f },     k_orange, { 1.f, 0.f } },
    {   { 1.0f, -1.0f, 1.0f },       k_gold, { 1.f, 1.f } },

    {  { -1.0f, 1.0f, -1.0f },       k_cyan, { 0.f, 1.f } },
    {   { -1.0f, 1.0f, 1.0f },        k_sky, { 0.f, 0.f } },
    {    { 1.0f, 1.0f, 1.0f },     k_violet, { 1.f, 0.f } },
    {   { 1.0f, 1.0f, -1.0f },    k_fuchsia, { 1.f, 1.f } },

    {  { -1.0f, -1.0f, 1.0f },       k_lime, { 0.f, 1.f } },
    { { -1.0f, -1.0f, -1.0f }, k_chartreuse, { 0.f, 0.f } },
    {  { 1.0f, -1.0f, -1.0f },     k_orange, { 1.f, 0.f } },
    {   { 1.0f, -1.0f, 1.0f },    k_crimson, { 1.f, 1.f } }
};

// clang-format off
constexpr UInt16 k_helloJamCubeIndices[] = {
    0, 1, 2, 0, 2, 3,         // front
    4, 5, 6, 4, 6, 7,         // back
    8, 9, 10, 8, 10, 11,      // left
    12, 13, 14, 12, 14, 15,   // right
    16, 17, 18, 16, 18, 19,   // top
    20, 21, 22, 20, 22, 23    // bottom
};
// clang-format on

struct CB_HELLO_JAM_TRANSFORM
{
    Mat4 world;
};

struct CB_HELLO_JAM_GLOBAL
{
    Mat4   view;
    Mat4   proj;
    UInt32 bTextureMapping;
    float  pad[3];
};

constexpr const char* k_helloJamSource = R"(
struct VSInput
{
    float3 position : POSITION;
    uint4 radiance    : COLOR0;
    float2 texCoord : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 radiance    : COLOR0;
    float2 texCoord : TEXCOORD0;
};

cbuffer CB_HELLO_JAM_TRANSFORM : register(b0)
{
    float4x4 mtx;
};

cbuffer CB_HELLO_JAM_GLOBAL : register(b1)
{
    float4x4 view;
    float4x4 proj;
    uint     bTextureMapping;
    float3   pad;
};

Texture2D t_texture : register(t0);
SamplerState s_sampler : register(s0);

PSInput VSMain(VSInput input)
{
    PSInput output;

    float4x4 wvp = mul(mtx, mul(view, proj));
    output.position = mul(float4(input.position, 1.0f), wvp);
    output.radiance.r = input.radiance.r / 255.0;
    output.radiance.g = input.radiance.g / 255.0;
    output.radiance.b = input.radiance.b / 255.0;
    output.radiance.a = input.radiance.a / 255.0; 
    output.texCoord = input.texCoord;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return (bTextureMapping == 1) ? t_texture.Sample(s_sampler, input.texCoord) : input.radiance;
}

)";

struct CubeMeshComponent
{
    VertexBufferHandle vbh;
    IndexBufferHandle  ibh;
};

void HelloJamScene::OnEnter()
{
    // program
    const ShaderHandle vs = CompileShader(k_helloJamSource, eShader::VertexShader, "VSMain");
    const ShaderHandle ps = CompileShader(k_helloJamSource, eShader::PixelShader, "PSMain");
    m_program       = Graphics::CreateProgram(vs, ps, true);

    // cbuffer
    m_transformCbh = Graphics::CreateConstantBuffer(sizeof(CB_HELLO_JAM_TRANSFORM));
    m_globalCbh    = Graphics::CreateConstantBuffer(sizeof(CB_HELLO_JAM_GLOBAL));

    // pSkyboxTexture
    if (const std::optional<TextureHandle>& optTh = Graphics::LoadTextureFile(R"(contents\assets\Imagetextures\kokona.png)", TODO))
    {
        m_cubeTh = optTh.value();
    }
    else
    {
        Log::Error("Graphics::LoadTextureFromMemory failed: {}", GlobalBuffer::GetLastError());
        JAM_ASSERT(0, "Graphics::LoadTextureFromMemory failed: {}", GlobalBuffer::GetLastError());
    }

    // cube mesh
    HelloJamVertex::InitLayout();
    m_vbh = Graphics::CreateVertexBuffer({ k_helloJamCubeVerts }, HelloJamVertex::s_layout);
    m_ibh = Graphics::CreateIndexBuffer({ k_helloJamCubeIndices }, eIndexFormat::U16);

    // camera
    {
        Entity entity = CreateEntity("camera");

        // set camera
        entity.CreateComponent<CameraComponent>();

        // set script
        ScriptComponent& s = entity.CreateComponent<ScriptComponent>();
        s.pScript          = MakeRef<CameraScript>(entity);

        // set transformHierarchy
        SceneNode& t = entity.GetSceneNode();
        t.SetWorldTranslation(13.5, 13.5, 0.f);
    }
}

void HelloJamScene::OnExit()
{
    JAM_DESTROY_FRAME_BUFFER(m_fbh);
    JAM_DESTROY_TEXTURE(m_renderTh);
    JAM_DESTROY_TEXTURE(m_depthTh);
    JAM_DESTROY_TEXTURE(m_cubeTh);
    JAM_DESTROY_VERTEX_BUFFER(m_vbh);
    JAM_DESTROY_INDEX_BUFFER(m_ibh);
    JAM_DESTROY_PROGRAM(m_program);
    JAM_DESTROY_CONSTANT_BUFFER(m_transformCbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_globalCbh);
}

void HelloJamScene::OnRender(const RenderView& _view)
{
    Graphics::SetFrameBuffer(m_fbh);
    Graphics::ClearRenderTarget(m_fbh, k_background);
    Graphics::ClearDepthStencil(m_fbh, true, false);
    Graphics::SetViewport(_view.vp);

    Graphics::SetProgram(m_program);
    Graphics::SetConstantBuffer(m_transformCbh, eShader::VertexShader, 0);
    Graphics::SetConstantBuffer(m_globalCbh, eShader::VertexShader, 1);
    Graphics::SetConstantBuffer(m_globalCbh, eShader::PixelShader, 1);

    Graphics::SetRenderState(m_gfxState);
    Graphics::SetBlendState(m_blendState, 0);

    CB_HELLO_JAM_GLOBAL global;
    global.view            = _view.view.Transpose();
    global.proj            = _view.proj.Transpose();
    global.bTextureMapping = m_bTextureMapping ? 1 : 0;
    Graphics::UploadBuffer(m_globalCbh, { global });

    constexpr float k_internal = 3.f;
    static float    s_time     = 0.f;

    if (m_bAnimation)
        s_time += Application::GetInstance().GetDeltaTimeSec();

    for (int y = 0; y < m_numCubePerRow; ++y)
    {
        for (int x = 0; x < m_numCubePerRow; ++x)
        {
            Vec3 pos {
                static_cast<float>(x) * k_internal,
                static_cast<float>(y) * k_internal,
                50.f
            };

            float yaw   = s_time * m_rotSpeedRad + ToRad(15.f) * static_cast<float>(x + y);
            float pitch = ToRad(15.f);

            CB_HELLO_JAM_TRANSFORM transformData;
            transformData.world = CreateTransform(pos, Quat::CreateFromYawPitchRoll(yaw, pitch, 0), Vec3::One).Transpose();
            Graphics::UploadBuffer(m_transformCbh, transformData);

            Graphics::SetTexture(m_cubeTh, eShader::PixelShader, 0);
            Graphics::SetSampler(SamplerState::k_linearClamp, eShader::PixelShader, 0);

            Graphics::SetVertexBuffer(m_vbh);
            Graphics::SetIndexBuffer(m_ibh);
            Graphics::Submit();
        }
    }

    TextureHandle renderTh = Graphics::GetRenderTargetTexture(_view.fbh, 0);
    Graphics::Blit(renderTh, m_renderTh);
}

void HelloJamScene::OnImGuiRender()
{
    ImGui::Begin("hello jam engine!");

    {
        ImGui::Text("this is a simple example scene.");
    }

    {
        ImGui::Separator();

        const float dt    = ImGui::GetIO().DeltaTime;
        auto [w, h] = Graphics::GetBackBufferResolution();
        ImGui::Text("resolution: %d x %d", w, h);
        ImGui::Text("delta time: %.3f ms", dt * 1000.f);
        ImGui::Text("fps: %.1f", 1.f / dt);
    }

    {
        ImGui::Separator();

        constexpr const char* const k_primitiveLabels[NumEnums<ePrimitive>()] = {
            "triangle list",
            "triangle strip",
            "line list",
            "line strip",
            "point list",
        };

        constexpr const char* const k_cullModeLabels[NumEnums<eCullMode>()] = {
            "none",
            "back",
            "front",
        };

        constexpr const char* const k_depthTestFuncLabels[NumEnums<eCompareFunc>()] = {
            "none",
            "less",
            "less equal",
            "greater",
            "greater equal",
            "equal",
            "not equal",
            "always",
            "never",
        };

        ImGui::Text("gfx state");

        Int32 i32Prim = IntFrom(m_gfxState.primitive);
        const Int32 numPrim = NarrowCast<Int32>(NumEnums<ePrimitive>());
        if (ImGui::Combo("primitive", &i32Prim, k_primitiveLabels, numPrim))
            m_gfxState.primitive = static_cast<ePrimitive>(i32Prim);

        Int32 i32CullMode = static_cast<Int32>(m_gfxState.cullMode);
        const Int32 numCullMode = NarrowCast<Int32>(NumEnums<eCullMode>());
        if (ImGui::Combo("cull mode", &i32CullMode, k_cullModeLabels, numCullMode))
            m_gfxState.cullMode = static_cast<eCullMode>(i32CullMode);

        bool bWireframe = m_gfxState.bWireframe;
        if (ImGui::Checkbox("wireframe", &bWireframe))
            m_gfxState.bWireframe = bWireframe;

        bool bDepthClamp = m_gfxState.bDepthClamp;
        if (ImGui::Checkbox("depth clamp", &bDepthClamp))
            m_gfxState.bDepthClamp = bDepthClamp;

        Int32 i32DepthFunc = static_cast<int>(m_gfxState.depthTestFunc);
        const Int32 numDepthFunc = NarrowCast<Int32>(NumEnums<eCompareFunc>());
        if (ImGui::Combo("depth test func", &i32DepthFunc, k_depthTestFuncLabels, numDepthFunc))
            m_gfxState.depthTestFunc = static_cast<eCompareFunc>(i32DepthFunc);

        bool bDepthWrite = m_gfxState.bDepthWrite;
        if (ImGui::Checkbox("depth write", &bDepthWrite))
            m_gfxState.bDepthWrite = bDepthWrite;
    }

    {
        ImGui::Text("write mask");

        bool        bDirty    = false;
        eWriteFlags writeMask = m_blendState.writeMask;

        bDirty |= ImGui::CheckboxFlags("R", reinterpret_cast<UInt32*>(&writeMask), static_cast<UInt32>(eWriteFlags_Red));
        bDirty |= ImGui::CheckboxFlags("G", reinterpret_cast<UInt32*>(&writeMask), static_cast<UInt32>(eWriteFlags_Green));
        bDirty |= ImGui::CheckboxFlags("B", reinterpret_cast<UInt32*>(&writeMask), static_cast<UInt32>(eWriteFlags_Blue));
        bDirty |= ImGui::CheckboxFlags("A", reinterpret_cast<UInt32*>(&writeMask), static_cast<UInt32>(eWriteFlags_Alpha));
        if (bDirty)
            m_blendState.writeMask = writeMask;
    }

    {
        ImGui::Separator();
        ImGui::Text("cube count: %d (%d x %d)", m_numCubePerRow * m_numCubePerRow, m_numCubePerRow, m_numCubePerRow);
        ImGui::DragInt("cube count per row", &m_numCubePerRow, 1, 1, k_cubeCountPerRowMax);
        ImGui::SliderAngle("rotation speed", &m_rotSpeedRad);
        ImGui::Checkbox("pSkyboxTexture mapping", &m_bTextureMapping);
        ImGui::Checkbox("animation", &m_bAnimation);
    }

    ImGui::End();
}

void HelloJamScene::OnResize(const Int32 _width, const Int32 _height)
{
    JAM_DESTROY_FRAME_BUFFER(m_fbh);
    JAM_DESTROY_TEXTURE(m_renderTh);
    JAM_DESTROY_TEXTURE(m_depthTh);

    m_renderTh = Graphics::CreateTexture2D(_width, _height, eTextureFormat::RGBA8_UNorm, 1, eTextureCreateFlags_RenderTarget | eTextureCreateFlags_WriteOnly);
    m_depthTh  = Graphics::CreateTexture2D(_width, _height, eTextureFormat::D32_Float, 1, eTextureCreateFlags_DepthStencil | eTextureCreateFlags_WriteOnly);

    const TextureHandle attachments[] = { m_renderTh, m_depthTh };
    m_fbh                       = Graphics::CreateFrameBuffer(attachments);

    for (auto&& [_, cc]: Each<CameraComponent>())
    {
        Camera& c = cc.camera;
        c.SetWidth(static_cast<float>(_width));
        c.SetHeight(static_cast<float>(_height));
    }
}
