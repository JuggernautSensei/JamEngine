#pragma once
#include "ModelImporter.h"
#include "Vertex.h"

namespace jam
{

class ModelExporter
{
public:
    void Load(std::span<const RawModelNode> _nodes, eTopology _topology, eVertexType _vertexType);
    bool Load(const Model& _model);
    bool Export(const fs::path& _path) const;

private:
    void Clear_();

    std::vector<RawModelNode> m_nodes;
    eTopology                 m_topology   = eTopology::TriangleList;
    eVertexType               m_vertexType = eVertexType::Vertex3;
    bool                      m_bLoaded    = false;
};

}   // namespace jam
