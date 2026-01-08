#include "pch.h"

#include "AssetUtilities.h"

#include "Application.h"
#include "Magics.h"

namespace jam
{

bool IsNativeAssetFile(
    const fs::path&  _path,
    const eAssetType _assetType)
{
    StringView      ext = _path.extension().native();
    Optional<Int32> dot = ext.FindLastOf('.');
    if (!dot)
    {
        return false;
    }

    ext = ext.SubView(dot.value());
    return ext == ExtensionOf(_assetType, true);
}

StringView ExtensionOf(
    const eAssetType _assetType,
    const bool       _bIncludeDot)
{
    StringView ext;
    switch (_assetType)
    {
        case eAssetType::ImageTexture:
            ext = k_jamImageTextureExt;
            break;
        case eAssetType::Material:
            ext = k_jamMaterialExt;
            break;
        case eAssetType::Mesh:
            ext = k_jamMeshExt;
            break;
        case eAssetType::Model:
            ext = k_jamModelExt;
            break;
        default:
            JAM_ASSERT(0, "Unhandled asset type in ExtensionFrom");
            ext = "";
            break;
    }

    if (_bIncludeDot)
    {
        return ext;
    }

    // exclude dot
    return ext.SubView(1);
}

}   // namespace jam