#include "pch.h"

#include "ModelLoader.h"

#include "Asset.h"
#include "AssetManager.h"
#include "AssetUtilities.h"
#include "vendor/flatbuffers/compiled/model_generated.h"
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

bool ModelLoader::Load(AssetManager& _assetMgrRef, const fs::path& _path)
{
    // 유효성 검사
    if (!IsCompatibleFromPath(eAssetType::Model, _path))
    {
        JAM_ERROR("ModelLoader::Load() - Invalid file path: {}", _path.string());
        return false;
    }

    // 파일 읽기
    std::ifstream fs(_path, std::ios::binary);
    if (!fs.is_open())
    {
        JAM_ERROR("Failed to open model file: {}", _path.string());
        return false;
    }

    // 초기화
    Clear_();

    // 파일 크기 확인 및 읽기
    fs.seekg(0, std::ios::end);
    const std::streamsize size = fs.tellg();
    fs.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!fs.read(buffer.data(), size))
    {
        JAM_ERROR("Failed to read model file: {}", _path.string());
        return false;
    }

    // FlatBuffers 버퍼 검증
    const uint8_t*        pBuffer = reinterpret_cast<const uint8_t*>(buffer.data());
    flatbuffers::Verifier verifier(pBuffer, static_cast<size_t>(size));
    if (!fbs::VerifyModelDataBuffer(verifier))
    {
        JAM_ERROR("Model file verification failed: {}", _path.string());
        return false;
    }

    const fbs::ModelData* fbsModelData = fbs::GetModelData(buffer.data());
    if (!fbsModelData)
    {
        JAM_ERROR("Failed to parse model file: {}", _path.string());
        return false;
    }

    m_modelNodes.reserve(fbsModelData->nodes()->size());
    for (const fbs::ModelNodeData* fbsNodeData: *fbsModelData->nodes())
    {
        // jam model data
        ModelNodeData modelNodeData;

        // name
        {
            modelNodeData.name = fbsNodeData->name()->str();
        }

        // Mesh (vertex, index, topology, vertex type)
        {
            // vertex
            const fbs::MeshData* fbsMeshData = fbsNodeData->mesh_data();
            modelNodeData.meshData.vertices.reserve(fbsMeshData->vertices()->size());
            for (const fbs::VertexAttribute* vertex: *fbsMeshData->vertices())
            {
                VertexAttribute vertexData;
                vertexData.position  = ToJamVec3(*vertex->position());
                vertexData.color     = ToJamVec3(*vertex->color());
                vertexData.normal    = ToJamVec3(*vertex->normal());
                vertexData.uv0       = ToJamVec2(*vertex->uv0());
                vertexData.uv1       = ToJamVec2(*vertex->uv1());
                vertexData.tangent   = ToJamVec3(*vertex->tangent());
                vertexData.bitangent = ToJamVec3(*vertex->bitangent());
                modelNodeData.meshData.vertices.emplace_back(std::move(vertexData));
            }

            // index
            modelNodeData.meshData.indices.assign(fbsMeshData->indices()->begin(), fbsMeshData->indices()->end());

            // vertex type and topology
            modelNodeData.topology   = ToJamTopology(fbsNodeData->topology());
            modelNodeData.vertexType = ToJamVertexType(fbsNodeData->vertex_type());
        }

        // Material
        {
            const fbs::Material* material        = fbsNodeData->material();
            modelNodeData.material.ambientColor  = ToJamVec3(*material->ambient_color());
            modelNodeData.material.diffuseColor  = ToJamVec3(*material->diffuse_color());
            modelNodeData.material.specularColor = ToJamVec3(*material->specular_color());
            modelNodeData.material.shininess     = material->shininess();
            modelNodeData.material.albedoColor   = ToJamVec3(*material->albedo_color());
            modelNodeData.material.metallic      = material->metallic();
            modelNodeData.material.roughness     = material->roughness();
            modelNodeData.material.ao            = material->ao();
            modelNodeData.material.emissive      = material->emissive();
            modelNodeData.material.emissiveColor = ToJamVec3(*material->emissive_color());
            modelNodeData.material.emissiveScale = material->emissive_scale();

            // textures load
            if (material->albedo_texture())
            {
                auto [asset, _]                      = _assetMgrRef.GetOrLoad<TextureAsset>(material->albedo_texture()->str());
                modelNodeData.material.albedoTexture = asset;
            }
            if (material->normal_texture())
            {
                auto [asset, _]                      = _assetMgrRef.GetOrLoad<TextureAsset>(material->normal_texture()->str());
                modelNodeData.material.normalTexture = asset;
            }
            if (material->metallic_texture())
            {
                auto [asset, _]                        = _assetMgrRef.GetOrLoad<TextureAsset>(material->metallic_texture()->str());
                modelNodeData.material.metallicTexture = asset;
            }
            if (material->roughness_texture())
            {
                auto [asset, _]                         = _assetMgrRef.GetOrLoad<TextureAsset>(material->roughness_texture()->str());
                modelNodeData.material.roughnessTexture = asset;
            }
            if (material->ao_texture())
            {
                auto [asset, _]                  = _assetMgrRef.GetOrLoad<TextureAsset>(material->ao_texture()->str());
                modelNodeData.material.aoTexture = asset;
            }
            if (material->emissive_texture())
            {
                auto [asset, _]                        = _assetMgrRef.GetOrLoad<TextureAsset>(material->emissive_texture()->str());
                modelNodeData.material.emissiveTexture = asset;
            }
            if (material->lightmap_texture())
            {
                auto [asset, _]                        = _assetMgrRef.GetOrLoad<TextureAsset>(material->lightmap_texture()->str());
                modelNodeData.material.lightmapTexture = asset;
            }
        }
        m_modelNodes.emplace_back(std::move(modelNodeData));
    }
    return true;
}

void ModelLoader::Clear_()
{
    *this = ModelLoader();
}

}   // namespace jam