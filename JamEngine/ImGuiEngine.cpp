#include "pch.h"

#include "Vendor/imgui/imgui_impl_win32.h"

#include "ImGuiEngine.h"

#include "GeometryUtilities.h"
#include "Graphics.h"
#include "Texture.h"
#include "ShaderCompiler.h"

namespace Jam
{
namespace
{

    constexpr const char* kImguiGlueVs =
        "cbuffer vertexBuffer : register(b0)\n"
        "{\n"
        "    float4x4 ProjectionMatrix;\n"
        "};\n"
        "struct VS_INPUT\n"
        "{\n"
        "    float2 pos : POSITION;\n"
        "    float2 uv  : TEXCOORD0;\n"
        "    float4 col : COLOR0;\n"
        "};\n"
        "\n"
        "struct PS_INPUT\n"
        "{\n"
        "    float4 pos : SV_POSITION;\n"
        "    float2 uv  : TEXCOORD0;\n"
        "    float4 col : COLOR0;\n"
        "};\n"
        "\n"
        "PS_INPUT main(VS_INPUT input)\n"
        "{\n"
        "    PS_INPUT output;\n"
        "    output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\n"
        "    output.uv  = input.uv;\n"
        "    output.col = input.col;\n"
        "    return output;\n"
        "}\n";

    constexpr const char* kImguiGlueBasicPs =
        "struct PS_INPUT\n"
        "{\n"
        "    float4 pos : SV_POSITION;\n"
        "    float2 uv  : TEXCOORD0;\n"
        "    float4 col : COLOR0;\n"
        "};\n"
        "\n"
        "sampler sampler0;\n"
        "Texture2D texture0;\n"
        "\n"
        "float4 main(PS_INPUT input) : SV_Target\n"
        "{\n"
        "    float4 out_col = input.col * texture0.Sample(sampler0, input.uv);\n"
        "    return out_col;\n"
        "}\n";

    constexpr const char* kImguiGlueTextureCubePs =
        "struct PS_INPUT\n"
        "{\n"
        "    float4 pos : SV_POSITION;\n"
        "    float2 uv  : TEXCOORD0;\n"
        "    float4 col : COLOR0;\n"
        "};\n"
        "\n"
        "sampler sampler0;\n"
        "TextureCube texture0;\n"
        "static const float2 kCenter = float2(0.5f, 0.5f);\n"
        "static const float kPadding = 0.01f;\n"
        "static const float kRadius = 0.5f - kPadding;\n"
        "\n"
        "float4 main(PS_INPUT input) : SV_Target\n"
        "{\n"
        "    float dist = length(input.uv - kCenter);\n"
        "    float weight = step(dist, kRadius); // dist <= kRadius : 1\n"
        "    float3 dir = float3(input.uv - kCenter, sqrt(1.f - dist * dist));\n"
        "    dir = normalize(dir);\n"
        "    float4 out_col = input.col * texture0.Sample(sampler0, dir) * weight;\n"
        "    return out_col;\n"
        "}\n";

    struct IMGUI_CONSTANT_BUFFER
    {
        float mvp[4][4];
    };

    struct TextureData
    {
        TextureHandle texh = kInvalidHandle;
        eTexture      type = eTexture::Texture2D;
    };

    ImTextureID ImTextureIDFrom_(
        const TextureHandle _texh,
        const eTexture      _type)
    {
        return (static_cast<UInt64>(_texh.GetValue()) << 32) | static_cast<UInt64>(UnderlyingFrom(_type));
    }

    TextureData TextureDataFrom_(
        const ImTextureID _texId)
    {
        TextureData texData = {};
        texData.texh        = TextureHandle { static_cast<UInt32>(_texId >> 32) };
        texData.type        = static_cast<eTexture>(_texId & 0xFFFFFFFF);
        return texData;
    }

    ShaderHandle CreateShader(
        const StringView _source,
        const eShader    _stage)
    {
        ShaderCompilerInfo desc = {};
        desc.stage              = _stage;

        ShaderCompiler complier = ShaderCompiler::CompileHLSLSource(_source, desc);
        JAM_ASSERT(complier.IsSucceeded(), "Failed to compile ImGui shader: {}", complier.GetError().ToErrorMessage());
        return GetGraphics().CreateShader(complier.GetBytecode());
    }

}   // namespace

ImGuiEngine& ImGuiEngine::GetInstance()
{
    return *s_pInstance;
}

ImGuiEngine::ImGuiEngine(
    const HWND _hWnd)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // init
    ImGui_ImplWin32_Init(_hWnd);

    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");
    io.BackendRendererName = "jam_engine_ImGui_glue_impl";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures;

    // create shaders
    m_vsh            = CreateShader(kImguiGlueVs, eShader::VertexShader);
    m_basicPsh       = CreateShader(kImguiGlueBasicPs, eShader::PixelShader);
    m_textureCubePsh = CreateShader(kImguiGlueTextureCubePs, eShader::PixelShader);

