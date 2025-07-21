#include "pch.h"

#include "ModelExporter.h"

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

}   // namespace

namespace jam
{

void ModelExporter::Load(std::span<const RawModelElement> _parts)
{
    Clear_();
    m_elements = std::vector(_parts.begin(), _parts.end());
    m_bExported    = true;
}

bool ModelExporter::Load(const Model& _model, eVertexType _type)
{
    Clear_();

    for (const ModelElement& part: _model)
    {
        RawModelElement rawModelPart;

        // 기본 데이터 복사
        rawModelPart.name     = part.name;
        rawModelPart.material = part.material;

        // 메쉬 기하 추출
        {
            MeshGeometry& rawMeshRef = rawModelPart.rawMesh;

            // 버텍스 버퍼
            const VertexBuffer&               vb       = part.mesh.GetVertexBuffer();
            std::optional<std::vector<UInt8>> vbRawOpt = vb.Download();

            if (vbRawOpt.has_value() == false)
            {
                return false;
            }

            const std::vector<UInt8>& vbRaw        = *vbRawOpt;
            const UInt32              vertexStride = vb.GetStride();
            JAM_ASSERT(vbRawOpt->size() % vertexStride == 0, "Vertex buffer size is not a multiple of vertex stride.");
            UInt32 vertexCount = static_cast<UInt32>(vbRaw.size() / vertexStride);
            rawMeshRef.vertices.reserve(vertexCount);
            for (UInt32 i = 0; i < vertexCount; ++i)
            {
                VertexAttribute vertex;
                UnpackVertex(_type, vbRaw.data() + i * vertexStride, vertex);
                rawMeshRef.vertices.emplace_back(vertex);
            }

            // 인덱스 버퍼
            const IndexBuffer&                ib       = part.mesh.GetIndexBuffer();
            std::optional<std::vector<UInt8>> ibRawOpt = ib.Download();

            if (ibRawOpt.has_value() == false)
            {
                return false;
            }

            const std::vector<UInt8>& ibRaw = *ibRawOpt;
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

        m_elements.emplace_back(rawModelPart);
    }

    m_bExported = true;
    return true;
}

bool ModelExporter::Export(const fs::path& _path) const
{
    using namespace flatbuffers;

    if (!m_bExported)
    {
        JAM_ERROR("ModelExporter::Export() - Model not loaded yet.");
        return false;
    }

    FlatBufferBuilder                         builder;
    std::vector<Offset<fbs::RawModelElement>> parts;
    parts.reserve(m_elements.size());
    for (const RawModelElement& part: m_elements)
    {
        // name
        const Offset<String> nameOffset = builder.CreateString(part.name);

        // mesh
        const MeshGeometry&                       rawMesh = part.rawMesh;
        std::vector<Offset<fbs::VertexAttribute>> vertices;
        vertices.reserve(part.rawMesh.vertices.size());
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

        parts.emplace_back(fbs::CreateRawModelElement(builder, nameOffset, meshOffset, materialOffset));
    }

    const Offset<Vector<Offset<fbs::RawModelElement>>> partsOffset    = builder.CreateVector(parts);
    const Offset<fbs::RawModel>                        rawModelOffset = fbs::CreateRawModel(builder, partsOffset);
    builder.Finish(rawModelOffset);

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
    m_elements.clear();
    m_bExported = false;
}

}   // namespace jam