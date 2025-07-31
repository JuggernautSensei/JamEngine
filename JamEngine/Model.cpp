#include "pch.h"

#include "Model.h"

#include "ModelLoader.h"

namespace jam
{

void Model::Initialize(const std::span<const ModelElement> _parts)
{
    m_elements = std::vector<ModelElement>(_parts.begin(), _parts.end());
}

void Model::Initialize(const std::span<const RawModelElement> _parts, const eVertexType _vertexType, const eTopology _topology)
{
    m_elements.reserve(_parts.size());
    for (const RawModelElement& partData: _parts)
    {
        MeshGeometry meshData;
        meshData.vertices = partData.rawMesh.vertices;
        meshData.indices  = partData.rawMesh.indices;

        ModelElement newPart;
        newPart.name     = partData.name;
        newPart.material = partData.material;
        newPart.mesh.Initialize(meshData, _vertexType, _topology);
        m_elements.emplace_back(newPart);
    }
}

bool Model::LoadFromFile(const fs::path& _filePath, const eVertexType _vertexType, const eTopology _topology)
{
    ModelLoader importer;
    if (!importer.Load(_filePath))
    {
        JAM_ERROR("Failed to import model from file: {}", _filePath.string());
        return false;
    }

    const std::vector<RawModelElement>& rawParts = importer.GetRawModelParts();
    if (rawParts.empty())
    {
        JAM_ERROR("No model parts found in imported model.");
        return false;
    }

    Initialize(rawParts, _vertexType, _topology);
    return true;
}

}   // namespace jam