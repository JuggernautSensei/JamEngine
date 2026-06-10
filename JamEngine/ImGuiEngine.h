#pragma once
// ReSharper disable CppUnusedIncludeDirective
#include "GraphicsBase.h"

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
    ShaderHandle m_psh            = kInvalidHandle;
    ShaderHandle m_textureCubePsh = kInvalidHandle;

    VertexLayout         m_layout = {};
    ConstantBufferHandle m_cbh    = kInvalidHandle;

    Flags<eBlendFlags>   m_blendFlags   = {};
    Flags<eRenderFlags>  m_renderFlags  = {};
    Flags<eSamplerFlags> m_samplerFlags = {};

    VertexBufferHandle m_vbh   = kInvalidHandle;
    Int32              m_vbCap = 0;

    IndexBufferHandle m_ibh   = kInvalidHandle;
    Int32             m_ibCap = 0;

    inline static ImGuiEngine* s_pInstance = nullptr;
    friend class Application;
};

[[nodiscard]] ImTextureRef AsImTextureRef(TextureHandle _texh, eTexture _type = eTexture::Texture2D);
[[nodiscard]] ImTextureRef AsImTextureRef(const Ref<Texture>& _pTexture);

[[nodiscard]] inline ImGuiEngine& GetImGuiEngine()
{
    return ImGuiEngine::GetInstance();
}

}   // namespace Jam
