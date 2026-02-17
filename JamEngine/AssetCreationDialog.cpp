#include "pch.h"

#include "AssetCreationDialog.h"

#include "ConfirmationDialog.h"
#include "Editor.h"
#include "IAsset.h"
#include "MessageDialog.h"

namespace Jam
{

AssetCreationDialog::AssetCreationDialog(
    const DialogInfo& _desc)
    : Dialog(_desc)
{
}

void AssetCreationDialog::PushEmptyNameErrorMessageDialog_() const
{
    MessageDialogInfo desc = {};
    desc.baseInfo.title    = "error";
    desc.msg               = "asset name cannot be empty.";
    JAM_EDITOR.PushDialog(MakeRef<MessageDialog>(desc));
}

void AssetCreationDialog::PushEmptySavePathErrorMessageDialog_() const
{
    MessageDialogInfo desc = {};
    desc.baseInfo.title    = "error";
    desc.msg               = "save path cannot be empty.";
    JAM_EDITOR.PushDialog(MakeRef<MessageDialog>(desc));
}

void AssetCreationDialog::PushNullVertexLayoutEntryErrorMessageDialog_() const
{
    MessageDialogInfo desc = {};
    desc.baseInfo.title    = "error";
    desc.msg               = "vertex layout cannot be null.";
    JAM_EDITOR.PushDialog(MakeRef<MessageDialog>(desc));
}

void AssetCreationDialog::PushOverwriteConfirmationDialog_(
    const Path& _path)
{
    auto onConfirm = [pSelf = Ref { this }, _path]
    {
        pSelf->CreateAndSaveAsset_(_path);
    };

    ScratchBuffer buf;
    buf.Format("file already exists: {}. do you want to overwrite it?", _path);

    ConfirmationDialogInfo desc = {};
    desc.baseInfo.title         = "confirmation";
    desc.msg                    = buf.ToStringView();
    desc.OnConfirm              = Bind(std::move(onConfirm));
    JAM_EDITOR.PushDialog(MakeRef<ConfirmationDialog>(std::move(desc)));
}

void AssetCreationDialog::PushEmptyImportPathErrorMessageDialog_() const
{
    MessageDialogInfo desc = {};
    desc.baseInfo.title    = "error";
    desc.msg               = "import path cannot be empty.";
    JAM_EDITOR.PushDialog(MakeRef<MessageDialog>(desc));
}

bool AssetCreationDialog::SaveAsset_(
    const Path&        _path,
    const Ref<IAsset>& _pAsset) const
{
    if (const Error err = _pAsset->Save(_path, false))
    {
        ScratchBuffer tmp;
        tmp.Format("failed to save asset file: {}. {}.", _path, err.ToErrorMessage());

        MessageDialogInfo desc = {};
        desc.baseInfo.title    = "error";
        desc.msg               = tmp.ToStringView();
        JAM_EDITOR.PushDialog(MakeRef<MessageDialog>(desc));
        return true;
    }

    return false;
}

}   // namespace Jam