#pragma once
#include "Material.h"
#include "Mesh.h"

namespace jam
{
class AssetManager;

struct ModelNodeData
{
    std::string name;
    MeshData    meshData;
    eVertexType vertexType;
    eTopology   topology;
    Material    material;
};

class Model
{
public:
    struct Node
    {
        std::string name;
        Mesh        mesh;
        Material    material;
    };

    void Initialize(std::span<const ModelNodeData> _nodes);
    bool LoadFromFile(AssetManager& _assetMgrRef, const fs::path& _filePath);
    bool SaveToFile(const fs::path& _filePath) const;

    void Reset();

    NODISCARD auto GetNodes() const { return std::span<const Node>(m_nodes); }
    NODISCARD auto GetNodesRef() { return std::span<Node>(m_nodes); }

private:
    std::vector<Node> m_nodes;
};

}   // namespace jam