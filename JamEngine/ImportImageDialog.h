#pragma once
#include "GraphicsBase.h"
#include "ImageBase.h"
#include "PopupDialog.h"

namespace jam
{

class ImportImageDialog
{
    class PreviewTexture : public RefCounted
    {
    public:
        PreviewTexture();
        ~PreviewTexture() override;

        PreviewTexture(const PreviewTexture& _other)            = delete;
        PreviewTexture& operator=(const PreviewTexture& _other) = delete;

        PreviewTexture(PreviewTexture&& _other) noexcept;
        PreviewTexture& operator=(PreviewTexture&& _other) noexcept;

        void Init(TextureHandle _texh);

        [[nodiscard]] TextureHandle      GetHandle() const;
        [[nodiscard]] const TextureInfo& GetInfo() const;

    private:
        void CleanUp_();

        TextureHandle m_texh = k_invalidHandle;
        TextureInfo   m_info = {};
    };

public:
    explicit ImportImageDialog(const fs::path& _path);
    void operator()();

private:
    bool OnClickImportButton_();
    void LoadPreviewTexture_();
    void Widget_PreviewTexture_() const;
    bool Widget_NameInput_();

    fs::path     m_parentDir;
    fs::path     m_importPath;
    std::string  m_nameBuf;
    eImageFormat m_imageFormat = eImageFormat::Unknown;

    // format conversion
    bool           m_bConvertFormat = false;
    eTextureFormat m_convertFormat  = eTextureFormat::RGBA8_UNorm;

    // color space
    bool m_bForceSrgb = false;

    // resizing
    bool   m_bResize = false;
    UInt64 m_resizeW = 0;
    UInt64 m_resizeH = 0;

    // mipmaps
    bool m_bDropMips = false;

    // transforms
    bool m_bFlipV = false;
    bool m_bFlipH = false;

    // alpha
    bool m_bPreMultAlpha = false;

    Ref<PreviewTexture> m_pPreviewTex      = nullptr;   // Shared for copy in std::function
    float               m_previewLoadTimer = 0.f;
    bool                m_bDirty           = false;
    PopupDialog         m_popup;

    static constexpr float  k_previewLoadDelay = 1.f;
    static constexpr ImVec2 k_previewSize      = ImVec2(128.f, 128.f);
};

}   // namespace jam