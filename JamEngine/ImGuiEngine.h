#pragma once
// ReSharper disable CppUnusedIncludeDirective
#include "GraphicsBase.h"
#include "Vendor/imgui/imgui.h"
#include "Vendor/imgui/imgui_internal.h"
#include "Vendor/imgui/imgui_stdlib.h"
#include "Vendor/ImGuizmo/ImGuizmo.h"

namespace Jam
{

class Texture;

class ImGuiEngine
{
public:
    ImGuiEngine(const ImGuiEngine&)            = delete;
    ImGuiEngine& operator=(const ImGuiEngine&) = delete;
    ImGuiEngine(ImGuiEngine&&)                 = delete;
    ImGuiEngine& operator=(ImGuiEngine&&)      = delete;

    [[nodiscard]] static ImGuiEngine& GetInstance();

private:
    explicit ImGuiEngine(HWND _hWnd);
    ~ImGuiEngine();

    static void CreateInstance(HWND _hWnd);
    static void DestroyInstance();

    void BeginFrame() const;
    void EndFrame();

    void Render_(ImDrawData* _pDrawData);
    void UpdateTexture_(ImTextureData* _pTexture) const;
    void DestroyTexture_(ImTextureData* _texture) const;

    ProgramHandle m_basicPh       = kInvalidHandle;
    ProgramHandle m_textureCubePh = kInvalidHandle;

    ShaderHandle m_vsh            = kInvalidHandle;
    ShaderHandle m_basicPsh       = kInvalidHandle;
    ShaderHandle m_textureCubePsh = kInvalidHandle;

    VertexLayout         m_vl  = {};
    ConstantBufferHandle m_cbh = kInvalidHandle;

    Flags<eBlendFlags>   m_bsFlags = {};
    Flags<eRenderFlags>  m_rsFlags = {};
    Flags<eSamplerFlags> m_ssFlags = {};

    VertexBufferHandle m_vbh   = kInvalidHandle;
    Int32              m_vbCap = 0;

    IndexBufferHandle m_ibh   = kInvalidHandle;
    Int32             m_ibCap = 0;

    bool m_bNeedCleanUpPlatformTextures = false;

    inline static ImGuiEngine* s_pInstance = nullptr;
    friend class Application;
};

[[nodiscard]] ImTextureRef ImTextureRefFrom(TextureHandle _texh, eTexture _type = eTexture::Texture2D);
[[nodiscard]] ImTextureRef ImTextureRefFrom(const Ref<Texture>& _pTexture);

[[nodiscard]] inline ImGuiEngine& GetImGuiEngine()
{
    return ImGuiEngine::GetInstance();
}

}   // namespace Jam
