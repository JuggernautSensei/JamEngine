#include "pch.h"

#include "Mesh.h"

#include "Renderer.h"

namespace jam
{

void Mesh::Initialize(const MeshData& _meshData, const eVertexType _vertexType, const eTopology _topology)
{
    UInt32                              stride   = GetVertexStride(_vertexType);
    const std::vector<VertexAttribute>& vertices = _meshData.vertices;
    const std::vector<Index>&           indices  = _meshData.indices;

    UInt32 vertexCount = static_cast<UInt32>(vertices.size());
    UInt32 indexCount  = static_cast<UInt32>(indices.size());

    // vertex buffer
    {
        // vertex packing
        std::vector<UInt8> vertexData(vertexCount * stride);
        for (UInt32 i = 0; i < vertexCount; ++i)
        {
            const VertexAttribute& vertex = vertices[i];
            PackVertex(vertex, _vertexType, vertexData.data() + i * stride);
        }

        // initialize vertex buffer
        BufferInitData initData;
        initData.pData = vertexData.data();
        m_vertexBuffer.Initialize(GetVertexStride(_vertexType), vertexCount, eResourceAccess::Immutable, initData);
    }

    // index buffer
    {
        IndexBufferInitData initData;
        initData.pData = indices.data();
        m_indexBuffer.Initialize(indexCount, eResourceAccess::Immutable, initData);
    }

    // topology
    m_topology   = _topology;
    m_vertexType = _vertexType;
}

void Mesh::Bind() const
{
    m_vertexBuffer.Bind();
    m_indexBuffer.Bind();
    Renderer::BindTopology(static_cast<D3D11_PRIMITIVE_TOPOLOGY>(m_topology));
}

}   // namespace jam