#include "pch.h"
#include "TerrainScene.h"

#include "Gui.h"
#include "ImageFilterManager.h"
#include "Utilities.h"
#include "RenderStates.h"

#define SCENE_PATH(_filepath) "Resources/TerrainScene/" _filepath

namespace
{

struct CBTerrain
{
    static constexpr Int32 kSlot = 0;

    float terrainSize;
    float heightScale;
    float heightMapSize;
    float tileSize;
    Vec3  lightDir;
    float snowHeight;
    float rockSlopeMin;
    float rockSlopeMax;
    float snowBlend;
    float debugView;
    float dirtHeight;
    float padding[3];
};

struct CBCamera
{
    static constexpr Int32 kSlot = 1;

    Vec3  eye;
    float padding1;
    Mtx4  viewProj;
};

constexpr Int32 kHeightMapSize = 513;   // 정점과 텍셀이 1:1
constexpr Int32 kNumQuads      = kHeightMapSize - 1;
constexpr float kTerrainSize   = 1024.f;

constexpr Int32 kHeightSlot = 0;
constexpr Int32 kGrassSlot  = 1;
constexpr Int32 kRockSlot   = 2;
constexpr Int32 kSnowSlot   = 3;
constexpr Int32 kDirtSlot   = 4;

UInt32 HashCoord(
    const Int32  _x,
    const Int32  _z,
    const UInt32 _seed)
{
    UInt32 h = static_cast<UInt32>(_x) * 374761393u + static_cast<UInt32>(_z) * 668265263u + _seed * 2246822519u;
    h        = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

const Array<Vec2, 256>& GradientTable()
{
    static const Array<Vec2, 256> s_table = [] {
        Array<Vec2, 256> table = {};
        for (Int32 i = 0; i < 256; ++i)
        {
            const float angle = static_cast<float>(i) * (k2Pi / 256.f);
            table[i]          = { Cos(angle), Sin(angle) };
        }
        return table;
    }();
    return s_table;
}

float Fade(const float _t)
{
    return _t * _t * _t * (_t * (_t * 6.f - 15.f) + 10.f);
}

float GradientDot(
    const Int32  _ix,
    const Int32  _iz,
    const float  _dx,
    const float  _dz,
    const UInt32 _seed)
{
    const Vec2& gradient = GradientTable()[HashCoord(_ix, _iz, _seed) & 255u];
    return gradient.x * _dx + gradient.y * _dz;
}

// 대략 [-1, 1]
float PerlinNoise(
    const float  _x,
    const float  _z,
    const UInt32 _seed)
{
    const Int32 x0 = static_cast<Int32>(std::floor(_x));
    const Int32 z0 = static_cast<Int32>(std::floor(_z));
    const float fx = _x - static_cast<float>(x0);
    const float fz = _z - static_cast<float>(z0);

    const float n00 = GradientDot(x0, z0, fx, fz, _seed);
    const float n10 = GradientDot(x0 + 1, z0, fx - 1.f, fz, _seed);
    const float n01 = GradientDot(x0, z0 + 1, fx, fz - 1.f, _seed);
    const float n11 = GradientDot(x0 + 1, z0 + 1, fx - 1.f, fz - 1.f, _seed);

    const float u = Fade(fx);
    const float v = Fade(fz);
    return Lerp(Lerp(n00, n10, u), Lerp(n01, n11, u), v) * 1.4142f;
}

}   // namespace

TerrainScene::TerrainScene()
{
    InitTerrain_();
    InitRenderGraph_();

    m_camera.camera.SetFarZ(4000.f);
    m_camera.dMove = 200.f;
    m_camera.transform.SetTranslation({ 0.f, 320.f, -700.f });
    m_camera.euler.pitch = ToRad(20.f);
    m_camera.transform.SetRotation(m_camera.euler);
}

TerrainScene::~TerrainScene()
{
    JAM_DESTROY_VERTEX_BUFFER(m_vbh);
    JAM_DESTROY_INDEX_BUFFER(m_ibh);
    JAM_DESTROY_TEXTURE(m_heightTexh);
}

void TerrainScene::OnUpdate(
    const float _deltaTime)
{
    m_camera.Update(_deltaTime);
}

void TerrainScene::OnRender(
    const RenderView& _view)
{
    Graphics& gfx = GetGraphics();

    CBCamera camera;
    camera.eye      = m_camera.GetEyePos();
    camera.viewProj = m_camera.CalcViewProj().Transpose();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(camera));

    CBTerrain terrain     = {};
    terrain.terrainSize   = kTerrainSize;
    terrain.heightScale   = m_heightScale;
    terrain.heightMapSize = static_cast<float>(kHeightMapSize);
    terrain.tileSize      = m_tileSize;
    terrain.lightDir      = Vec3 { Cos(m_lightPitch) * Cos(m_lightYaw), Sin(m_lightPitch), Cos(m_lightPitch) * Sin(m_lightYaw) };
    terrain.snowHeight    = m_snowHeight;
    terrain.rockSlopeMin  = m_rockSlopeMin;
    terrain.rockSlopeMax  = m_rockSlopeMax;
    terrain.snowBlend     = m_snowBlend;
    terrain.debugView     = static_cast<float>(m_debugView);
    terrain.dirtHeight    = m_dirtHeight;
    gfx.UpdateBuffer(m_terrainCbh, AsMemoryView(terrain));

