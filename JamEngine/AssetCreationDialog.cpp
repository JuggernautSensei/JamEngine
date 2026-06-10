#include "pch.h"

#include "AssetCreationDialog.h"

#include "Editor.h"
#include "IAsset.h"
#include "MessageDialog.h"

namespace Jam
{

AssetCreationDialog::AssetCreationDialog(
    const StringView _title,
    const ImVec2     _size)
    : Dialog(_title, _size)
{
}

void AssetCreationDialog::PushEmptyNameErrorMessageDialog_() const
{
    GetEditor().PushMessageDialog("error", "name cannot be empty.");
}

void AssetCreationDialog::PushEmptySavePathErrorMessageDialog_() const
{
    GetEditor().PushMessageDialog("error", "save path cannot be empty.");
}

void AssetCreationDialog::PushNullVertexLayoutEntryErrorMessageDialog_() const
{
    GetEditor().PushMessageDialog("error", "vertex layout entry cannot be null.");
}

void AssetCreationDialog::PushOverwriteConfirmationDialog_(
    StringView _path)
{
    const Delegate<void()> OnConfirm = Bind(
        [pSelf = Ref { this }, _path] {
            pSelf->CreateAndSaveAsset_(_path);
        });

    GetEditor().PushConfirmationDialog(
        "overwrite confirmation",
        String::Formatted("file already exists at path: {}. Do you want to overwrite it?", _path),
        OnConfirm);
}

void AssetCreationDialog::PushEmptyImportPathErrorMessageDialog_() const
{
    GetEditor().PushMessageDialog("error", "import path cannot be empty.");
}

bool AssetCreationDialog::SaveAsset_(
    const StringView   _path,
    const Ref<IAsset>& _pAsset) const
{
    if (const Error err = _pAsset->Save(_path))
    {
        GetEditor().PushMessageDialog(
            "error",
            String::Formatted("failed to save asset at path: {}.\n{}", _path, err));

        return true;
    }

    return false;
}

}   // namespace Jam