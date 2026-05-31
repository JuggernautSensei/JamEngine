#include "pch.h"

#include "ImageImportDialog.h"

#include "Editor.h"
#include "EditorUtilities.h"
#include "EditorWidgets.h"
#include "FileDialog.h"
#include "Graphics.h"
#include "GraphicsBase.h"

namespace Jam
{

namespace
{
    [[nodiscard]] FileDialog CreateImportFileDialog_()
    {
        constexpr FileDialogFilter kFilters[] = {
            { "image files", "*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.gif;*.dds;*.hdr;*.psd;*.tiff" }
        };
        OpenFileDialogInfo info = {};
        info.title              = "select image file";
        info.filtersOrEmpty     = kFilters;
        return FileDialog { info };
    }

    [[nodiscard]] FileDialog CreateSaveFileDialog_()
    {
        SaveFileDialogInfo info = {};
        info.title              = "select save directory";
        return FileDialog { info };
    }

}   // namespace

ImageImportDialog::ImageImportDialog(
    const ImageImportDialogInfo& _info)
    : AssetCreationDialog(_info.title, _info.size)
    , m_importPath(_info.importPathOrEmpty)
    , m_importFd(CreateImportFileDialog_())
    , m_saveFd(CreateSaveFileDialog_())
    , m_pPreviewTex(nullptr)
{
    if (!_info.importPathOrEmpty.IsEmpty())
    {
        m_assetSaveDir  = _info.importPathOrEmpty.GetDirectory();
        m_assetBaseName = _info.importPathOrEmpty.GetBaseName();
        LoadPreviewTexture_();
    }

    // default import option
    m_options.bGenerateMips = true;
}

void ImageImportDialog::OnShow()
{
    // preview load if need
    ImGuiIO& io = ImGui::GetIO();
    m_previewLoadCounterSec += io.DeltaTime;
    if (m_bPreviewLoadRequested && m_previewLoadCounterSec >= kPreviewLoadDebounceSec)
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
        Widget_Text("import path", true);
        ImGui::TableNextColumn();
        if (Widget_FileDialog("##ImagePathSelector", m_importPath, m_importFd))
        {
            m_bPreviewLoadRequested = true;
        }

        // preview
        ImGui::TableNextColumn();
        Widget_Text("preview", true);
        ImGui::TableNextColumn();
        Widget_PreviewTexture_();

        // output directory
        ImGui::TableNextColumn();
        Widget_Text("output directory", true);
        ImGui::TableNextColumn();
        Widget_FileDialog("##ImageSaveDirSelector", m_assetSaveDir, m_saveFd);

        // name
        ImGui::TableNextColumn();
        Widget_Text("asset name", true);
        ImGui::TableNextColumn();
        Widget_InputText("##ImageName", m_assetBaseName);

        // format conversion
        ImGui::TableNextColumn();
        Widget_Text("format conversion", true);
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##ConvertFormat", &m_options.bConvertFormat))
        {
            m_bPreviewLoadRequested = true;
        }

        if (m_options.bConvertFormat)
        {
            ImGui::Indent();
            ImGui::TableNextColumn();
            Widget_Text("format", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_TextureFormatCombo_("##ConvertTextureFormat", m_options.convertFormat))
            {
                m_bPreviewLoadRequested = true;
            }
            ImGui::Unindent();
        }

        // force sRGB
        ImGui::TableNextColumn();
        Widget_Text("force sRGB", true);
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##ForceSrgb", &m_options.bForceSRGB))
        {
            m_bPreviewLoadRequested = true;
        }

        // resize
        ImGui::TableNextColumn();
        Widget_Text("resize", true);
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
            Widget_Text("width x heigth", true);
            ImGui::TableNextColumn();
            SetNextItemToAvailWidth();
            if (Widget_DragInt2("##ResizeWidthHeight", &size.x, 0.05f, 0, INT16_MAX))
            {
                m_options.newWidth      = size.x;
                m_options.newHeight     = size.y;
                m_bPreviewLoadRequested = true;
            }

            ImGui::Unindent();
        }

