#pragma once
#include "ConfirmationDialog.h"
#include "Dialog.h"
#include "MessageDialog.h"

namespace jam
{
class IAsset;

class AssetDialog : public Dialog
{
public:
    explicit AssetDialog(StringView _title, ImVec2 _size = ImVec2 { 0.f, 0.f }, eDialogFlags _flags = eDialogFlags_None);

protected:
    void PushOverwriteConfirmationDialog_(const Ref<IAsset>& _pAsset, const fs::path& _path);
    void OnOverwriteConfirmationDialogOkButton_();

private:
    fs::path    m_overwriteFilename = {};
    fs::path    m_overwritePath     = {};
    Ref<IAsset> m_pOverwriteAsset   = nullptr;
};

}   // namespace jam
