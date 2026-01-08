#include "pch.h"
#include "AssetDialog.h"

#include "AssetUtilities.h"
#include "Editor.h"
#include "EditorUtilities.h"

namespace jam
{

AssetDialog::AssetDialog(
    const StringView   _title,
    const ImVec2       _size,
    const eDialogFlags _flags)
    : Dialog(_title, _size, _flags)
{
}

void AssetDialog::PushOverwriteConfirmationDialog_(
    const Ref<IAsset>& _pAsset,
    const fs::path&    _path)
{
    JAM_ASSERT(_pAsset, "asset is null.");
    JAM_ASSERT(!fs::is_directory(_path), "given path is a directory: {}", _path.native());

    // 다이얼로그 생성
    const StringView msg = GlobalBuffer::FormatTmp("an asset named '{}' already exists. do you want to overwrite it?", _path.filename().native());
    Editor::PushDialog(MakeScoped<ConfirmationDialog>("overwrite asset", msg, JAM_BIND_METHOD(this, AssetDialog::OnOverwriteConfirmationDialogOkButton_)));

    // 저장 정보 저장
    m_overwriteFilename = _path.filename();
    m_overwritePath     = _path;
    m_pOverwriteAsset   = _pAsset;
}

void AssetDialog::OnOverwriteConfirmationDialogOkButton_()
{
    JAM_ASSERT(!m_overwritePath.empty(), "overwrite path is empty.");
    JAM_ASSERT(m_pOverwriteAsset != nullptr, "overwrite asset is null.");

    // 저장
    if (!m_pOverwriteAsset->Save(m_overwritePath))
    {
        const eAssetType type = m_pOverwriteAsset->GetAssetType();
        const StringView name = EditorDisplayStringFrom(type);
        const StringView msg  = GlobalBuffer::FormatTmp("failed to save {} '{}'. {}.", name, m_overwritePath.native(), GlobalBuffer::GetLastError());
        Editor::PushMessageDialog("error", msg);
        return;
    }

    // 저장 성공시 Asset Dialog 종료
    Close();
}

}   // namespace jam