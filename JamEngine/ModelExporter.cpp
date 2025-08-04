#include "pch.h"

#include "ModelExporter.h"

#include "BufferReader.h"
#include "Model.h"
#include "flatbuffers/compiled/model_generated.h"

#include <fstream>

namespace
{

NODISCARD jam::fbs::Vec3 ToFlatBuffersVec3(const jam::Vec3& vec)
{
    return { vec.x, vec.y, vec.z };
}

NODISCARD jam::fbs::Vec2 ToFlatBuffersVec2(const jam::Vec2& vec)
{
    return { vec.x, vec.y };
}

NODISCARD jam::fbs::eVertexType ToFlatBuffersVertexType(const jam::eVertexType vertexType)
{
    switch (vertexType)
    {
        case jam::eVertexType::Vertex2: return jam::fbs::eVertexType_Vertex2;
        case jam::eVertexType::Vertex3: return jam::fbs::eVertexType_Vertex3;
        case jam::eVertexType::Vertex3PosOnly: return jam::fbs::eVertexType_Vertex3PosOnly;
        default:
            JAM_ERROR("Unknown vertex type");
            return jam::fbs::eVertexType_Vertex3;   // Default fallback
    }
}

NODISCARD jam::fbs::eTopology ToFlatBuffersTopology(const jam::eTopology topology)
{
    switch (topology)
    {
        case jam::eTopology::Undefined: return jam::fbs::eTopology_Undefined;
        case jam::eTopology::PointList: return jam::fbs::eTopology_PointList;
        case jam::eTopology::LineList: return jam::fbs::eTopology_LineList;
        case jam::eTopology::LineStrip: return jam::fbs::eTopology_LineStrip;
        case jam::eTopology::TriangleList: return jam::fbs::eTopology_TriangleList;
        case jam::eTopology::TriangleStrip: return jam::fbs::eTopology_TriangleStrip;
        default:
            JAM_ERROR("Unknown topology type");
            return jam::fbs::eTopology_TriangleList;   // Default fallback
    }
}

}   // namespace

