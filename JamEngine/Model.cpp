#include "pch.h"

#include "Model.h"

#include "ModelLoader.h"

namespace jam
{

Model Model::Create(const std::span<const ModelElement> _parts)
{
    Model model;
    model.m_elements = std::vector<ModelElement>(_parts.begin(), _parts.end());
    return model;
}

Model Model::Create(std::span<const RawModelElement> _parts, eVertexType _vertexType, eTopology _topology)
{
    Model model;
    model.m_elements.reserve(_parts.size());
    for (const RawModelElement& partData: _parts)
    {
        MeshGeometry meshData;
        meshData.vertices = partData.rawMesh.vertices;
        meshData.indices  = partData.rawMesh.indices;

        ModelElement newPart;
        newPart.name     = partData.name;
        newPart.material = partData.material;
        newPart.mesh     = Mesh::Create(meshData, _vertexType, _topology);
        model.m_elements.emplace_back(newPart);
    }
    return model;
}

std::optional<Model> Model::CreateFromFile(const fs::path& _filePath, const eVertexType _vertexType, const eTopology _topology)
{
    ModelLoader importer;
    if (!importer.Load(_filePath))
    {
        JAM_ERROR("Failed to import model from file: {}", _filePath.string());
        return std::nullopt;
    }

    const std::vector<RawModelElement>& rawParts = importer.GetRawModelParts();
    if (rawParts.empty())
    {
        JAM_ERROR("No model parts found in imported model.");
        return std::nullopt;
    }

    Model model = Create(rawParts, _vertexType, _topology);
    return model;
}

}   // namespace jam