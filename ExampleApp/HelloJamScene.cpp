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
constexpr RGBA k_background = RGBA::FromUNorm(0.16f, 0.18f, 0.21f);

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
    uint4 color    : COLOR0;
    float2 texCoord : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color    : COLOR0;
    float2 texCoord : TEXCOORD0;
};

cbuffer CB_HELLO_JAM_TRANSFORM : register(b0)
{
    float4x4 world;
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

    float4x4 wvp = mul(world, mul(view, proj));
    output.position = mul(float4(input.position, 1.0f), wvp);
    output.color.r = input.color.r / 255.0;
    output.color.g = input.color.g / 255.0;
    output.color.b = input.color.b / 255.0;
    output.color.a = input.color.a / 255.0; 
    output.texCoord = input.texCoord;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return (bTextureMapping == 1) ? t_texture.Sample(s_sampler, input.texCoord) : input.color;
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
    ShaderHandle vs = CompileShader(k_helloJamSource, eShader::VertexShader, "VSMain");
    ShaderHandle ps = CompileShader(k_helloJamSource, eShader::PixelShader, "PSMain");
    m_program       = Graphics::CreateProgram(vs, ps, true);

    // cbuffer
    m_transformCbh = Graphics::CreateConstantBuffer(sizeof(CB_HELLO_JAM_TRANSFORM));
    m_globalCbh    = Graphics::CreateConstantBuffer(sizeof(CB_HELLO_JAM_GLOBAL));

    // texture
    auto [ec, tex] = Graphics::LoadTexture(R"(contents\assets\Imagetextures\kokona.png)");
    if (IsError(ec))
    {
        Log::Error("Graphics::LoadTexture failed: {}", GetErrorMessage());
        JAM_ASSERT(0, "Graphics::LoadTexture failed: {}", EnumToString(ec));
    }
    m_cubeTh = tex;

    // cube mesh
    HelloJamVertex::InitLayout();
    m_vbh = Graphics::CreateVertexBuffer(MemoryView { k_helloJamCubeVerts }, HelloJamVertex::s_layout);
    m_ibh = Graphics::CreateIndexBuffer(MemoryView { k_helloJamCubeIndices }, eIndexFormat::U16);

    auto [w, h] = SceneManager::GetSceneResoultion();
    Resize(w, h);

    // camera
    {
        m_cmrEntt = CreateEntity("main camera");

        CameraComponent& cmrCmp = m_cmrEntt.CreateComponent<CameraComponent>();
        cmrCmp.width            = static_cast<float>(w);
        cmrCmp.height           = static_cast<float>(h);
        cmrCmp.bMain            = true;

        ScriptComponent& scrtCmp = m_cmrEntt.CreateComponent<ScriptComponent>();
        scrtCmp.BindScript<CameraScript>(m_cmrEntt);

        TransformHierarchy& trans = m_cmrEntt.GetTransform();
        trans.SetWorldTranslation(13.5, 13.5, -50);
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

void HelloJamScene::OnUpdate(const float _deltaSec)
{
    m_dt = _deltaSec;
}

void HelloJamScene::OnRender(const RenderView& _view)
{
    Graphics::SetFrameBuffer(m_fbh);
    Graphics::ClearRenderTarget(m_fbh, k_background);
    Graphics::ClearDepthStencil(m_fbh, true, false);
    Graphics::SetViewport(_view.viewport);

    Graphics::SetProgram(m_program);
    Graphics::SetConstantBuffer(m_transformCbh, eShader::VertexShader, 0);
    Graphics::SetConstantBuffer(m_globalCbh, eShader::VertexShader, 1);
    Graphics::SetConstantBuffer(m_globalCbh, eShader::PixelShader, 1);

    Graphics::SetGfxState(m_gfxState);
    Graphics::SetBlendState(m_blendState, 0);

    CB_HELLO_JAM_GLOBAL global;
    global.view            = _view.view.Transpose();
    global.proj            = _view.proj.Transpose();
    global.bTextureMapping = m_bTextureMapping ? 1 : 0;
    Graphics::UploadBuffer(m_globalCbh, MemoryView { global });

    constexpr float k_internal = 3.f;
    static float    s_time     = 0.f;

    if (m_bAnimation)
        s_time += m_dt;

    for (int y = 0; y < m_cubeCountPerRow; ++y)
    {
        for (int x = 0; x < m_cubeCountPerRow; ++x)
        {
            Vec3 pos {
                static_cast<float>(x) * k_internal,
                static_cast<float>(y) * k_internal,
                0.f
            };

            float yaw   = s_time * m_rotSpeedRad + ToRad(15.f) * static_cast<float>(x + y);
            float pitch = ToRad(15.f);

            CB_HELLO_JAM_TRANSFORM transformData;
            transformData.world = CreateWorld(pos, Quat::CreateFromYawPitchRoll(yaw, pitch, 0), Vec3::One).Transpose();
            Graphics::UploadBuffer(m_transformCbh, MemoryView { transformData });

            Graphics::SetTexture(m_cubeTh, eShader::PixelShader, 0);
            Graphics::SetSampler(SamplerState::k_linearClamp, eShader::PixelShader, 0);

            Graphics::SetVertexBuffer(m_vbh);
            Graphics::SetIndexBuffer(m_ibh);
            Graphics::Submit();
        }
    }

    TextureHandle renderTh = Graphics::GetRenderTargetTexture(_view.frameBufferHandle, 0);
    Graphics::Blit(m_renderTh, renderTh);
}

void HelloJamScene::OnImGuiRender()
{
    ImGui::Begin("hello jam engine!");

    {
        ImGui::Text("this is a simple example scene.");
    }

    {
        ImGui::Separator();

        auto [w, h] = SceneManager::GetSceneResoultion();
        ImGui::Text("resolution: %d x %d", w, h);
        ImGui::Text("delta time: %.3f ms", m_dt * 1000.f);
        ImGui::Text("fps: %.1f", 1.f / m_dt);
    }

    {
        ImGui::Separator();

        constexpr const char* const k_primitiveLabels[EnumCount<ePrimitive>()] = {
            "triangle list",
            "triangle strip",
            "line list",
            "line strip",
            "point list",
        };

        constexpr const char* const k_cullModeLabels[EnumCount<eCullMode>()] = {
            "none",
            "back",
            "front",
        };

        constexpr const char* const k_depthTestFuncLabels[EnumCount<eCompareFunc>()] = {
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

        Int32 primitiveInt   = static_cast<Int32>(m_gfxState.primitive);
        Int32 primitiveCount = NarrowCast<Int32>(EnumCount<ePrimitive>());
        if (ImGui::Combo("primitive", &primitiveInt, k_primitiveLabels, primitiveCount))
            m_gfxState.primitive = static_cast<ePrimitive>(primitiveInt);

        Int32 cullModeInt   = static_cast<Int32>(m_gfxState.cullMode);
        Int32 cullModeCount = NarrowCast<Int32>(EnumCount<eCullMode>());
        if (ImGui::Combo("cull mode", &cullModeInt, k_cullModeLabels, cullModeCount))
            m_gfxState.cullMode = static_cast<eCullMode>(cullModeInt);

        bool bWireframe = m_gfxState.bWireframe;
        if (ImGui::Checkbox("wireframe", &bWireframe))
            m_gfxState.bWireframe = bWireframe;

        bool bDepthClamp = m_gfxState.bDepthClamp;
        if (ImGui::Checkbox("depth clamp", &bDepthClamp))
            m_gfxState.bDepthClamp = bDepthClamp;

        Int32 depthTestFuncInt   = static_cast<int>(m_gfxState.depthTestFunc);
        Int32 depthTestFuncCount = NarrowCast<Int32>(EnumCount<eCompareFunc>());
        if (ImGui::Combo("depth test func", &depthTestFuncInt, k_depthTestFuncLabels, depthTestFuncCount))
            m_gfxState.depthTestFunc = static_cast<eCompareFunc>(depthTestFuncInt);

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
        ImGui::Text("cube count: %d (%d x %d)", m_cubeCountPerRow * m_cubeCountPerRow, m_cubeCountPerRow, m_cubeCountPerRow);
        ImGui::DragInt("cube count per row", &m_cubeCountPerRow, 1, 1, k_cubeCountPerRowMax);
        ImGui::SliderAngle("rotation speed", &m_rotSpeedRad);
        ImGui::Checkbox("texture mapping", &m_bTextureMapping);
        ImGui::Checkbox("animation", &m_bAnimation);
    }

    ImGui::End();
}

void HelloJamScene::OnResize(const Int32 _width, const Int32 _height)
{
    Resize(_width, _height);
}

void HelloJamScene::Resize(const Int32 _width, const Int32 _height)
{
    JAM_DESTROY_FRAME_BUFFER(m_fbh);
    JAM_DESTROY_TEXTURE(m_renderTh);
    JAM_DESTROY_TEXTURE(m_depthTh);

    m_renderTh = Graphics::CreateTexture2D(_width, _height, eFormat::RGBA8_UNorm, 1, eTextureCreateFlags_RenderTarget | eTextureCreateFlags_WriteOnly);
    m_depthTh  = Graphics::CreateTexture2D(_width, _height, eFormat::D32_Float, 1, eTextureCreateFlags_DepthStencil | eTextureCreateFlags_WriteOnly);

    TextureHandle attachments[] = { m_renderTh, m_depthTh };
    m_fbh                       = Graphics::CreateFrameBuffer(attachments);

    if (m_cmrEntt.IsValid())
    {
        CameraComponent& cmrCmp = m_cmrEntt.GetComponent<CameraComponent>();
        cmrCmp.width            = static_cast<float>(_width);
        cmrCmp.height           = static_cast<float>(_height);
    }
}