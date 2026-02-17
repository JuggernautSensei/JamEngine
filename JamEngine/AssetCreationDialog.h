#pragma once
#include "Dialog.h"

namespace Jam
{

class IAsset;

class AssetCreationDialog : public Dialog
{
public:
    explicit AssetCreationDialog(const DialogInfo& _desc);

protected:
    // utility
    void PushEmptyNameErrorMessageDialog_() const;
    void PushEmptySavePathErrorMessageDialog_() const;
    void PushNullVertexLayoutEntryErrorMessageDialog_() const;
    void PushOverwriteConfirmationDialog_(const Path& _path);
    void PushEmptyImportPathErrorMessageDialog_() const;
    bool SaveAsset_(const Path& _path, const Ref<IAsset>& _pAsset) const;

    // to be overridden
    virtual bool CreateAndSaveAsset_(const Path& _path) = 0;
};

}   // namespace Jam