#include "pch.h"

#include "ImageImportDialog.h"

#include "Editor.h"
#include "EditorUtilities.h"
#include "WindowsUtilities.h"
#include "EditorWidgets.h"
#include "GraphicsBase.h"
#include "ImGuiUtilities.h"
#include "ImageImporter.h"

namespace jam
{

ImageImportDialog::ImageImportDialog(
    const fs::path& _path)
    : AssetDialog("import image")
    , m_pPreviewTex(MakeRef<ImageTexture>())
{
    if (fs::is_directory(_path))
    {
        m_dir = _path;
    }
    else
    {
        // if not a directory, load preview immediately
        LoadPreviewTexture_();
        m_dir = _path.parent_path();
    }

    // default import option
    {
        m_options.bGenerateMips = true;
    }

    // file dialog
    {
        constexpr FileDialogFilter k_filters[] = {
            { "image files", "*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.gif;*.dds;*.hdr;*.psd;*.tiff" }
       };

        m_fd.SetTitle("open image file");
        m_fd.SetFilters(k_filters);
    }
}

void ImageImportDialog::OnShow()
{
    // preview load if need
    ImGuiIO& io = ImGui::GetIO();
    m_previewLoadCounterSec += io.DeltaTime;
    if (m_bPreviewLoadRequested && m_previewLoadCounterSec >= k_previewLoadDebounceSec)
    {
        LoadPreviewTexture_();
        m_bPreviewLoadRequested = false;
        m_previewLoadCounterSec = 0.f;
    }

    // render table
    if (BeginLabelValueTable("ImportImageDialog"))
    {
        // path
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("path");
        ImGui::TableNextColumn();
        if (Widget_PathSelector("##ImagePathSelector", m_dir, m_fd))
        {
            m_bPreviewLoadRequested = true;
        }

        // preview
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("preview");
        ImGui::TableNextColumn();
        Widget_PreviewTexture_();

        // name
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("name");
        ImGui::TableNextColumn();
        ImGui::InputText("##ImageName", &m_name);

        // format conversion
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("format conversion");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##ConvertFormat", &m_options.bConvertFormat))
        {
            m_bPreviewLoadRequested = true;
        }

        if (m_options.bConvertFormat)
        {
            ImGui::Indent();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("format");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            if (Widget_TextureFormatCombo("##ConvertTextureFormat", m_options.convertFormat, true))
            {
                m_bPreviewLoadRequested = true;
            }
            ImGui::Unindent();
        }

        // force sRGB
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("force sRGB");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##ForceSrgb", &m_options.bForceSRGB))
        {
            m_bPreviewLoadRequested = true;
        }

        // resize
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("resize");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##Resize", &m_options.bResize))
        {
            m_bPreviewLoadRequested = true;
        }

        // resize options
        if (m_options.bResize)
        {
            ImGui::Indent();

            Vec2i size = { static_cast<Int32>(m_options.newWidth), static_cast<Int32>(m_options.newHeight) };

            ImGui::TableNextColumn();
            Widget_FrameAlignedText("width x heigth");
            ImGui::TableNextColumn();
            SetNextItemAvailWidth();
            if (Widget_DragInt2("##ResizeWidthHeight", &size.x, 0.05, 0, INT16_MAX))
            {
                m_options.newWidth      = size.x;
                m_options.newHeight     = size.y;
                m_bPreviewLoadRequested = true;
            }

            ImGui::Unindent();
        }

        // generate mips << not supported for preview

        // flip vertical
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("flip vertical");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##FlipV", &m_options.bFlipVertical))
        {
            m_bPreviewLoadRequested = true;
        }

        // flip horizontal
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("flip horizontal");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##FlipH", &m_options.bFlipHorizontal))
        {
            m_bPreviewLoadRequested = true;
        }

        // pre-multiply alpha
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("pre-multiply alpha");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##PreMultAlpha", &m_options.bPreMultiplyAlpha))
        {
            m_bPreviewLoadRequested = true;
        }

        ImGui::EndTable();
    }

    // buttons
    const ImVec2 btnSize = CalcFitButtonSize(2);
    if (ImGui::Button("import", btnSize))
    {
        if (OnClickImportButton_())
        {
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("cancel", btnSize))
    {
        ImGui::CloseCurrentPopup();
    }
}

void ImageImportDialog::LoadPreviewTexture_()
{
    m_pPreviewTex.Reset();

    m_importer.SetOptions(m_options);
    if (!m_importer.Import(m_dir))
    {
        // silently fail
        // because this is only for preview
        Log::Error("failed to import preview image '{}': {}", m_dir.native(), GlobalBuffer::GetLastError());
    }
}

void ImageImportDialog::Widget_PreviewTexture_() const
{
    constexpr ImVec2 k_previewSize = { 128.f, 128.f };
    if (m_pPreviewTex)
    {
        const auto [w, h]    = m_pPreviewTex->GetResolution();
        const ImVec2 texSize = { static_cast<float>(w), static_cast<float>(h) };
        Widget_ImageFrame(m_pPreviewTex, texSize, k_previewSize, { 0.f, 0.f }, { 1.f, 1.f }, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg), true);
    }
    else
    {
        Widget_BorderedText("no preview image", k_previewSize, true);
    }
}

bool ImageImportDialog::OnClickImportButton_()
{
    if (m_bPreviewLoadRequested)
    {
        LoadPreviewTexture_();
        m_bPreviewLoadRequested = false;
    }

    // check preview texture
    if (!m_pPreviewTex)
    {
        Editor::PushMessageDialog("import image error", "No image to import. Please select a valid image file.");
        return false;
    }

    return SaveAsset(m_pPreviewTex, TODO);
}

}   // namespace jam