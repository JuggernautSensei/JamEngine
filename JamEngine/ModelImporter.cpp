#include "pch.h"

#include "ModelImporter.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/mesh.h>

namespace jam
{

bool ModelImporter::Import(const fs::path& _path)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(_path.string(), aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_RemoveRedundantMaterials | aiProcess_FindDegenerates | aiProcess_FindInvalidData | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph | aiProcess_MakeLeftHanded | aiProcess_ImproveCacheLocality);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        JAM_ERROR("Assimp error: {}", importer.GetErrorString());
        return false;
    }

    // 모델 지오메트리 초기화
    {
        Clear_();
        m_rawModelElems.reserve(scene->mNumMeshes);
    }

    ProcessNode_(scene->mRootNode, scene);
    m_bImported = true;
    return true;
}

const std::vector<RawModelElement>& ModelImporter::GetRawModelParts() const
{
    if (!m_bImported)
    {
        JAM_ERROR("ModelImporter::GetRawModelParts() - Model not imported yet.");
        throw std::runtime_error("Model not imported yet.");
    }
    return m_rawModelElems;
}

void ModelImporter::Clear_()
{
    m_rawModelElems.clear();
    m_bImported = false;
}

void ModelImporter::ProcessNode_(const aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessMesh_(mesh, scene);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode_(node->mChildren[i], scene);
    }
}

void ModelImporter::ProcessMesh_(const aiMesh* mesh, const aiScene* scene)
{
    RawModelElement node;

    // 이름
    node.name = mesh->mName.C_Str();

    // 메시
    {
        MeshGeometry& meshGeometry = node.rawMesh;

        std::vector<VertexAttribute>& vertices = meshGeometry.vertices;
        vertices.reserve(mesh->mNumVertices);

        // 정점
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            VertexAttribute vertex;

            // 정점 위치
            const aiVector3D pos = mesh->mVertices[i];
            vertex.position      = Vec3(pos.x, pos.y, pos.z);

            // 정점 노말
            if (mesh->HasNormals())
            {
                const aiVector3D normal = mesh->mNormals[i];
                vertex.normal           = Vec3(normal.x, normal.y, normal.z);
            }

            // 텍스처 좌표
            if (mesh->HasTextureCoords(0))
            {
                const aiVector3D texCoord = mesh->mTextureCoords[0][i];
                vertex.uv0                = Vec2(texCoord.x, texCoord.y);
            }

            // 텍스처 좌표 2
            if (mesh->HasTextureCoords(1))
            {
                const aiVector3D texCoord = mesh->mTextureCoords[1][i];
                vertex.uv1                = Vec2(texCoord.x, texCoord.y);
            }

            // 탠젠트
            if (mesh->HasTangentsAndBitangents())
            {
                const aiVector3D tangent   = mesh->mTangents[i];
                const aiVector3D bitangent = mesh->mBitangents[i];
                vertex.tangent             = Vec3(tangent.x, tangent.y, tangent.z);
                vertex.bitangent           = Vec3(bitangent.x, bitangent.y, bitangent.z);
            }

            // 색상
            if (mesh->HasVertexColors(0))
            {
                const aiColor4D color = mesh->mColors[0][i];
                vertex.color          = Vec3(color.r, color.g, color.b);
            }

            // 정점 데이터 추가
            vertices.emplace_back(vertex);
        }

        std::vector<UInt32>& indices = meshGeometry.indices;
        indices.reserve(mesh->mNumFaces * 3);

        // 인덱스
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            const aiFace& face = mesh->mFaces[i];
            JAM_ASSERT(face.mNumIndices == 3, "Assimp mesh face must have 3 indices for triangulation.");
            for (unsigned int j = 0; j < face.mNumIndices; j++)
            {
                indices.emplace_back(face.mIndices[j]);
            }
        }
    }

    // 머티리얼
    {
        Material&         material   = node.material;
        const aiMaterial* aiMaterial = scene->mMaterials[mesh->mMaterialIndex];
        aiColor4D         color;

        if (AI_SUCCESS == aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color))
        {
            material.diffuseColor = Vec3(color.r, color.g, color.b);
        }

        if (AI_SUCCESS == aiMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color))
        {
            material.specularColor = Vec3(color.r, color.g, color.b);
        }

        if (AI_SUCCESS == aiMaterial->Get(AI_MATKEY_COLOR_AMBIENT, color))
        {
            material.ambientColor = Vec3(color.r, color.g, color.b);
        }

        if (AI_SUCCESS == aiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, color))
        {
            material.emissiveColor = Vec3(color.r, color.g, color.b);
        }

        float emmisiveIntensity = 0.f;
        if (AI_SUCCESS == aiMaterial->Get(AI_MATKEY_EMISSIVE_INTENSITY, emmisiveIntensity))
        {
            material.emissiveScale = emmisiveIntensity;
        }

        float shiniess = 0.f;
        if (AI_SUCCESS == aiMaterial->Get(AI_MATKEY_SHININESS, shiniess))
        {
            material.shininess = shiniess;
        }
    }

    m_rawModelElems.emplace_back(node);
}

}   // namespace jam