#include "pch.h"

#include "ImGuiGlue.h"

#include "GeometryUtilities.h"
#include "Graphics.h"
#include "ImageTexture.h"
#include "ShaderCompiler.h"
#include "Vendor/imgui/ImGuizmo.h"
#include "Vendor/imgui/imgui_impl_win32.h"

namespace jam
{

constexpr const char* k_imguiGlueVs =
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

constexpr const char* k_imguiGlueBasicPs =
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

constexpr const char* k_imguiGlueTextureCubePs =
    "struct PS_INPUT\n"
    "{\n"
    "    float4 pos : SV_POSITION;\n"
    "    float2 uv  : TEXCOORD0;\n"
    "    float4 col : COLOR0;\n"
    "};\n"
    "\n"
    "sampler sampler0;\n"
    "TextureCube texture0;\n"
    "static const float2 k_center = float2(0.5f, 0.5f);\n"
    "static const float k_padding = 0.01f;\n"
    "static const float k_radius = 0.5f - k_padding;\n"
    "\n"
    "float4 main(PS_INPUT input) : SV_Target\n"
    "{\n"
    "    float dist = length(input.uv - k_center);\n"
    "    float weight = step(dist, k_radius); // dist <= k_radius : 1\n"
    "    float3 dir = float3(input.uv - k_center, sqrt(1.f - dist * dist));\n"
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
    TextureHandle texh = k_invalidHandle;
    eTextureType  type = eTextureType::Texture2D;
};

struct ImGuiGlueContext
{
    void Init();
    void Shutdown();
    void Render(ImDrawData* _pDrawData);
    void UpdateTexture(ImTextureData* _pTex) const;
    void DestroyTexture(ImTextureData* _tex) const;

    [[nodiscard]] ShaderHandle CreateShader(const char* _pShaderCode, eShader _shaderStage) const;
    [[nodiscard]] ImTextureRef ImTextureRefFrom(TextureHandle _texh, eTextureType _type = eTextureType::Texture2D) const;
    [[nodiscard]] ImTextureID  ImTextureIDFrom(TextureHandle _texh, eTextureType _type = eTextureType::Texture2D) const;
    [[nodiscard]] TextureData  TextureDataFrom(ImTextureID _texId) const;

    ProgramHandle basicPh;
    ProgramHandle textureCubePh;

    ShaderHandle vsh;
    ShaderHandle basicPsh;
    ShaderHandle textureCubePsh;

    VertexLayout         vl;
    ConstantBufferHandle cbh;

    BlendState   bs;
    RenderState  gs;
    SamplerState ss;

    VertexBufferHandle vbh;
    Int32              vbCap = 0;

    IndexBufferHandle ibh   = k_invalidHandle;
    Int32             ibCap = 0;

    bool bNeedCleanUpPlatformTextures = false;

}* g_pImguiCtx = nullptr;

void ImGuiGlueContext::Init()
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");
    io.BackendRendererName = "jam_engine_ImGui_glue_impl";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures;

    // create shaders
    vsh            = CreateShader(k_imguiGlueVs, eShader::VertexShader);
    basicPsh       = CreateShader(k_imguiGlueBasicPs, eShader::PixelShader);
    textureCubePsh = CreateShader(k_imguiGlueTextureCubePs, eShader::PixelShader);

    // create program
    basicPh       = Graphics::CreateProgram(vsh, basicPsh);
    textureCubePh = Graphics::CreateProgram(vsh, textureCubePsh);

    // create vertex vertexLayout
    vl.Push(eVertexAttribute::Position, eAttributeFormat::Float2);
    vl.Push(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
    vl.Push(eVertexAttribute::Color0, eAttributeFormat::UNorm8x4);

    // create constant buffer
    cbh = Graphics::CreateConstantBuffer(sizeof(IMGUI_CONSTANT_BUFFER));

    // state
    bs               = BlendState::k_alphaBlend;
    gs.bWireframe    = false;
    gs.cullMode      = eCullMode::None;
    gs.bScissor      = true;
    gs.bDepthClamp   = false;
    gs.depthTestFunc = eCompareFunc::None;
    ss               = SamplerState::k_linearClamp;
}

