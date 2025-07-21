#pragma once
#include "Buffers.h"
#include "Vertex.h"

namespace jam
{

struct MeshGeometry
{
    std::vector<VertexAttribute> vertices;
    std::vector<Index>           indices;
};

class Mesh
{
public:
    static Mesh Create(const MeshGeometry& _meshData, eVertexType _vertexType, eTopology _topology);
    void        Bind() const;

    const VertexBuffer& GetVertexBuffer() const { return m_vertexBuffer; }
    const IndexBuffer&  GetIndexBuffer() const { return m_indexBuffer; }
    eTopology           GetTopology() const { return m_topology; }

private:
    VertexBuffer m_vertexBuffer;
    IndexBuffer  m_indexBuffer;
    eTopology    m_topology = eTopology::TriangleList;
};

}   // namespace jam