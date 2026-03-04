#pragma once
#include "Dialog.h"

namespace Jam
{

class IAsset;

class AssetCreationDialog : public Dialog
{
public:
    explicit AssetCreationDialog(StringView _title, ImVec2 _size);

protected:
    // utility
    void PushEmptyNameErrorMessageDialog_() const;
    void PushEmptySavePathErrorMessageDialog_() const;
    void PushNullVertexLayoutEntryErrorMessageDialog_() const;
    void PushOverwriteConfirmationDialog_(StringView _path);
    void PushEmptyImportPathErrorMessageDialog_() const;
    bool SaveAsset_(StringView _path, const Ref<IAsset>& _pAsset) const;

    // to be overridden
    virtual bool CreateAndSaveAsset_(StringView _path) = 0;
};

}   // namespace Jam