    // create program
    Graphics& gfx   = GetGraphics();
    m_basicPh       = gfx.CreateProgram(m_vsh, m_basicPsh);
    m_textureCubePh = gfx.CreateProgram(m_vsh, m_textureCubePsh);

    // create vertex vertexLayout
    m_vl.Push(eVertexAttribute::Position, eAttributeFormat::Float2);
    m_vl.Push(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
    m_vl.Push(eVertexAttribute::Color0, eAttributeFormat::UNorm8x4);

    // create constant buffer
    m_cbh = gfx.CreateConstantBuffer(sizeof(IMGUI_CONSTANT_BUFFER));

    // state
    m_bsFlags = eBlendFlags::Enable
              | eBlendFlags::SrcBlend_SrcAlpha | eBlendFlags::DstBlend_InvSrcAlpha | eBlendFlags::BlendOp_Add
              | eBlendFlags::SrcBlendAlpha_SrcAlpha | eBlendFlags::DstBlendAlpha_InvSrcAlpha | eBlendFlags::BlendOpAlpha_Add
              | eBlendFlags::WriteMask_All;
    m_rsFlags = eRenderFlags::CullMode_None | eRenderFlags::Scissor;
    m_ssFlags = eSamplerFlags::Filter_MinLinear_MagLinear_MipLinear | eSamplerFlags::AddressU_Clamp | eSamplerFlags::AddressV_Clamp;
}

ImGuiEngine::~ImGuiEngine()
{
    ImGui_ImplWin32_Shutdown();

    ImGuiIO& io                = ImGui::GetIO();
    io.BackendRendererName     = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures);

    ImGuiPlatformIO& platformIo = ImGui::GetPlatformIO();
    platformIo.ClearRendererHandlers();

    // 텍스처 정리
    for (ImTextureData* texture: platformIo.Textures)
    {
        const bool bIsDestroyStatus = texture->Status == ImTextureStatus_WantDestroy
                                   || texture->Status == ImTextureStatus_OK
                                   || texture->Status == ImTextureStatus_WantUpdates;

        if (texture->RefCount == 1 && bIsDestroyStatus)
        {
            DestroyTexture_(texture);
        }
    }

    // 리소스 파괴
    JAM_DESTROY_SHADER(m_vsh);
    JAM_DESTROY_SHADER(m_basicPsh);
    JAM_DESTROY_SHADER(m_textureCubePsh);
    JAM_DESTROY_PROGRAM(m_basicPh);
    JAM_DESTROY_PROGRAM(m_textureCubePh);
    JAM_DESTROY_CONSTANT_BUFFER(m_cbh);
    JAM_DESTROY_VERTEX_BUFFER(m_vbh);
    JAM_DESTROY_INDEX_BUFFER(m_ibh);

    // ImGui 컨텍스트 파괴
    ImGui::DestroyContext();
}

void ImGuiEngine::CreateInstance(
    const HWND _hWnd)
{
    JAM_ASSERT(!s_pInstance, "ImGuiGlue::CreateInstance() called more than once without DestroyInstance().");
    s_pInstance = new ImGuiEngine(_hWnd);
}

void ImGuiEngine::DestroyInstance()
{
    delete s_pInstance;
    s_pInstance = nullptr;
}

