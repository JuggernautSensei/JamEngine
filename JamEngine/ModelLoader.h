#pragma once
#include "Model.h"

namespace jam
{
class AssetManager;

class ModelLoader
{
public:
    bool           Load(AssetManager& _assetMgrRef, const fs::path& _path);
    NODISCARD auto GetLoadData() const { return std::span<const ModelNodeData>(m_modelNodes); }
    NODISCARD bool IsLoaded() const { return !m_modelNodes.empty(); }

private:
    void Clear_();

    std::vector<ModelNodeData> m_modelNodes;
};

}   // namespace jam