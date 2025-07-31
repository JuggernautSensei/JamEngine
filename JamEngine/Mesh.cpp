#include "pch.h"

#include "Mesh.h"

#include "Renderer.h"

namespace jam
{

void Mesh::Initialize(const MeshGeometry& _meshData, const eVertexType _vertexType, const eTopology _topology)
{
    const UInt32 stride      = GetVertexStride(_vertexType);
    const UInt32 vertexCount = static_cast<UInt32>(_meshData.vertices.size());
    const UInt32 indexCount  = static_cast<UInt32>(_meshData.indices.size());

    // vertex buffer
    {
        // vertex packing
        std::vector<UInt8> vertexData(vertexCount * stride);
        for (size_t i = 0; i < vertexCount; ++i)
        {
            const VertexAttribute& vertex = _meshData.vertices[i];
            PackVertex(vertex, _vertexType, vertexData.data() + i * stride);
        }

        // initialize vertex buffer
        BufferInitializeData initData;
        initData.pData = vertexData.data();
        m_vertexBuffer.Initialize(GetVertexStride(_vertexType), vertexCount, eResourceAccess::Immutable, initData);
    }

    // index buffer
    {
        IndexBufferInitializeData initData;
        initData.pData = _meshData.indices.data();
        m_indexBuffer.Initialize(indexCount, eResourceAccess::Immutable, initData);
    }

    // topology
    m_topology = _topology;
}

void Mesh::Bind() const
{
    m_vertexBuffer.Bind();
    m_indexBuffer.Bind();
    Renderer::BindTopology(static_cast<D3D11_PRIMITIVE_TOPOLOGY>(m_topology));
}

}   // namespace jam