void ImGuiGlueContext::Shutdown()
{
    ImGuiIO& io                = ImGui::GetIO();
    io.BackendRendererName     = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures);

    ImGuiPlatformIO& platformIo = ImGui::GetPlatformIO();
    platformIo.ClearRendererHandlers();

    // 텍스처 정리
    for (ImTextureData* tex: platformIo.Textures)
    {
        const bool bIsDestroyStatus = tex->Status == ImTextureStatus_WantDestroy
                                   || tex->Status == ImTextureStatus_OK
                                   || tex->Status == ImTextureStatus_WantUpdates;

        if (tex->RefCount == 1 && bIsDestroyStatus)
        {
            DestroyTexture(tex);
        }
    }

    // 리소스 파괴
    JAM_DESTROY_SHADER(vsh);
    JAM_DESTROY_SHADER(basicPsh);
    JAM_DESTROY_SHADER(textureCubePsh);
    JAM_DESTROY_PROGRAM(basicPh);
    JAM_DESTROY_PROGRAM(textureCubePh);
    JAM_DESTROY_CONSTANT_BUFFER(cbh);
    JAM_DESTROY_VERTEX_BUFFER(vbh);
    JAM_DESTROY_INDEX_BUFFER(ibh);
}

void ImGuiGlueContext::Render(ImDrawData* _pDrawData)
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

    // Update textures
    if (_pDrawData->Textures)
    {
        for (ImTextureData* pImTex: *_pDrawData->Textures)
        {
            if (pImTex->Status != ImTextureStatus_OK)
            {
                UpdateTexture(pImTex);
            }
        }
    }

    // realloc
    if (vbCap < _pDrawData->TotalVtxCount)
    {
        JAM_DESTROY_VERTEX_BUFFER(vbh);
        vbCap = _pDrawData->TotalVtxCount + 5000;
        vbh   = Graphics::CreateDynamicVertexBuffer(vbCap * sizeof(ImDrawVert), vl);
    }

    // realloc
    if (ibCap < _pDrawData->TotalIdxCount)
    {
        JAM_DESTROY_INDEX_BUFFER(ibh);
        const bool bUseU32 = IsGreaterThanUInt16(_pDrawData->TotalVtxCount);
        ibCap              = _pDrawData->TotalIdxCount + 10000;
        ibh                = Graphics::CreateDynamicIndexBuffer(ibCap, bUseU32);
    }

    // copy data to cpu buffers
    const MemView gpuVtxData  = Graphics::BeginUploadBuffer(vbh);
    Byte*         pGpuVtxData = gpuVtxData.GetPtr();

    const MemView gpuIdxData  = Graphics::BeginUploadBuffer(ibh);
    Byte*         pGpuIdxData = gpuIdxData.GetPtr();

    for (const ImDrawList* pDrawList: _pDrawData->CmdLists)
    {
        // copy vertex data
        const size_t vtxCpySize = pDrawList->VtxBuffer.Size * sizeof(ImDrawVert);
        std::memcpy(pGpuVtxData, pDrawList->VtxBuffer.Data, vtxCpySize);

        // copy index data
        const size_t idxCpySize = pDrawList->IdxBuffer.Size * sizeof(ImDrawIdx);
        std::memcpy(pGpuIdxData, pDrawList->IdxBuffer.Data, idxCpySize);

        pGpuVtxData += vtxCpySize;
        pGpuIdxData += idxCpySize;
    }

    // unmap
    Graphics::EndUploadBuffer(vbh);
    Graphics::EndUploadBuffer(ibh);

    // setup
    const Viewport vp = { NarrowCast<UInt16>(_pDrawData->DisplaySize.x), NarrowCast<UInt16>(_pDrawData->DisplaySize.y) };
    Graphics::SetViewport(vp);

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

    Graphics::UploadBuffer(cbh, cb);
    Graphics::SetConstantBuffer(cbh, eShader::VertexShader, 0);
    Graphics::SetBlendState(bs, 0);
    Graphics::SetRenderState(gs);
    Graphics::SetSampler(ss, eShader::PixelShader, 0);
    Graphics::SetStencil(StencilState::k_disable, StencilState::k_disable);
    Graphics::SetVertexBuffer(vbh);

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
            const UInt16 clipMinX = NarrowCast<UInt16>(Max(0.f, (cmd.ClipRect.x - clipOff.x) * clipScale.x));
            const UInt16 clipMinY = NarrowCast<UInt16>(Max(0.f, (cmd.ClipRect.y - clipOff.y) * clipScale.y));
            const UInt16 clipMaxX = NarrowCast<UInt16>(Max(0.f, (cmd.ClipRect.z - clipOff.x) * clipScale.x));
            const UInt16 clipMaxY = NarrowCast<UInt16>(Max(0.f, (cmd.ClipRect.w - clipOff.y) * clipScale.y));

            if (clipMaxX <= clipMinX || clipMaxY <= clipMinY)
            {
                continue;
            }

            // Apply scissor/clipping rectangle
            Graphics::SetScissorRect(clipMinX, clipMinY, clipMaxX, clipMaxY);

            // Draw
            Graphics::SetIndexBuffer(ibh, cmd.ElemCount, 0);
            Graphics::SetSubmitParameter(eSubmitParameterType::StartIndexLocation, cmd.IdxOffset + startIdxLoc);
            Graphics::SetSubmitParameter(eSubmitParameterType::BaseVertexLocation, cmd.VtxOffset + baseVtxLoc);

            // Bind texture
            const auto [texh, texType] = TextureDataFrom(cmd.GetTexID());

            // set program
            const ProgramHandle ph = (texType == eTextureType::TextureCube) ? textureCubePh : basicPh;
            Graphics::SetProgram(ph);

            // submit
            Graphics::Submit();
        }

        startIdxLoc += pDrawList->IdxBuffer.Size;
        baseVtxLoc += pDrawList->VtxBuffer.Size;
    }

    // reset state
    Graphics::SetSubmitParameter(eSubmitParameterType::StartIndexLocation, 0);
    Graphics::SetSubmitParameter(eSubmitParameterType::BaseVertexLocation, 0);
    Graphics::Flush();
}

