#include "pch.h"

#include "Model.h"

#include "ModelExporter.h"
#include "ModelLoader.h"

namespace jam
{

void Model::Initialize(const std::span<const RawModelNode> _parts, const eVertexType _vertexType, const eTopology _topology)
{
    m_nodes.reserve(_parts.size());
    for (const RawModelNode& partData: _parts)
    {
        MeshGeometry geo;
        geo.vertices = partData.meshGeometry.vertices;
        geo.indices  = partData.meshGeometry.indices;

        ModelNode nodes;
        nodes.name     = partData.name;
        nodes.material = partData.material;
        nodes.mesh.Initialize(geo, _vertexType, _topology);
        m_nodes.emplace_back(nodes);
    }
}

bool Model::LoadFromFile(const fs::path& _filePath)
{
    ModelLoader loader;
    if (!loader.Load(_filePath))
    {
        JAM_ERROR("Failed to load model from file: {}", _filePath.string());
        return false;
    }

    const ModelLoadData& loadData = loader.GetLoadData();
    Initialize(loadData.nodes, loadData.vertexType, loadData.topology);
    return true;
}

bool Model::SaveToFile(const fs::path& _filePath) const
{
    JAM_ASSERT(IsLoaded(), "Model::SaveToFile() - Model is not loaded.");

    ModelExporter exporter;
    exporter.Load(*this);
    if (!exporter.Export(_filePath))
    {
        JAM_ERROR("Failed to save model to file: {}", _filePath.string());
        return false;
    }
    return true;
}

bool Model::IsLoaded() const
{
    return !m_nodes.empty();
}

void Model::Unload()
{
    m_nodes.clear();
}

}   // namespace jam