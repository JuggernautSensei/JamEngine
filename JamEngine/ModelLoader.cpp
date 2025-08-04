#include "pch.h"

#include "ModelLoader.h"

#include "Config.h"
#include "StringUtilities.h"
#include "flatbuffers/compiled/model_generated.h"
#include <fstream>

namespace
{

NODISCARD jam::Vec3 ToJamVec3(const jam::fbs::Vec3& vec)
{
    return { vec.x(), vec.y(), vec.z() };
}

NODISCARD jam::Vec2 ToJamVec2(const jam::fbs::Vec2& vec)
{
    return { vec.x(), vec.y() };
}

NODISCARD jam::eVertexType ToJamVertexType(const jam::fbs::eVertexType vertexType)
{
    switch (vertexType)
    {
        case jam::fbs::eVertexType_Vertex2: return jam::eVertexType::Vertex2;
        case jam::fbs::eVertexType_Vertex3: return jam::eVertexType::Vertex3;
        case jam::fbs::eVertexType_Vertex3PosOnly: return jam::eVertexType::Vertex3PosOnly;
        default:
            JAM_ERROR("Unknown vertex type");
            return jam::eVertexType::Vertex3;   // Default fallback
    }
}

NODISCARD jam::eTopology ToJamTopology(const jam::fbs::eTopology topology)
{
    switch (topology)
    {
        case jam::fbs::eTopology_Undefined: return jam::eTopology::Undefined;
        case jam::fbs::eTopology_PointList: return jam::eTopology::PointList;
        case jam::fbs::eTopology_LineList: return jam::eTopology::LineList;
        case jam::fbs::eTopology_LineStrip: return jam::eTopology::LineStrip;
        case jam::fbs::eTopology_TriangleList: return jam::eTopology::TriangleList;
        case jam::fbs::eTopology_TriangleStrip: return jam::eTopology::TriangleStrip;
        default:
            JAM_ERROR("Unknown topology type");
            return jam::eTopology::TriangleList;   // Default fallback
    }
}

}   // namespace

namespace jam
{

bool ModelLoader::Load(const fs::path& _path)
{
    std::wstring ext = ToLower(_path.extension().native());
    if (ext != k_jamModelExtensionW)
    {
        JAM_ERROR("Invalid model file extension: {}. Expected: {}", ConvertToString(ext), ConvertToString(k_jamModelExtensionW));
        return false;
    }

    std::ifstream fs(_path, std::ios::binary);
    if (!fs.is_open())
    {
        JAM_ERROR("Failed to open model file: {}", _path.string());
        return false;
    }

    Clear_();

    fs.seekg(0, std::ios::end);
    const std::streamsize size = fs.tellg();

    fs.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!fs.read(buffer.data(), size))
    {
        JAM_ERROR("Failed to read model file: {}", _path.string());
        return false;
    }

    flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(buffer.data()), static_cast<size_t>(size));
    if (!fbs::VerifyRawModelBuffer(verifier))
    {
        JAM_ERROR("Model file verification failed: {}", _path.string());
        return false;
    }

    const fbs::RawModel* rawModel = fbs::GetRawModel(buffer.data());
    if (!rawModel)
    {
        JAM_ERROR("Failed to parse model file: {}", _path.string());
        return false;
    }

    m_loadData.nodes.reserve(rawModel->nodes()->size());
    for (const fbs::RawModelNode* rawPart: *rawModel->nodes())
    {
        RawModelNode part;
        part.name = rawPart->name()->str();

        // Mesh
        {
            const fbs::MeshGeometry* rawMesh = rawPart->mesh();
            part.meshGeometry.vertices.reserve(rawMesh->vertices()->size());
            for (const fbs::VertexAttribute* vertex: *rawMesh->vertices())
            {
                VertexAttribute vertexData;
                vertexData.position  = ToJamVec3(*vertex->position());
                vertexData.color     = ToJamVec3(*vertex->color());
                vertexData.normal    = ToJamVec3(*vertex->normal());
                vertexData.uv0       = ToJamVec2(*vertex->uv0());
                vertexData.uv1       = ToJamVec2(*vertex->uv1());
                vertexData.tangent   = ToJamVec3(*vertex->tangent());
                vertexData.bitangent = ToJamVec3(*vertex->bitangent());
                part.meshGeometry.vertices.emplace_back(std::move(vertexData));
            }

            part.meshGeometry.indices.assign(rawMesh->indices()->begin(), rawMesh->indices()->end());
        }

        // Material
        {
            const fbs::Material* material   = rawPart->material();
            part.material.ambientColor      = ToJamVec3(*material->ambient_color());
            part.material.diffuseColor      = ToJamVec3(*material->diffuse_color());
            part.material.specularColor     = ToJamVec3(*material->specular_color());
            part.material.shininess         = material->shininess();
            part.material.albedoColor       = ToJamVec3(*material->albedo_color());
            part.material.metallic          = material->metallic();
            part.material.roughness         = material->roughness();
            part.material.ao                = material->ao();
            part.material.emissive          = material->emissive();
            part.material.emissiveColor     = ToJamVec3(*material->emissive_color());
            part.material.emissiveScale     = material->emissive_scale();
            part.material.displacementScale = material->displacement_scale();
        }
        m_loadData.nodes.emplace_back(std::move(part));
    }

    m_loadData.topology   = ToJamTopology(rawModel->topology());
    m_loadData.vertexType = ToJamVertexType(rawModel->vertex_type());
    m_bLoaded             = true;
    return true;
}

const ModelLoadData& ModelLoader::GetLoadData() const
{
    JAM_ASSERT(m_bLoaded, "ModelLoader::GetLoadData() - Model not loaded yet.");
    return m_loadData;
}

void ModelLoader::Clear_()
{
    *this = ModelLoader();
}

}   // namespace jam