    m_graph.Execute(_view.fbh, _view.viewport);
}

void TerrainScene::OnImGuiRender()
{
    if (!BeginSceneDemoWindow("TerrainScene"))
    {
        ImGui::End();
        return;
    }

    // noise
    ImGui::SeparatorText("Noise");
    ImGui::InputInt("Seed", &m_noise.seed);
    ImGui::SliderInt("Octaves", &m_noise.octaves, 1, 8);
    ImGui::SliderFloat("Frequency", &m_noise.frequency, 0.5f, 8.f);
    ImGui::SliderFloat("Persistence", &m_noise.persistence, 0.2f, 0.8f);
    ImGui::SliderFloat("Lacunarity", &m_noise.lacunarity, 1.5f, 3.f);
    ImGui::SliderFloat("Exponent", &m_noise.exponent, 0.5f, 3.f);
    if (ImGui::Button("Regenerate"))
    {
        GenerateHeights_();
        UploadHeights_();
    }

    // shape
    ImGui::SeparatorText("Shape");
    ImGui::SliderFloat("Height Scale", &m_heightScale, 20.f, 400.f);

    // blend
    ImGui::SeparatorText("Blend");
    ImGui::SliderFloat("Tile Size", &m_tileSize, 2.f, 40.f);
    ImGui::SliderFloat("Dirt Height", &m_dirtHeight, 0.f, 0.5f);
    ImGui::SliderFloat("Snow Height", &m_snowHeight, 0.3f, 1.f);
    ImGui::SliderFloat("Snow Blend", &m_snowBlend, 0.01f, 0.4f);
    ImGui::SliderFloat("Rock Slope Min", &m_rockSlopeMin, 0.f, 0.5f);
    ImGui::SliderFloat("Rock Slope Max", &m_rockSlopeMax, 0.05f, 0.8f);

    // light
    ImGui::SeparatorText("Light");
    ImGui::SliderAngle("Yaw", &m_lightYaw, 0.f, 360.f);
    ImGui::SliderAngle("Pitch", &m_lightPitch, 5.f, 90.f);

    // debug
    ImGui::SeparatorText("Debug");
    const char* kViews[] = { "None", "Height", "Normal", "Weights (R: rock, G: snow, B: dirt)" };
    Int32       view     = static_cast<Int32>(m_debugView);
    ImGui::Combo("View", &view, kViews, 4);
    m_debugView = static_cast<eDebugView>(view);
    ImGui::Checkbox("Wireframe", &m_bWireframe);

    ImGui::End();
}

void TerrainScene::OnResize(
    const Int32 _width,
    const Int32 _height)
{
    m_camera.SetAspectRatioFromBackBuffer();
}

