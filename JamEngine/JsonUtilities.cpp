#include "pch.h"

#include "JsonUtilities.h"

#include "AssetManager.h"

namespace jam
{

Json AssetToJson(const Ref<IAsset>& _pAssetOrNull)
{
    if (_pAssetOrNull)
    {
        return _pAssetOrNull->GetPath().string();
    }
    else
    {
        return nullptr;
    }
}

}   // namespace jam