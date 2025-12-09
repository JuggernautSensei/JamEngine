#include "pch.h"

#include "ImGuiGlue.h"

#include "Graphics.h"
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

constexpr const char* k_imguiGluePs =
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

struct ImGui_CONSTANT_BUFFER
{
    float mvp[4][4];
};

struct ImGuiGlueContext
{
    void Init();
    void Shutdown();
    void Render(ImDrawData* _pDrawData);
    void UpdateTexture(ImTextureData* _pTex) const;
    void DestroyTexture(ImTextureData* _tex) const;

    [[nodiscard]] ShaderHandle  CreateShader(const char* _pShaderCode, eShader _shaderStage) const;
    [[nodiscard]] ImTextureID   ToImTexureID(TextureHandle _texh) const;
    [[nodiscard]] TextureHandle FromImTexureID(ImTextureID _id) const;

    ProgramHandle        ph;
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
    const ShaderHandle vsh = CreateShader(k_imguiGlueVs, eShader::VertexShader);
    const ShaderHandle psh = CreateShader(k_imguiGluePs, eShader::PixelShader);

    // create program
    ph = Graphics::CreateProgram(vsh, psh, true);

    // create vertex vertexLayout
    vl.Add(eVertexAttribute::Position, eAttributeFormat::Float2)
        .Add(eVertexAttribute::TexCoord0, eAttributeFormat::Float2)
        .Add(eVertexAttribute::Color0, eAttributeFormat::UNorm8x4);

    // create constant buffer
    cbh = Graphics::CreateConstantBuffer(sizeof(ImGui_CONSTANT_BUFFER));

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

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.ClearRendererHandlers();

    for (ImTextureData* tex: platform_io.Textures)
    {
        const bool bIsDestroyStatus = tex->Status == ImTextureStatus_WantDestroy
                                   || tex->Status == ImTextureStatus_OK
                                   || tex->Status == ImTextureStatus_WantUpdates;

        if (tex->RefCount == 1 && bIsDestroyStatus)
        {
            DestroyTexture(tex);
        }
    }

    JAM_DESTROY_PROGRAM(ph);
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
        for (ImTextureData* tex: *_pDrawData->Textures)
        {
            if (tex->Status != ImTextureStatus_OK)
            {
                UpdateTexture(tex);
            }
        }
    }

    if (vbCap < _pDrawData->TotalVtxCount)
    {
        JAM_DESTROY_VERTEX_BUFFER(vbh);
        vbCap = _pDrawData->TotalVtxCount + 5000;
        vbh   = Graphics::CreateDynamicVertexBuffer(vbCap * sizeof(ImDrawVert), vl);
    }

    if (ibCap < _pDrawData->TotalIdxCount)
    {
        JAM_DESTROY_INDEX_BUFFER(ibh);
        const bool bUseU32 = vbCap > 65535;
        ibCap              = _pDrawData->TotalIdxCount + 10000;
        ibh                = Graphics::CreateDynamicIndexBuffer(ibCap, bUseU32);
    }

    // copy data to cpu buffers
    MemView vtxMem  = Graphics::BeginUploadBuffer(vbh);
    Byte*   pVtxMem = vtxMem.GetDataPtr();

    MemView idxMem  = Graphics::BeginUploadBuffer(ibh);
    Byte*   pIdxMem = idxMem.GetDataPtr();

    for (const ImDrawList* pDrawList: _pDrawData->CmdLists)
    {
        // copy vertex data
        size_t vtxCpySize = pDrawList->VtxBuffer.Size * sizeof(ImDrawVert);
        memcpy(pVtxMem, pDrawList->VtxBuffer.Data, vtxCpySize);

        // copy index data
        size_t idxCpySize = pDrawList->IdxBuffer.Size * sizeof(ImDrawIdx);
        memcpy(pIdxMem, pDrawList->IdxBuffer.Data, idxCpySize);

        pVtxMem += vtxCpySize;
        pIdxMem += idxCpySize;
    }

    Graphics::EndUploadBuffer(vbh);
    Graphics::EndUploadBuffer(ibh);

    // setup
    Graphics::SetViewport(0, 0, NarrowCast<UInt16>(_pDrawData->DisplaySize.x), NarrowCast<UInt16>(_pDrawData->DisplaySize.y));

    const float L = _pDrawData->DisplayPos.x;
    const float R = _pDrawData->DisplayPos.x + _pDrawData->DisplaySize.x;
    const float T = _pDrawData->DisplayPos.y;
    const float B = _pDrawData->DisplayPos.y + _pDrawData->DisplaySize.y;

    ImGui_CONSTANT_BUFFER cb;
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
    Graphics::SetProgram(ph);
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
                continue;

            // Apply scissor/clipping rectangle
            Graphics::SetScissorRect(clipMinX, clipMinY, clipMaxX, clipMaxY);

            // Bind texture
            const TextureHandle texh = FromImTexureID(cmd.GetTexID());
            Graphics::SetTexture(texh, eShader::PixelShader, 0);

            // Draw
            Graphics::SetIndexBuffer(ibh, cmd.ElemCount, 0);
            Graphics::SetSubmitParameter(eSubmitParameterType::StartIndexLocation, cmd.IdxOffset + startIdxLoc);
            Graphics::SetSubmitParameter(eSubmitParameterType::BaseVertexLocation, cmd.VtxOffset + baseVtxLoc);
            Graphics::Submit();
        }

        startIdxLoc += pDrawList->IdxBuffer.Size;
        baseVtxLoc += pDrawList->VtxBuffer.Size;
    }

    Graphics::SetSubmitParameter(eSubmitParameterType::StartIndexLocation, 0);
    Graphics::SetSubmitParameter(eSubmitParameterType::BaseVertexLocation, 0);
    Graphics::SetTexture(k_invalidHandle, eShader::PixelShader, 0);
    Graphics::Flush();
}