void TerrainScene::InitTerrain_()
{
    Graphics& gfx = GetGraphics();

    // 격자 정점. 위치는 [0, 1] 격자 좌표만 담고 높이는 VS가 하이트맵에서 읽음
    Vector<Vec2> vertices;
    vertices.reserve(kHeightMapSize * kHeightMapSize);
    for (Int32 z = 0; z < kHeightMapSize; ++z)
    {
        for (Int32 x = 0; x < kHeightMapSize; ++x)
        {
            vertices.push_back({ static_cast<float>(x) / static_cast<float>(kNumQuads), static_cast<float>(z) / static_cast<float>(kNumQuads) });
        }
    }

    VertexLayout layout = {};
    layout.Add(eVertexAttribute::Position, eAttributeFormat::Float2);
    m_vbh = gfx.CreateVertexBuffer(vertices, layout);

    Vector<UInt32> indices;
    indices.reserve(kNumQuads * kNumQuads * 6);
    for (Int32 z = 0; z < kNumQuads; ++z)
    {
        for (Int32 x = 0; x < kNumQuads; ++x)
        {
            const UInt32 i0 = static_cast<UInt32>(z * kHeightMapSize + x);
            const UInt32 i1 = i0 + 1;
            const UInt32 i2 = i0 + kHeightMapSize;
            const UInt32 i3 = i2 + 1;

            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }
    m_ibh = gfx.CreateIndexBuffer(indices, true);

    // 하이트맵
    GenerateHeights_();
    const MemoryView initData[] = {
        MemoryView { m_heights.data(), static_cast<SizeT>(m_heights.size() * sizeof(float)) }
    };
    m_heightTexh = gfx.CreateTexture2D(kHeightMapSize, kHeightMapSize, eTextureFormat::R32_Float, false, 1, eTextureFlags::Mutable, initData);

    // 지형 텍스처
    m_pGrass = LoadTexture(SCENE_PATH("Grass.jpg"));
    m_pRock  = LoadTexture(SCENE_PATH("Rock.jpg"));
    m_pSnow  = LoadTexture(SCENE_PATH("Snow.jpg"));
    m_pDirt  = LoadTexture(SCENE_PATH("Dirt.jpg"));
}

void TerrainScene::InitRenderGraph_()
{
    LoadProgram(m_graph, "P_Terrain", SCENE_PATH("TerrainShader.hlsl"));

    m_graph.EmplaceConstantBuffer("CB_Terrain", sizeof(CBTerrain));
    m_graph.EmplaceConstantBuffer("CB_Camera", sizeof(CBCamera));
    m_graph.EmplaceTexture2D("T_SceneColor", 1.f, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_SceneDepth", 1.f, eTextureFormat::D32_Float, false, 1, eTextureFlags::DepthStencil);

    AttachmentRG attchments[2] = {};
    attchments[0].name         = "T_SceneColor";
    attchments[1].name         = "T_SceneDepth";
    m_graph.EmplaceFrameBuffer("FB_Scene", attchments);

    m_terrainCbh = m_graph.GetConstantBuffer("CB_Terrain");
    m_cmrCbh     = m_graph.GetConstantBuffer("CB_Camera");

    // 지형 패스
    m_graph.AddRenderPass("Pass_Terrain")
        .SetProgram("P_Terrain")
        .SetFrameBuffer("FB_Scene")
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.35f, 0.5f, 0.7f, 1.f), 0)
        .SetDepthClearCommand(1.f)
        .SetSampler(kLinearWrap, eShader::PixelShader, 0)
        .SetConstantBuffer("CB_Terrain", eShader::VertexShader, CBTerrain::kSlot)
        .SetConstantBuffer("CB_Terrain", eShader::PixelShader, CBTerrain::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, CBCamera::kSlot)
        .SetRenderFlags(kBackCullDepthTest)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetExecutor(Bind<&TerrainScene::Submit_>(this));

    // blit
    ImageFilterManager& ifm = GetImageFilterManager();

    ImageFilterInfo blitInfo = {};
    blitInfo.filter          = eImageFilter::Blit;
    blitInfo.output          = eImageFilterOutput::Final;
    blitInfo.input0Texture   = "T_SceneColor";
    ifm.AddImageFilter(m_graph, "Pass_Blit", blitInfo);
}

void TerrainScene::GenerateHeights_()
{
    m_heights.resize(kHeightMapSize * kHeightMapSize);

    const UInt32 seed     = static_cast<UInt32>(m_noise.seed);
    const float  invQuads = 1.f / static_cast<float>(kNumQuads);
    const float  startAmp = 1.f;

    for (Int32 z = 0; z < kHeightMapSize; ++z)
    {
        for (Int32 x = 0; x < kHeightMapSize; ++x)
        {
            const float u = static_cast<float>(x) * invQuads;
            const float v = static_cast<float>(z) * invQuads;

            // fBm. 옥타브마다 주기는 lacunarity배, 진폭은 persistence배
            float sum       = 0.f;
            float amplitude = startAmp;
            float frequency = m_noise.frequency;
            float norm      = 0.f;
            for (Int32 octave = 0; octave < m_noise.octaves; ++octave)
            {
                sum += PerlinNoise(u * frequency, v * frequency, seed + static_cast<UInt32>(octave) * 131u) * amplitude;
                norm += amplitude;
                amplitude *= m_noise.persistence;
                frequency *= m_noise.lacunarity;
            }

            const float height01              = Clamp(sum / norm * 0.9f + 0.5f, 0.f, 1.f);
            m_heights[z * kHeightMapSize + x] = std::pow(height01, m_noise.exponent);
        }
    }
}

void TerrainScene::UploadHeights_() const
{
    GetGraphics().UpdateTexture2D(m_heightTexh, 0, 0, 0, 0, 0, 0, MemoryView { m_heights.data(), static_cast<SizeT>(m_heights.size() * sizeof(float)) }, kHeightMapSize * static_cast<Int32>(sizeof(float)));
}

void TerrainScene::Submit_() const
{
    Graphics& gfx = GetGraphics();

    gfx.SetRenderFlags(m_bWireframe ? (kBackCullDepthTest | eRenderFlags::Wireframe) : kBackCullDepthTest);

    gfx.SetTexture(m_heightTexh, eShader::VertexShader, kHeightSlot);
    m_pGrass->Bind(eShader::PixelShader, kGrassSlot);
    m_pRock->Bind(eShader::PixelShader, kRockSlot);
    m_pSnow->Bind(eShader::PixelShader, kSnowSlot);
    m_pDirt->Bind(eShader::PixelShader, kDirtSlot);

    gfx.SetVertexBuffer(m_vbh);
    gfx.SetIndexBuffer(m_ibh);
    gfx.Submit();
}
