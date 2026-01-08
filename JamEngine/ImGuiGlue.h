#pragma once
#include "GraphicsBase.h"
#include "IAsset.h"
#include "Vendor/imgui/imgui.h"
#include "Vendor/imgui/imgui_internal.h"
#include "Vendor/imgui/imgui_stdlib.h"

namespace jam
{

class ImageTexture;

class ImGuiGlue
{
public:
    static void Init(HWND _hwnd);
    static void Shutdown();
    static void BeginFrame();
    static void EndFrame();

    [[nodiscard]] static ImTextureRef ImTextureRefFrom(TextureHandle _texh, eTextureType _type = eTextureType::Texture2D);
    [[nodiscard]] static ImTextureRef ImTextureRefFrom(const Ref<ImageTexture>& _pTex);
};

}   // namespace jam