void ImGuiGlueContext::UpdateTexture(ImTextureData* _pTex) const
{
    if (_pTex->Status == ImTextureStatus_WantCreate)
    {
        JAM_ASSERT(_pTex->TexID == ImTextureID_Invalid && _pTex->BackendUserData == nullptr, "Trying to create a texture that is already created.");
        JAM_ASSERT(_pTex->Format == ImTextureFormat_RGBA32, "ImGuiGlueContext::UpdateTexture - Unsupported texture format.");

        const CMemView texData[] = {
            CMemView { _pTex->GetPixels(), _pTex->GetSizeInBytes() }
        };

        // dynamic texture creation
        const TextureHandle texh = Graphics::CreateTexture2D(_pTex->Width, _pTex->Height, eTextureFormat::RGBA8_UNorm, 1, eTextureCreateFlags_Mutable, texData);
        _pTex->SetTexID(texh.value);
        _pTex->SetStatus(ImTextureStatus_OK);
    }
    else if (_pTex->Status == ImTextureStatus_WantUpdates)
    {
        const auto [texh, _] = TextureDataFrom(_pTex->GetTexID());

        // upload dynamic
        for (const ImTextureRect& rect: _pTex->Updates)
        {
            const CMemView texData = { _pTex->GetPixelsAt(rect.x, rect.y), _pTex->GetSizeInBytes() };
            Graphics::UploadTexture2D(texh, 0, 0, rect.x, rect.y, texData, rect.w, rect.h, _pTex->GetPitch());
        }
        _pTex->SetStatus(ImTextureStatus_OK);
    }

    if (_pTex->Status == ImTextureStatus_WantDestroy && _pTex->UnusedFrames > 0)
    {
        DestroyTexture(_pTex);
    }
}

