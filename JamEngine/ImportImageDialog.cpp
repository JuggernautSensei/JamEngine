#include "pch.h"

#include "ImportImageDialog.h"

#include "WindowsUtilities.h"
#include "EditorWidgets.h"
#include "Graphics.h"
#include "GraphicsBase.h"
#include "ImGuiUtilities.h"
#include "ImageExporter.h"
#include "ImageImporter.h"

namespace jam
{

ImportImageDialog::ImportImageDialog(const fs::path& _path)
{
    m_parentDir = fs::is_directory(_path) ? _path : _path.parent_path();

    if (!fs::is_directory(_path))
    {
        // load preview texture immediately
        m_importPath = _path;
        LoadPreviewTexture_();
    }
}

void ImportImageDialog::operator()()
{
    // preview loader
    m_previewLoadTimer += ImGui::GetIO().DeltaTime;
    if (m_bDirty && m_previewLoadTimer >= k_previewLoadDelay)
    {
        LoadPreviewTexture_();
        m_bDirty           = false;
        m_previewLoadTimer = 0.f;
    }

    // table
    if (ImGui::BeginTable("ImportTextureDialogTable", 2, ImGuiTableFlags_SizingFixedFit, { 700.f, 0.f }))
    {
        constexpr FileDialogFilter k_filters[] = {
            { "image files", "*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.gif;*.dds;*.hdr;*.psd;*.tiff" },
            {   "all files",                                                           "*.*" }
        };

        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        // Path selection
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("path");
        ImGui::TableNextColumn();
        if (Widget_SingleFileDialog("##FileDialog", m_importPath, TODO))
            m_bDirty = true;

        // Preview
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("preview");
        ImGui::TableNextColumn();
        Widget_PreviewTexture_();

        // Name & Extension
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("name");
        ImGui::TableNextColumn();
        Widget_NameInput_();

        // Convert format
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("convert format");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##ConvertFormat", &m_bConvertFormat))
            m_bDirty = true;

        if (m_bConvertFormat)
        {
            ImGui::Indent();
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("format");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (Widget_TextureFormatCombo("##ConvertTextureFormat", m_convertFormat, true))
                m_bDirty = true;

            ImGui::Unindent();
        }

        // Force sRGB
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("force sRGB");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##forceSrgb", &m_bForceSrgb))
            m_bDirty = true;

        // Resize
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("resize");
        ImGui::TableNextColumn();
        ImGui::Checkbox("##resize", &m_bResize);

        // resize option
        if (m_bResize)
        {
            ImGui::Indent();

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("width");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (ImGui::InputScalar("##resizeWidth", ImGuiDataType_U64, &m_resizeW))
                m_bDirty = true;

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            Widget_FrameAlignedText("height");
            ImGui::TableNextColumn();
            SetNextItemWidthToAvailableWidth();
            if (ImGui::InputScalar("##resizeHeight", ImGuiDataType_U64, &m_resizeH))
                m_bDirty = true;

            ImGui::Unindent();
        }

        // Generate mips
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("drop mips");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##dropMips", &m_bDropMips))
            m_bDirty = true;

        // Flip vertical
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("flip vertical");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##flipV", &m_bFlipV))
            m_bDirty = true;

        // Flip horizontal
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("flip horizontal");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##flipH", &m_bFlipH))
            m_bDirty = true;

        // Pre-multiply alpha
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        Widget_FrameAlignedText("pre-multiply alpha");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##preMultAlpha", &m_bPreMultAlpha))
            m_bDirty = true;

        ImGui::EndTable();
    }

    // buttons
    {
        const ImVec2 btnSize = CalcFitButtonSize(2);

        if (ImGui::Button("import", btnSize))
        {
            if (OnClickImportButton_())
                ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("cancel", btnSize))
            ImGui::CloseCurrentPopup();
    }

    m_popup.Show();
}

bool ImportImageDialog::OnClickImportButton_()
{
    // reload if dirty
    if (m_bDirty)
    {
        LoadPreviewTexture_();
        m_bDirty = false;
    }

    // check preview texture
    if (!m_pPreviewTex)
    {
        m_popup.Open("import texture error", "No texture to import. Please select a valid image file.");
        return false;
    }

    // Validation: Check filename
    if (m_nameBuf.empty())
    {
        m_popup.Open("import texture error", "Texture name cannot be empty.");
        return false;
    }

    // Validation: Check extension
    if (m_imageFormat == eImageFormat::Unknown)
    {
        m_popup.Open("import texture error", "Please select a valid output format.");
        return false;
    }

    // build output
    fs::path outputPath = m_parentDir / m_nameBuf;
    outputPath.replace_extension(StringFrom(m_imageFormat));

    // Validation: Check extension
    if (outputPath.extension().empty())
    {
        m_popup.Open("import texture error", "File extension is missing. Please select an output format.");
        return false;
    }

    // Validation: Check if file exists
    if (fs::exists(outputPath))
    {
        m_popup.Open("import texture error",
                     std::format("Target file already exists:\n{}", outputPath.native()));
        return false;
    }

    // Export texture
    ImageExporter exporter;
    exporter.SetPath(outputPath);
    exporter.SetTexture(m_pPreviewTex->GetHandle());

    if (!exporter.Export())
    {
        const std::string msg = std::format("Failed to save texture:\n{}\n\nError: {}", outputPath.native(), GlobalBuffer::GetLastError());
        m_popup.Open("import texture error", msg);

        return false;
    }

    return true;
}

void ImportImageDialog::LoadPreviewTexture_()
{
    ImageImporter importer;
    importer.SetPath(m_importPath);

    // apply import options in correct order
    importer.ForceSRGB(m_bForceSrgb);
    importer.Resize(m_bResize, m_resizeW, m_resizeH);
    importer.GenerateMips(m_bDropMips);
    importer.FlipVertical(m_bFlipV);
    importer.FlipHorizontal(m_bFlipH);
    importer.PreMultiplyAlpha(m_bPreMultAlpha);
    importer.ConvertFormat(m_bConvertFormat, m_convertFormat);

    if (!importer.Import())
    {
        m_popup.Open("error", std::format("failed to import image: {}\n{}", m_importPath.native(), GlobalBuffer::GetLastError()));
        return;
    }

    const TextureHandle texh = Graphics::LoadTexture(importer.GetScratchImage());
    m_pPreviewTex            = MakeRef<PreviewTexture>();
    m_pPreviewTex->Init(texh);
}

void ImportImageDialog::Widget_PreviewTexture_() const
{
    if (m_pPreviewTex)
    {
        const TextureInfo& texInfo = m_pPreviewTex->GetInfo();
        const float        aspect  = static_cast<float>(texInfo.width) / static_cast<float>(texInfo.height);
        const ImVec2       size    = aspect >= 1.f ? ImVec2(k_previewSize.x, k_previewSize.x / aspect)
                                                   : ImVec2(k_previewSize.y * aspect, k_previewSize.y);

        ImGui::ImageWithBg(ImTextureRefFrom(m_pPreviewTex->GetHandle()), size);

        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("resolution: %d x %d x %d", texInfo.width, texInfo.height, texInfo.depth);
            ImGui::Text("format: %s", StringFrom(texInfo.format).data());
            ImGui::Text("texture type: %s", StringFrom(texInfo.type).data());
            ImGui::Text("array size: %d", texInfo.arraySize);
            ImGui::Text("samples: %d", texInfo.samples);
            ImGui::Text("mip levels: %d", texInfo.numMips);
            ImGui::EndTooltip();
        }
    }
    else
    {
        Widget_BorderedText("no image", k_previewSize);
    }
}

bool ImportImageDialog::Widget_NameInput_()
{
    constexpr float k_selectorWidth = 100.f;

    const ImGuiStyle& style     = ImGui::GetStyle();
    const float       itemWidth = ImGui::GetContentRegionAvail().x - k_selectorWidth - style.ItemSpacing.x;
    bool              bDirty    = false;

    ImGui::SetNextItemWidth(itemWidth);
    bDirty |= ImGui::InputText("##TextureName", &m_nameBuf);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(k_selectorWidth);
    bDirty |= Widget_ImageFormatCombo("##Extension", m_imageFormat);
    return bDirty;
}

}   // namespace jam