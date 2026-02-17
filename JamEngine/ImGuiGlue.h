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

class ImGuiGlue
{
public:
    ImGuiGlue(const ImGuiGlue&)            = delete;
    ImGuiGlue& operator=(const ImGuiGlue&) = delete;
    ImGuiGlue(ImGuiGlue&&)                 = delete;
    ImGuiGlue& operator=(ImGuiGlue&&)      = delete;

    [[nodiscard]] static ImGuiGlue& GetInstance();

private:
    explicit ImGuiGlue(HWND _hWnd);
    ~ImGuiGlue();

    static void CreateInstance(HWND _hWnd);
    static void DestroyInstance();

    void BeginFrame() const;
    void EndFrame();

    void Render_(ImDrawData* _pDrawData);
    void UpdateTexture_(ImTextureData* _pTexture) const;
    void DestroyTexture_(ImTextureData* _texture) const;

    ProgramHandle m_basicPh       = k_nullHandle;
    ProgramHandle m_textureCubePh = k_nullHandle;

    ShaderHandle m_vsh            = k_nullHandle;
    ShaderHandle m_basicPsh       = k_nullHandle;
    ShaderHandle m_textureCubePsh = k_nullHandle;

    VertexLayout         m_vl  = {};
    ConstantBufferHandle m_cbh = k_nullHandle;

    BlendState   m_bs = {};
    RenderState  m_rs = {};
    SamplerState m_ss = {};

    VertexBufferHandle m_vbh   = k_nullHandle;
    Int32              m_vbCap = 0;

    IndexBufferHandle m_ibh   = k_nullHandle;
    Int32             m_ibCap = 0;

    bool m_bNeedCleanUpPlatformTextures = false;

    inline static ImGuiGlue* s_pInstance = nullptr;
    friend class Application;
};

[[nodiscard]] ImTextureRef ImTextureRefFrom(TextureHandle _texh, eTextureType _type = eTextureType::Texture2D);
[[nodiscard]] ImTextureRef ImTextureRefFrom(const Ref<Texture>& _pTexture);

}   // namespace Jam

#define JAM_IMGUI Jam::ImGuiGlue::GetInstance()