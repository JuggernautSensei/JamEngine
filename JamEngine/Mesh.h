#pragma once
#include "Buffers.h"
#include "Vertex.h"

namespace jam
{

struct MeshData
{
    std::vector<VertexAttribute> vertices;   // vertex attributes
    std::vector<Index>           indices;    // index buffer
};

class Mesh
{
public:
    void Initialize(const MeshData& _meshData, eVertexType _vertexType, eTopology _topology);
    void Bind() const;

    const VertexBuffer& GetVertexBuffer() const { return m_vertexBuffer; }
    const IndexBuffer&  GetIndexBuffer() const { return m_indexBuffer; }

    void                  SetTopology(const eTopology _topology) { m_topology = _topology; }
    NODISCARD eTopology   GetTopology() const { return m_topology; }
    NODISCARD eVertexType GetVertexType() const { return m_vertexType; }

private:
    VertexBuffer m_vertexBuffer;
    IndexBuffer  m_indexBuffer;
    eVertexType  m_vertexType = eVertexType::Vertex3;

    // 토폴로지는 변경할 수 있음
    eTopology m_topology = eTopology::TriangleList;
};

}   // namespace jam