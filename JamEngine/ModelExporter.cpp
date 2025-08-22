#include "pch.h"

#include "ModelExporter.h"

#include "BufferReader.h"
#include "Model.h"
#include "TextureAsset.h"
#include "vendor/flatbuffers/compiled/model_generated.h"

#include <fstream>

namespace
{

using namespace jam;

NODISCARD fbs::Vec3 ToFlatBuffersVec3(const Vec3& vec)
{
    return { vec.x, vec.y, vec.z };
}

NODISCARD fbs::Vec2 ToFlatBuffersVec2(const Vec2& vec)
{
    return { vec.x, vec.y };
}

NODISCARD fbs::eVertexType ToFlatBuffersVertexType(const eVertexType vertexType)
{
    switch (vertexType)
    {
        case eVertexType::Vertex2: return fbs::eVertexType_Vertex2;
        case eVertexType::Vertex3: return fbs::eVertexType_Vertex3;
        case eVertexType::Vertex3PosOnly: return fbs::eVertexType_Vertex3PosOnly;
        default:
            JAM_ERROR("Unknown vertex type");
            return fbs::eVertexType_Vertex3;   // Default fallback
    }
}

NODISCARD fbs::eTopology ToFlatBuffersTopology(const eTopology topology)
{
    switch (topology)
    {
        case eTopology::Undefined: return fbs::eTopology_Undefined;
        case eTopology::PointList: return fbs::eTopology_PointList;
        case eTopology::LineList: return fbs::eTopology_LineList;
        case eTopology::LineStrip: return fbs::eTopology_LineStrip;
        case eTopology::TriangleList: return fbs::eTopology_TriangleList;
        case eTopology::TriangleStrip: return fbs::eTopology_TriangleStrip;
        default:
            JAM_ERROR("Unknown topology type");
            return fbs::eTopology_TriangleList;   // Default fallback
    }
}

NODISCARD flatbuffers::Offset<flatbuffers::String> ToFlatBuffersString(flatbuffers::FlatBufferBuilder& builder, const std::optional<Ref<TextureAsset>>& _texture)
{
    if (_texture)
    {
        const Ref<TextureAsset>& texture = *_texture;
        return builder.CreateString(texture->GetPath().string());
    }
    else
    {
        return 0;   // 빈 문자열
    }
}

}   // namespace