        // flip vertical
        ImGui::TableNextColumn();
        Widget_Text("flip vertical", true);
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##FlipV", &m_options.bFlipVertical))
        {
            m_bPreviewLoadRequested = true;
        }

        // flip horizontal
        ImGui::TableNextColumn();
        Widget_Text("flip horizontal", true);
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##FlipH", &m_options.bFlipHorizontal))
        {
            m_bPreviewLoadRequested = true;
        }

        // pre-multiply alpha
        ImGui::TableNextColumn();
        Widget_Text("pre-multiply alpha", true);
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

Error ImageImportDialog::LoadPreviewTexture_()
{
    // import
    Expect<ImageImporter> importer = ImageImporter::ImportFromFile(m_importPath, m_options);
    if (!importer)
    {
        return importer.GetError();
    }

    // create texture
    Graphics&           gfx    = GetGraphics();
    const TextureHandle texh = gfx.CreateTexture(importer->GetImageContainer());

    // set preview texture
    m_pPreviewTex = Texture::Load(texh);
    return Error::None;
}

bool ImageImportDialog::OnClickImportButton_()
{
    if (m_assetBaseName.IsEmpty())
    {
        PushEmptyNameErrorMessageDialog_();
        return false;
    }

    if (m_importPath.IsEmpty())
    {
        PushEmptyImportPathErrorMessageDialog_();
        return false;
    }

    if (m_assetSaveDir.IsEmpty())
    {
        PushEmptySavePathErrorMessageDialog_();
        return false;
    }

    String path = String::Joined(m_assetSaveDir, m_assetBaseName, eJoin::File);
    path.ReplaceExtension(Texture::kExtension);

    if (FileSystem::Exists(path))
    {
        PushOverwriteConfirmationDialog_(path);
        return false;
    }

    return CreateAndSaveAsset_(path);
}

bool ImageImportDialog::CreateAndSaveAsset_(
    const StringView _path)
{
    // use preview texture for saving
    if (m_bPreviewLoadRequested)
    {
        if (const Error err = LoadPreviewTexture_())
        {
            GetEditor().PushMessageDialog(
                "error",
                String::Formatted("failed to load preview texture for '{}': {}", m_importPath, err.ToErrorMessage()));

            return false;
        }
        m_bPreviewLoadRequested = false;
    }

    // create texture asset
    return SaveAsset_(_path, m_pPreviewTex);
}

void ImageImportDialog::Widget_PreviewTexture_() const
{
    constexpr ImVec2 kPreviewSize = { 128.f, 128.f };
    if (m_pPreviewTex)
    {
        const TextureInfo& info = m_pPreviewTex->GetInfo();
        const ImVec2       size = { static_cast<float>(info.width), static_cast<float>(info.height) };
        Widget_ImageFrame(m_pPreviewTex, size, kPreviewSize, { 0.f, 0.f }, { 1.f, 1.f }, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg), true);
    }
    else
    {
        Widget_TextFrame("no preview image", kPreviewSize, {}, true);
    }
}

bool ImageImportDialog::Widget_TextureFormatCombo_(
    const char*     _label,
    eTextureFormat& _format) const
{
    const char* preview  = NameOf(_format).GetPtr();
    bool        bChanged = false;
    if (ImGui::BeginCombo(_label, preview, ImGuiComboFlags_NoArrowButton))
    {
        for (const eTextureFormat type: RangesOf<eTextureFormat>())
        {
            // exclude depth & unknown
            if (IsDepthFormat(type) || type == eTextureFormat::Unknown)
            {
                continue;
            }

            const bool  bSelected = (type == _format);
            const char* typeName  = NameOf(type).GetPtr();
            if (ImGui::Selectable(typeName, bSelected))
            {
                _format  = type;
                bChanged = true;
            }

            if (bSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    return bChanged;
}

}   // namespace Jam