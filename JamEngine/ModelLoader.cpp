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

}   // namespace

namespace jam
{

bool ModelLoader::Load(const fs::path& _path)
{
    std::wstring_view extension = _path.extension().native();
    if (extension != k_jamModelExtensionW)
    {
        JAM_ERROR("Invalid model file extension: {}. Expected: {}", ConvertToString(extension), ConvertToString(k_jamModelExtensionW));
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

    m_rawModelElems.reserve(rawModel->parts()->size());
    for (const fbs::RawModelElement* rawPart: *rawModel->parts())
    {
        RawModelElement part;
        part.name = rawPart->name()->str();

        // Mesh
        {
            const fbs::MeshGeometry* rawMesh = rawPart->mesh();
            part.rawMesh.vertices.reserve(rawMesh->vertices()->size());
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
                part.rawMesh.vertices.emplace_back(std::move(vertexData));
            }

            part.rawMesh.indices.assign(rawMesh->indices()->begin(), rawMesh->indices()->end());
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
        m_rawModelElems.emplace_back(std::move(part));
    }

    m_bLoaded = true;
    return true;
}

const std::vector<RawModelElement>& ModelLoader::GetRawModelParts() const
{
    JAM_ASSERT(m_bLoaded, "ModelLoader::GetRawModelParts() - Model not loaded yet.");
    return m_rawModelElems;
}

void ModelLoader::Clear_()
{
    m_rawModelElems.clear();
    m_bLoaded = false;
}

}   // namespace jam