void ImGuiEngine::BeginFrame() const
{
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void ImGuiEngine::EndFrame()
{
    // 백버퍼 바인딩
    Graphics&   gfx = GetGraphics();
    const Vec2i res = gfx.GetBackBufferResolution();
    gfx.SetFrameBuffer(kInvalidHandle);
    gfx.SetViewport(0, 0, res.w, res.h);

    // draw data 처리
    ImGui::Render();
    Render_(ImGui::GetDrawData());

    // viewport 모드는 지원하지 않음
    // if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    // {
    //     ImGui::UpdatePlatformWindows();
    //     ImGui::RenderPlatformWindowsDefault();
    // }
}

void ImGuiEngine::Render_(
    ImDrawData* _pDrawData)
{
    // avoid rendering when minimized
    if (_pDrawData->DisplaySize.x <= 0.0f || _pDrawData->DisplaySize.y <= 0.0f)
    {
        return;
    }

    // avoid empty draw data
    if (_pDrawData->TotalVtxCount == 0 || _pDrawData->TotalIdxCount == 0)
    {
        return;
    }

    // Update texes
    if (_pDrawData->Textures)
    {
        for (ImTextureData* pImTex: *_pDrawData->Textures)
        {
            if (pImTex->Status != ImTextureStatus_OK)
            {
                UpdateTexture_(pImTex);
            }
        }
    }

    Graphics& gfx = GetGraphics();

    // realloc
    if (m_vbCap < _pDrawData->TotalVtxCount)
    {
        JAM_DESTROY_VERTEX_BUFFER(m_vbh);
        m_vbCap = _pDrawData->TotalVtxCount + 5000;
        m_vbh   = gfx.CreateDynamicVertexBuffer(m_vbCap * sizeof(ImDrawVert), m_vl);
    }

    // realloc
    if (m_ibCap < _pDrawData->TotalIdxCount)
    {
        JAM_DESTROY_INDEX_BUFFER(m_ibh);
        const bool bUseU32 = std::cmp_greater(_pDrawData->TotalVtxCount, Limits<UInt16>::kHighest);
        m_ibCap            = _pDrawData->TotalIdxCount + 10000;
        m_ibh              = gfx.CreateDynamicIndexBuffer(m_ibCap, bUseU32);
    }

    // copy data to cpu buffers
    const MutableMemoryView gpuVtxData  = gfx.BeginUpdateBuffer(m_vbh);
    Byte*                   pGpuVtxData = gpuVtxData.GetPtr();

    const MutableMemoryView gpuIdxData  = gfx.BeginUpdateBuffer(m_ibh);
    Byte*                   pGpuIdxData = gpuIdxData.GetPtr();

    for (const ImDrawList* pDrawList: _pDrawData->CmdLists)
    {
        // copy vertex data
        const Int32 vtxCpySize = pDrawList->VtxBuffer.Size * sizeof(ImDrawVert);
        std::memcpy(pGpuVtxData, pDrawList->VtxBuffer.Data, vtxCpySize);

        // copy pos data
        const Int32 idxCpySize = pDrawList->IdxBuffer.Size * sizeof(ImDrawIdx);
        std::memcpy(pGpuIdxData, pDrawList->IdxBuffer.Data, idxCpySize);

        pGpuVtxData += vtxCpySize;
        pGpuIdxData += idxCpySize;
    }

    // unmap
    gfx.EndUpdateBuffer(m_vbh);
    gfx.EndUpdateBuffer(m_ibh);

    // setup
    const Viewport vp = { static_cast<UInt16>(_pDrawData->DisplaySize.x), static_cast<UInt16>(_pDrawData->DisplaySize.y) };
    gfx.SetViewport(vp);

    const float L = _pDrawData->DisplayPos.x;
    const float R = _pDrawData->DisplayPos.x + _pDrawData->DisplaySize.x;
    const float T = _pDrawData->DisplayPos.y;
    const float B = _pDrawData->DisplayPos.y + _pDrawData->DisplaySize.y;

    IMGUI_CONSTANT_BUFFER cb;
    cb.mvp[0][0] = 2.0f / (R - L);
    cb.mvp[0][1] = 0.0f;
    cb.mvp[0][2] = 0.0f;
    cb.mvp[0][3] = 0.0f;

    cb.mvp[1][0] = 0.0f;
    cb.mvp[1][1] = 2.0f / (T - B);
    cb.mvp[1][2] = 0.0f;
    cb.mvp[1][3] = 0.0f;

    cb.mvp[2][0] = 0.0f;
    cb.mvp[2][1] = 0.0f;
    cb.mvp[2][2] = 0.5f;
    cb.mvp[2][3] = 0.0f;

    cb.mvp[3][0] = (R + L) / (L - R);
    cb.mvp[3][1] = (T + B) / (B - T);
    cb.mvp[3][2] = 0.5f;
    cb.mvp[3][3] = 1.0f;

    gfx.UpdateBuffer(m_cbh, AsMemoryView(cb));
    gfx.SetConstantBuffer(m_cbh, eShader::VertexShader, 0);
    gfx.SetBlendFlags(m_bsFlags, 0);
    gfx.SetRenderFlags(m_rsFlags);
    gfx.SetSampler(m_ssFlags, eShader::PixelShader, 0);
    gfx.SetStencilFlags(eStencilFlags::None, eStencilFlags::None);
    gfx.SetVertexBuffer(m_vbh);

    // Render command lists
    Int32 startIdxLoc = 0;
    Int32 baseVtxLoc  = 0;

    const ImVec2 clipOff   = _pDrawData->DisplayPos;
    const ImVec2 clipScale = _pDrawData->FramebufferScale;
    for (const ImDrawList* pDrawList: _pDrawData->CmdLists)
    {
        for (const ImDrawCmd& cmd: pDrawList->CmdBuffer)
        {
            // Project scissor/clipping rectangles into framebuffer space
            const UInt16 clipMinX = static_cast<UInt16>(Max(0.f, (cmd.ClipRect.x - clipOff.x) * clipScale.x));
            const UInt16 clipMinY = static_cast<UInt16>(Max(0.f, (cmd.ClipRect.y - clipOff.y) * clipScale.y));
            const UInt16 clipMaxX = static_cast<UInt16>(Max(0.f, (cmd.ClipRect.z - clipOff.x) * clipScale.x));
            const UInt16 clipMaxY = static_cast<UInt16>(Max(0.f, (cmd.ClipRect.w - clipOff.y) * clipScale.y));

            if (clipMaxX <= clipMinX || clipMaxY <= clipMinY)
            {
                continue;
            }

            // Apply scissor/clipping rectangle
            gfx.SetScissorRect(clipMinX, clipMinY, clipMaxX, clipMaxY);

            // Draw
            gfx.SetIndexBuffer(m_ibh, 0, cmd.ElemCount);
            gfx.SetSubmitParameter(eSubmitParameter::StartIndexLocation, cmd.IdxOffset + startIdxLoc);
            gfx.SetSubmitParameter(eSubmitParameter::BaseVertexLocation, cmd.VtxOffset + baseVtxLoc);

            // Bind texture
            const auto [texh, type] = TextureDataFrom_(cmd.GetTexID());

            // set program
            const ProgramHandle ph = (type == eTexture::TextureCube) ? m_textureCubePh : m_basicPh;
            gfx.SetProgram(ph);
            gfx.SetTexture(texh, eShader::PixelShader, 0);

            // submit
            gfx.Submit();
        }

        startIdxLoc += pDrawList->IdxBuffer.Size;
        baseVtxLoc += pDrawList->VtxBuffer.Size;
    }

    // reset state
    gfx.SetSubmitParameter(eSubmitParameter::StartIndexLocation, 0);
    gfx.SetSubmitParameter(eSubmitParameter::BaseVertexLocation, 0);

    // reset texture
    gfx.SetTexture(kInvalidHandle, eShader::PixelShader, 0);
    gfx.Touch();
}

void ImGuiEngine::UpdateTexture_(
    ImTextureData* _pTexture) const
{
    Graphics& gfx = GetGraphics();

    if (_pTexture->Status == ImTextureStatus_WantCreate)
    {
        JAM_ASSERT(_pTexture->TexID == ImTextureID_Invalid && _pTexture->BackendUserData == nullptr, "Trying to create a texture that is already created.");
        JAM_ASSERT(_pTexture->Format == ImTextureFormat_RGBA32, "ImGuiGlueContext::UpdateTexture - Unsupported texture format.");

        const MemoryView texData[] = {
            MemoryView { _pTexture->GetPixels(), _pTexture->GetSizeInBytes() }
        };

        // dynamic texture creation
        const TextureHandle texh = gfx.CreateTexture2D(_pTexture->Width, _pTexture->Height, eTextureFormat::RGBA8_UNorm, false, 1, eTextureCreateFlags::Mutable, texData);
        gfx.SetTextureName(texh, "ImGui Texture");
        _pTexture->SetTexID(ImTextureIDFrom_(texh, eTexture::Texture2D));
        _pTexture->SetStatus(ImTextureStatus_OK);
    }
    else if (_pTexture->Status == ImTextureStatus_WantUpdates)
    {
        const auto [texh, _] = TextureDataFrom_(_pTexture->GetTexID());

        // upload dynamic
        for (const ImTextureRect& rect: _pTexture->Updates)
        {
            const MemoryView texData = { _pTexture->GetPixelsAt(rect.x, rect.y), (_pTexture->GetPitch() * rect.h) };
            gfx.UpdateTexture2D(texh, 0, 0, rect.x, rect.y, rect.w, rect.h, texData, _pTexture->GetPitch());
        }
        _pTexture->SetStatus(ImTextureStatus_OK);
    }

    if (_pTexture->Status == ImTextureStatus_WantDestroy && _pTexture->UnusedFrames > 0)
    {
        DestroyTexture_(_pTexture);
    }
}

void ImGuiEngine::DestroyTexture_(ImTextureData* _texture) const
{
    JAM_ASSERT(_texture->Status != ImTextureStatus_Destroyed && _texture->Status != ImTextureStatus_WantCreate, "Trying to destroy a texture that is not created.");

    // destroy texture
    Graphics& gfx        = GetGraphics();
    const auto [texh, _] = TextureDataFrom_(_texture->GetTexID());
    gfx.DestroyTexture(texh);

    // clear info
    _texture->SetTexID(ImTextureID_Invalid);
    _texture->SetStatus(ImTextureStatus_Destroyed);
}

ImTextureRef ImTextureRefFrom(
    const TextureHandle _texh,
    const eTexture      _type)
{
    return ImTextureIDFrom_(_texh, _type);
}

ImTextureRef ImTextureRefFrom(
    const Ref<Texture>& _pTexture)
{
    return ImTextureIDFrom_(_pTexture->GetHandle(), _pTexture->GetTextureType());
}

}   // namespace Jam
