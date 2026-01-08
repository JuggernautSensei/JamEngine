#pragma once
#include "IAsset.h"

namespace jam
{

[[nodiscard]] bool       IsNativeAssetFile(const fs::path& _path, eAssetType _assetType);
[[nodiscard]] StringView ExtensionOf(eAssetType _assetType, bool _bIncludeDot = false);

}   // namespace jam
