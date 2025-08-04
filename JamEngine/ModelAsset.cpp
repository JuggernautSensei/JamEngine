#include "pch.h"

#include "ModelAsset.h"

#include "ModelExporter.h"
#include "ModelLoader.h"

namespace jam
{

bool ModelAsset::Save(const fs::path& _path) const
{
    if (!m_model.SaveToFile(_path))
    {
        JAM_ERROR("ModelAsset::Save() - Failed to save model to file: {}", _path.string());
        return false;
    }

    return true;
}

bool ModelAsset::Load(const fs::path& _path)
{
    if (!m_model.LoadFromFile(_path))
    {
        JAM_ERROR("ModelAsset::Load() - Failed to load model from file: {}", _path.string());
        return false;
    }

    m_path = _path;
    return true;
}

void ModelAsset::Unload()
{
    m_path.clear();
    m_model.Unload();
}

bool ModelAsset::IsLoaded() const
{
    return !m_path.empty();
}

eAssetType ModelAsset::GetType() const
{
    return eAssetType::Model;
}

}   // namespace jam