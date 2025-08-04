#pragma once
#include "Material.h"
#include "Mesh.h"

namespace jam
{

struct RawModelNode
{
    std::string  name;
    MeshGeometry meshGeometry;
    Material     material;
};

// for model runtime usage
struct ModelNode
{
    std::string name;
    Mesh        mesh;
    Material    material;
};

class Model
{
public:
    void Initialize(std::span<const RawModelNode> _parts, eVertexType _vertexType, eTopology _topology);
    bool SaveToFile(const fs::path& _filePath) const;
    bool LoadFromFile(const fs::path& _filePath);

    NODISCARD bool IsLoaded() const;
    void           Unload();

    NODISCARD eVertexType GetVertexType() const { return m_vertexType; }
    NODISCARD eTopology   GetTopology() const { return m_topology; }

    NODISCARD auto GetModelNodes() const { return std::span<const ModelNode>(m_nodes); }
    NODISCARD auto GetModelNodesRef() { return std::span<ModelNode>(m_nodes); }

private:
    std::vector<ModelNode> m_nodes;
    eVertexType            m_vertexType = eVertexType::Vertex3;
    eTopology              m_topology   = eTopology::TriangleList;
};

}   // namespace jam