namespace jam
{

void ModelExporter::Load(std::span<const RawModelNode> _nodes, const eTopology _topology, const eVertexType _vertexType)
{
    Clear_();
    m_nodes      = std::vector(_nodes.begin(), _nodes.end());
    m_topology   = _topology;
    m_vertexType = _vertexType;
    m_bLoaded    = true;
}

bool ModelExporter::Load(const Model& _model)
{
    Clear_();

    m_topology   = _model.GetTopology();
    m_vertexType = _model.GetVertexType();

    // for gpu data download
    BufferReader reader;

    for (const ModelNode& node: _model.GetModelNodes())
    {
        RawModelNode rawNode;

        // 기본 데이터 복사
        rawNode.name     = node.name;
        rawNode.material = node.material;

        // 메쉬 기하 추출
        {
            MeshGeometry& rawMeshRef = rawNode.meshGeometry;

            // 버텍스 버퍼
            const VertexBuffer&               vb     = node.mesh.GetVertexBuffer();
            std::optional<std::vector<UInt8>> vbData = reader.ReadBuffer(vb);

            if (!vbData)
            {
                return false;
            }

            const std::vector<UInt8>& vbRaw        = *vbData;
            const UInt32              vertexStride = vb.GetStride();
            JAM_ASSERT(vbData->size() % vertexStride == 0, "Vertex buffer size is not a multiple of vertex stride.");
            UInt32 vertexCount = static_cast<UInt32>(vbRaw.size() / vertexStride);
            rawMeshRef.vertices.reserve(vertexCount);
            for (UInt32 i = 0; i < vertexCount; ++i)
            {
                VertexAttribute vertex;
                UnpackVertex(m_vertexType, vbRaw.data() + i * vertexStride, vertex);
                rawMeshRef.vertices.emplace_back(vertex);
            }

            // 인덱스 버퍼
            const IndexBuffer&                ib     = node.mesh.GetIndexBuffer();
            std::optional<std::vector<UInt8>> ibData = reader.ReadBuffer(ib);

            if (!ibData)
            {
                return false;
            }

            const std::vector<UInt8>& ibRaw = *ibData;
            JAM_ASSERT(ibRaw.size() % sizeof(Index) == 0, "Index buffer size is not a multiple of index size.");
            UInt32 indexCount = static_cast<UInt32>(ibRaw.size() / sizeof(Index));
            rawMeshRef.indices.reserve(indexCount);
            for (UInt32 i = 0; i < indexCount; ++i)
            {
                Index index;
                std::memcpy(&index, ibRaw.data() + i * sizeof(Index), sizeof(Index));
                rawMeshRef.indices.emplace_back(index);
            }
        }

        m_nodes.emplace_back(rawNode);
    }

    m_bLoaded = true;
    return true;
}

bool ModelExporter::Export(const fs::path& _path) const
{
    using namespace flatbuffers;

    if (!m_bLoaded)
    {
        JAM_ERROR("ModelExporter::Export() - Model not loaded yet.");
        return false;
    }

    FlatBufferBuilder                      builder;
    std::vector<Offset<fbs::RawModelNode>> parts;
    parts.reserve(m_nodes.size());
    for (const RawModelNode& part: m_nodes)
    {
        // name
        const Offset<String> nameOffset = builder.CreateString(part.name);

        // mesh
        const MeshGeometry&                       rawMesh = part.meshGeometry;
        std::vector<Offset<fbs::VertexAttribute>> vertices;
        vertices.reserve(part.meshGeometry.vertices.size());
        for (const VertexAttribute& vertex: rawMesh.vertices)
        {
            fbs::Vec3 position  = ToFlatBuffersVec3(vertex.position);
            fbs::Vec3 color     = ToFlatBuffersVec3(vertex.color);
            fbs::Vec3 normal    = ToFlatBuffersVec3(vertex.normal);
            fbs::Vec2 uv0       = ToFlatBuffersVec2(vertex.uv0);
            fbs::Vec2 uv1       = ToFlatBuffersVec2(vertex.uv1);
            fbs::Vec3 tangent   = ToFlatBuffersVec3(vertex.tangent);
            fbs::Vec3 bitangent = ToFlatBuffersVec3(vertex.bitangent);
            vertices.emplace_back(fbs::CreateVertexAttribute(builder, &position, &color, &normal, &uv0, &uv1, &tangent, &bitangent));
        }
        const Offset<Vector<Offset<fbs::VertexAttribute>>> verticesOffset = builder.CreateVector(vertices);
        const Offset<Vector<UInt32>>                       indicesOffset  = builder.CreateVector(rawMesh.indices);
        const Offset<fbs::MeshGeometry>                    meshOffset     = fbs::CreateMeshGeometry(builder, verticesOffset, indicesOffset);

        // material
        const Material&             material       = part.material;
        fbs::Vec3                   ambientColor   = ToFlatBuffersVec3(material.ambientColor);
        fbs::Vec3                   diffuseColor   = ToFlatBuffersVec3(material.diffuseColor);
        fbs::Vec3                   specularColor  = ToFlatBuffersVec3(material.specularColor);
        fbs::Vec3                   albedoColor    = ToFlatBuffersVec3(material.albedoColor);
        fbs::Vec3                   emissiveColor  = ToFlatBuffersVec3(material.emissiveColor);
        const Offset<fbs::Material> materialOffset = fbs::CreateMaterial(builder, &ambientColor, &diffuseColor, &specularColor, material.shininess, &albedoColor, material.metallic, material.roughness, material.ao, material.emissive, &emissiveColor, material.emissiveScale, material.displacementScale);

        parts.emplace_back(fbs::CreateRawModelNode(builder, nameOffset, meshOffset, materialOffset));
    }

    const Offset<Vector<Offset<fbs::RawModelNode>>> modelNodes = builder.CreateVector(parts);
    const Offset<fbs::RawModel>                     rawModel   = fbs::CreateRawModel(builder, modelNodes, ToFlatBuffersTopology(m_topology), ToFlatBuffersVertexType(m_vertexType));
    builder.Finish(rawModel);

    std::fstream fs(_path, std::ios::out | std::ios::binary);
    if (!fs.is_open())
    {
        JAM_ERROR("ModelExporter::Export() - Failed to open file: {}", _path.string());
        return false;
    }

    fs.write(reinterpret_cast<const char*>(builder.GetBufferPointer()), builder.GetSize());
    return true;
}

void ModelExporter::Clear_()
{
    *this = ModelExporter();
}

}   // namespace jam