void ImGuiGlueContext::DestroyTexture(ImTextureData* _tex) const
{
    JAM_ASSERT(_tex->Status != ImTextureStatus_Destroyed && _tex->Status != ImTextureStatus_WantCreate, "Trying to destroy a texture that is not created.");

    // destroy texture
    const auto [texh, _] = TextureDataFrom(_tex->GetTexID());
    Graphics::DestroyTexture(texh);

    // clear info
    _tex->SetTexID(ImTextureID_Invalid);
    _tex->SetStatus(ImTextureStatus_Destroyed);
}

ShaderHandle ImGuiGlueContext::CreateShader(const char* _pShaderCode, const eShader _shaderStage) const
{
    ShaderCompiler compiler;
    if (!compiler.CompileHLSLSource(_pShaderCode, _shaderStage, "main", {}, eShaderCompileFlags_ReleaseDefault))
    {
        JAM_ASSERT(0, "Failed to compile ImGui shader: {}", GlobalBuffer::GetLastError());
    }

    return Graphics::CreateShader(compiler.GetBytecode());
}

ImTextureRef ImGuiGlueContext::ImTextureRefFrom(
    const TextureHandle _texh,
    const eTextureType  _type) const
{
    return ImTextureIDFrom(_texh, _type);
}

ImTextureID ImGuiGlueContext::ImTextureIDFrom(
    const TextureHandle _texh,
    const eTextureType  _type) const
{
    return static_cast<UInt64>(_texh.value) << 32 | static_cast<UInt64>(_type);
}

TextureData ImGuiGlueContext::TextureDataFrom(
    const ImTextureID _texId) const
{
    TextureData texData;
    texData.texh.value = static_cast<UInt32>(_texId >> 32);
    texData.type       = static_cast<eTextureType>(_texId & 0xFFFFFFFF);
    return texData;
}

}   // namespace jam

namespace jam
{

void ImGuiGlue::Init(
    const HWND _hwnd)
{
    JAM_ASSERT(g_pImguiCtx == nullptr, "ImGuiGlue::Init() called more than once without Shutdown().");
    g_pImguiCtx = JAM_NEW(ImGuiGlueContext);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // init
    ImGui_ImplWin32_Init(_hwnd);
    g_pImguiCtx->Init();
}

void ImGuiGlue::Shutdown()
{
    JAM_ASSERT(g_pImguiCtx, "ImGuiGlue::Shutdown() called without Init().");

    // shutdown
    ImGui_ImplWin32_Shutdown();
    g_pImguiCtx->Shutdown();
    ImGui::DestroyContext();

    JAM_DELETE(g_pImguiCtx);
}

void ImGuiGlue::BeginFrame()
{
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void ImGuiGlue::EndFrame()
{
    // 백버퍼 바인딩
    const auto [w, h] = Graphics::GetBackBufferResolution();
    Graphics::SetFrameBuffer(k_invalidHandle);
    Graphics::SetViewport(0, 0, w, h);

    // draw data 처리
    ImGui::Render();
    g_pImguiCtx->Render(ImGui::GetDrawData());

    // viewport 모드는 지원하지 않음
    // if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    // {
    //     ImGui::UpdatePlatformWindows();
    //     ImGui::RenderPlatformWindowsDefault();
    // }
}

ImTextureRef ImGuiGlue::ImTextureRefFrom(
    const TextureHandle _texh,
    const eTextureType  _type)
{
    return g_pImguiCtx->ImTextureRefFrom(_texh, _type);
}

ImTextureRef ImGuiGlue::ImTextureRefFrom(
    const Ref<ImageTexture>& _pTex)
{
    return g_pImguiCtx->ImTextureRefFrom(_pTex->GetHandle(), _pTex->GetTextureType());
}

}   // namespace jam