void ImGuiGlueContext::UpdateTexture(ImTextureData* _pTex) const
{
    if (_pTex->Status == ImTextureStatus_WantCreate)
    {
        IM_ASSERT(_pTex->TexID == ImTextureID_Invalid && _pTex->BackendUserData == nullptr);
        IM_ASSERT(_pTex->Format == ImTextureFormat_RGBA32);

        ConstMemView mem[] = {
            ConstMemView { _pTex->GetPixels(), static_cast<size_t>(_pTex->GetSizeInBytes()) }
        };

        const TextureHandle texh = Graphics::CreateTexture2D(_pTex->Width, _pTex->Height, eTextureFormat::RGBA8_UNorm, 1, eTextureCreateFlags_Mutable, mem);
        _pTex->SetTexID(texh.value);
        _pTex->SetStatus(ImTextureStatus_OK);
    }
    else if (_pTex->Status == ImTextureStatus_WantUpdates)
    {
        const TextureHandle texh = FromImTexureID(_pTex->GetTexID());
        for (ImTextureRect& rect: _pTex->Updates)
        {
            ConstMemView mem { _pTex->GetPixelsAt(rect.x, rect.y), static_cast<size_t>(_pTex->GetSizeInBytes()) };
            Graphics::UploadTexture2D(texh, 0, 0, rect.x, rect.y, mem, rect.w, rect.h, _pTex->GetPitch());
        }
        _pTex->SetStatus(ImTextureStatus_OK);
    }

    if (_pTex->Status == ImTextureStatus_WantDestroy && _pTex->UnusedFrames > 0)
        DestroyTexture(_pTex);
}

void ImGuiGlueContext::DestroyTexture(ImTextureData* _tex) const
{
    JAM_ASSERT(_tex->Status != ImTextureStatus_Destroyed && _tex->Status != ImTextureStatus_WantCreate, "Trying to destroy a texture that is not created.");

    TextureHandle texh = FromImTexureID(_tex->GetTexID());
    JAM_DESTROY_TEXTURE(texh);
    _tex->SetTexID(ImTextureID_Invalid);
    _tex->SetStatus(ImTextureStatus_Destroyed);
}

ShaderHandle ImGuiGlueContext::CreateShader(const char* _pShaderCode, const eShader _shaderStage) const
{
    ShaderCompiler compiler;
    if (!compiler.CompileFromSource(_pShaderCode, _shaderStage, "main", eShaderCompileFlags_ReleaseDefault))
    {
        JAM_ASSERT(0, "ImGuiGlueContext::CreateShader - Shader compilation failed.\n"
                      "Error message: {}\n",
                   GetErrorString());
    }

    return Graphics::CreateShader(compiler.GetBytecode());
}

ImTextureID ImGuiGlueContext::ToImTexureID(const TextureHandle _texh) const
{
    return _texh.value;
}

TextureHandle ImGuiGlueContext::FromImTexureID(const ImTextureID _id) const
{
    TextureHandle texh;
    texh.value = NarrowCast<UInt32>(_id);
    return texh;
}

}   // namespace jam

namespace jam
{

void ImGuiGlue::Init(const HWND _hwnd)
{
    JAM_ASSERT(g_pImguiCtx == nullptr, "ImGuiGlue::Init() called more than once without Shutdown().");
    g_pImguiCtx = JAM_NEW(ImGuiGlueContext);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplWin32_Init(_hwnd);
    g_pImguiCtx->Init();
}

void ImGuiGlue::Shutdown()
{
    JAM_ASSERT(g_pImguiCtx, "ImGuiGlue::Shutdown() called without Init().");

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
    auto [w, h] = Graphics::GetBackBufferResolution();
    Graphics::SetFrameBuffer(k_invalidHandle);
    Graphics::SetViewport(0, 0, w, h);

    ImGui::Render();
    g_pImguiCtx->Render(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}
}   // namespace jam
