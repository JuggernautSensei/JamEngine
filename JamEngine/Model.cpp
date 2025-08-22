#include "pch.h"

#include "Model.h"

#include "ModelExporter.h"
#include "ModelLoader.h"

namespace jam
{

void Model::Initialize(const std::span<const ModelNodeData> _nodes)
{
    m_nodes.reserve(_nodes.size());
    for (const ModelNodeData& node: _nodes)
    {
        Mesh mesh;
        mesh.Initialize(node.meshData, node.vertexType, node.topology);
        m_nodes.emplace_back(node.name, std::move(mesh), node.material);
    }
}

bool Model::LoadFromFile(AssetManager& _assetMgrRef, const fs::path& _filePath)
{
    ModelLoader loader;
    if (!loader.Load(_assetMgrRef, _filePath))
    {
        JAM_ERROR("Failed to load model from file: {}", _filePath.string());
        return false;
    }

    std::span<const ModelNodeData> loadData = loader.GetLoadData();
    Initialize(loadData);
    return true;
}

bool Model::SaveToFile(const fs::path& _filePath) const
{
    JAM_ASSERT(m_nodes.empty() == false, "Model m_modelNodes must not be empty before saving");

    ModelExporter exporter;
    exporter.Load(*this);
    if (!exporter.Export(_filePath))
    {
        JAM_ERROR("Failed to save model to file: {}", _filePath.string());
        return false;
    }
    return true;
}

void Model::Reset()
{
    m_nodes.clear();
}

}   // namespace jam