namespace jam
{

void ModelExporter::Load(std::span<const ModelNodeData> _nodes)
{
    Clear_();
    m_nodes = std::vector(_nodes.begin(), _nodes.end());
}

bool ModelExporter::Load(const Model& _model)
{
    Clear_();

    BufferReader reader;   // 메모리 풀
    for (const Model::Node& node: _model.GetNodes())
    {
        // 필요한 mesh 데이터 추출
        const Mesh&         gpuMesh     = node.mesh;
        const VertexBuffer& gpuVertices = gpuMesh.GetVertexBuffer();
        eVertexType         vertexT     = gpuMesh.GetVertexType();
        eTopology           topology    = gpuMesh.GetTopology();

        // 노드 데이터 생성
        MeshData cpuMeshData;

        // 메쉬 기하 추출
        {
            // 버텍스 버퍼
            {
                auto [cpuVertics, bReseult] = reader.ReadBuffer(gpuVertices);
                if (bReseult == false)   // 로드 실패
                {
                    JAM_ERROR("ModelExporter::Load() - Failed to read vertex buffer data for node: {}", node.name);
                    return false;
                }

                const UInt32 stride = gpuVertices.GetStride();
                JAM_ASSERT(cpuVertics.size() % stride == 0, "Vertex buffer size is not a multiple of vertex stride.");   // stride가 맞지 않으면 문제가 생길 수 있음
                UInt32 vertexCount = static_cast<UInt32>(cpuVertics.size() / stride);                                    // 버텍스 개수
                cpuMeshData.vertices.reserve(vertexCount);                                                               // 버텍스 개수만큼 미리 예약
                for (UInt32 i = 0; i < vertexCount; ++i)
                {
                    VertexAttribute vertex;
                    UnpackVertex(vertexT, cpuVertics.data() + i * stride, vertex);
                    cpuMeshData.vertices.emplace_back(vertex);
                }
            }

            // 인덱스 버퍼
            {
                const IndexBuffer& gpuIndices = node.mesh.GetIndexBuffer();
                auto [cpuIndices, bResult]    = reader.ReadBuffer(gpuIndices);

                if (bResult == false)   // 로드 실패
                {
                    JAM_ERROR("ModelExporter::Load() - Failed to read index buffer data for node: {}", node.name);
                    return false;
                }

                JAM_ASSERT(cpuIndices.size() % sizeof(Index) == 0, "Index buffer size is not a multiple of index size.");
                UInt32 indexCount = static_cast<UInt32>(cpuIndices.size() / sizeof(Index));
                cpuMeshData.indices.reserve(indexCount);
                for (UInt32 i = 0; i < indexCount; ++i)
                {
                    Index index;
                    std::memcpy(&index, cpuIndices.data() + i * sizeof(Index), sizeof(Index));
                    cpuMeshData.indices.emplace_back(index);
                }
            }
        }

        m_nodes.emplace_back(node.name, std::move(cpuMeshData), vertexT, topology, node.material);
    }

    return true;
}

bool ModelExporter::Export(const fs::path& _path) const
{
    using namespace flatbuffers;

    if (IsLoaded() == false)
    {
        JAM_ERROR("ModelExporter::Export() - Model not loaded yet.");
        return false;
    }

    FlatBufferBuilder                       builder;          // 플랫 버퍼의 빌더
    std::vector<Offset<fbs::ModelNodeData>> modelNodesData;   // 모델 노드 데이터
    modelNodesData.reserve(m_nodes.size());                   // 예약
    for (const ModelNodeData& node: m_nodes)
    {
        // 이름
        const Offset<String> nameOffset = builder.CreateString(node.name);

        // 메시 데이터
        const MeshData&                           meshData = node.meshData;
        std::vector<Offset<fbs::VertexAttribute>> fbsVertices;   // 버텍스 속성 fbs 벡터
        fbsVertices.reserve(meshData.vertices.size());
        for (const VertexAttribute& vertex: meshData.vertices)
        {
            fbs::Vec3 position  = ToFlatBuffersVec3(vertex.position);
            fbs::Vec3 color     = ToFlatBuffersVec3(vertex.color);
            fbs::Vec3 normal    = ToFlatBuffersVec3(vertex.normal);
            fbs::Vec2 uv0       = ToFlatBuffersVec2(vertex.uv0);
            fbs::Vec2 uv1       = ToFlatBuffersVec2(vertex.uv1);
            fbs::Vec3 tangent   = ToFlatBuffersVec3(vertex.tangent);
            fbs::Vec3 bitangent = ToFlatBuffersVec3(vertex.bitangent);
            fbsVertices.emplace_back(fbs::CreateVertexAttribute(builder, &position, &color, &normal, &uv0, &uv1, &tangent, &bitangent));
        }

        // 버텍스, 인덱스, 메쉬 데이터
        const Offset<Vector<Offset<fbs::VertexAttribute>>> verticesOffset = builder.CreateVector(fbsVertices);
        const Offset<Vector<UInt32>>                       indicesOffset  = builder.CreateVector(meshData.indices);
        const Offset<fbs::MeshData>                        meshDataOffset = fbs::CreateMeshData(builder, verticesOffset, indicesOffset);

        // 머테리얼
        const Material& material      = node.material;
        fbs::Vec3       ambientColor  = ToFlatBuffersVec3(material.ambientColor);
        fbs::Vec3       diffuseColor  = ToFlatBuffersVec3(material.diffuseColor);
        fbs::Vec3       specularColor = ToFlatBuffersVec3(material.specularColor);
        fbs::Vec3       albedoColor   = ToFlatBuffersVec3(material.albedoColor);
        fbs::Vec3       emissiveColor = ToFlatBuffersVec3(material.emissiveColor);

        // 텍스처
        Offset<String> albedoTexturePath    = ToFlatBuffersString(builder, material.albedoTexture);
        Offset<String> normalTexturePath    = ToFlatBuffersString(builder, material.normalTexture);
        Offset<String> metallicTexturePath  = ToFlatBuffersString(builder, material.metallicTexture);
        Offset<String> roughnessTexturePath = ToFlatBuffersString(builder, material.roughnessTexture);
        Offset<String> aoTexturePath        = ToFlatBuffersString(builder, material.aoTexture);
        Offset<String> emissiveTexturePath  = ToFlatBuffersString(builder, material.emissiveTexture);
        Offset<String> lightmapTexturePath  = ToFlatBuffersString(builder, material.lightmapTexture);

        // 머테리얼
        const Offset<fbs::Material> materialOffset = fbs::CreateMaterial(
            builder,
            &ambientColor,
            &diffuseColor,
            &specularColor,
            material.shininess,
            &albedoColor,
            material.metallic,
            material.roughness,
            material.ao,
            material.emissive,
            &emissiveColor,
            material.emissiveScale,
            albedoTexturePath,
            normalTexturePath,
            metallicTexturePath,
            roughnessTexturePath,
            aoTexturePath,
            emissiveTexturePath,
            lightmapTexturePath);

        // 모델 노드 데이터 생성
        modelNodesData.emplace_back(fbs::CreateModelNodeData(builder, nameOffset, meshDataOffset, ToFlatBuffersVertexType(node.vertexType), ToFlatBuffersTopology(node.topology), materialOffset));
    }

    // 최종 모델 데이터 생성
    const Offset<Vector<Offset<fbs::ModelNodeData>>> nodesVector = builder.CreateVector(modelNodesData);
    const Offset<fbs::ModelData>                     modelData   = fbs::CreateModelData(builder, nodesVector);
    builder.Finish(modelData);

    // 파일 저장
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