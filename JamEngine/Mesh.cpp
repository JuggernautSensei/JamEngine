#include "pch.h"

#include "Mesh.h"

#include "Renderer.h"

namespace jam
{

Mesh Mesh::Create(const MeshGeometry& _meshData, const eVertexType _vertexType, const eTopology _topology)
{
    Mesh mesh;

    const UInt32 stride      = GetVertexStride(_vertexType);
    const UInt32 vertexCount = static_cast<UInt32>(_meshData.vertices.size());
    const UInt32 indexCount  = static_cast<UInt32>(_meshData.indices.size());

    std::vector<UInt8> vertexData(vertexCount * stride);
    for (size_t i = 0; i < vertexCount; ++i)
    {
        const VertexAttribute& vertex = _meshData.vertices[i];
        PackVertex(vertex, _vertexType, vertexData.data() + i * stride);
    }

    mesh.m_vertexBuffer = VertexBuffer::Create(eResourceAccess::Immutable, _vertexType, vertexCount, vertexData.data());
    mesh.m_indexBuffer  = IndexBuffer::Create(eResourceAccess::Immutable, indexCount, _meshData.indices.data());
    mesh.m_topology     = _topology;
    return mesh;
}

void Mesh::Bind() const
{
    m_vertexBuffer.Bind();
    m_indexBuffer.Bind();
    Renderer::SetTopology(static_cast<D3D11_PRIMITIVE_TOPOLOGY>(m_topology));
}

}   // namespace jam