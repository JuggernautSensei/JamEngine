#include "pch.h"

#include "Model.h"

#include "AssetManager.h"
#include "FileUtilities.h"
#include "MemView.h"
#include "Profile.h"
#include "Resources/flatc/jmtl_generated.h"

namespace jam
{

static std::optional<ModelNode> CreateModelNode_Recursive(const jmtl::ModelNode* _pFbsNode)
{
    ModelNode node = {};

    if (const flatbuffers::String* pName = _pFbsNode->name())
    {
        node.name = pName->str();
    }
    else
    {
        SetErrorString("Model node is missing name field");
        return std::nullopt;
    }

    if (const auto* pMeshIndices = _pFbsNode->mesh_indices())
    {
        node.meshIndices.reserve(pMeshIndices->size());
        for (int32_t idx: *pMeshIndices)
        {
            node.meshIndices.emplace_back(idx);
        }
    }

    if (const auto* pMaterialIndices = _pFbsNode->material_indice())
    {
        node.materialIndices.reserve(pMaterialIndices->size());
        for (int32_t idx: *pMaterialIndices)
        {
            node.materialIndices.emplace_back(idx);
        }
    }

    if (const jmtl::Matrix4x4* pFbsLocalTransform = _pFbsNode->transform())
    {
        const flatbuffers::Array<float, 16>* floats = pFbsLocalTransform->values();
        for (Int32 r = 0; r < 4; ++r)
        {
            for (Int32 c = 0; c < 4; ++c)
            {
                Int32 index            = r * 4 + c;
                node.transform.m[r][c] = floats->Get(index);
            }
        }

        // delete invalid values
        node.transform._14 = 0.0f;
        node.transform._24 = 0.0f;
        node.transform._34 = 0.0f;
        node.transform._44 = 1.0f;
    }
    else
    {
        SetErrorString("Model node is missing local_transform field");
        return std::nullopt;
    }

    if (const flatbuffers::Vector<flatbuffers::Offset<jmtl::ModelNode>>* pFbsChildren = _pFbsNode->children())
    {
        node.children.reserve(pFbsChildren->size());
        for (const jmtl::ModelNode* pFbsChildNode: *pFbsChildren)
        {
            if (std::optional<ModelNode> optChildNode = CreateModelNode_Recursive(pFbsChildNode))
            {
                node.children.push_back(std::move(*optChildNode));
            }
            else
            {
                return std::nullopt;
            }
        }
    }
    // if no children, leave as empty vector

    return node;
}

[[nodiscard]] static jmtl::Matrix4x4 ToFbsMatrix(const Mat4& _mtx)
{
    std::array<float, 16> vals {};
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            vals[r * 4 + c] = _mtx.m[r][c];
        }
    }

    // delete invalid values
    vals[0 + 3]  = 0.0f;
    vals[4 + 3]  = 0.0f;
    vals[8 + 3]  = 0.0f;
    vals[12 + 3] = 1.0f;

    // construct
    return jmtl::Matrix4x4(::flatbuffers::span<const float, 16>(vals.data(), vals.size()));
}

static flatbuffers::Offset<jmtl::ModelNode> CreateFbsModelNode_Recursive(flatbuffers::FlatBufferBuilder& _builder, const ModelNode& _node)
{
    // name
    auto fbsName = _builder.CreateString(_node.name);

    // mesh indices
    auto fbsMeshIndices = _node.meshIndices.empty() ? 0 : _builder.CreateVector<int32_t>(_node.meshIndices);

    // material indices
    auto fbsMaterialIndices = _node.materialIndices.empty() ? 0 : _builder.CreateVector<int32_t>(_node.materialIndices);

    // transform struct
    jmtl::Matrix4x4 fbsTransform = ToFbsMatrix(_node.transform);

    // children
    std::vector<flatbuffers::Offset<jmtl::ModelNode>> childOffsets;
    childOffsets.reserve(_node.children.size());
    for (const ModelNode& child: _node.children)
    {
        childOffsets.emplace_back(CreateFbsModelNode_Recursive(_builder, child));
    }
    auto fbsChildren = childOffsets.empty() ? 0 : _builder.CreateVector(childOffsets);

    return jmtl::CreateModelNode(_builder, fbsName, &fbsTransform, fbsMeshIndices, fbsMaterialIndices, fbsChildren);
}

}   // namespace jam

namespace jam
{

void Model::LoadFromMemory(const ModelDesc& _desc, const fs::path& _virtualPath)
{
    JAM_FUNCTION_PROFILE();
    Unload();

    m_root      = _desc.rootNode;
    m_meshPaths = _desc.meshPaths;
    m_matPaths  = _desc.matPaths;
    m_path      = _virtualPath;
    m_bValid    = true;
}

bool Model::Load(const fs::path& _path)
{
    JAM_FUNCTION_PROFILE();
    Unload();

    // read
    std::optional<std::vector<Byte>> optBuf = LoadBinaryFromFile(_path);
    if (!optBuf)
    {
        return false;
    }
    const std::vector<Byte>& buf = optBuf.value();

    // verify
    flatbuffers::Verifier verifier { buf.data(), buf.size() };
    if (!jmtl::VerifyModelBuffer(verifier))
    {
        SetErrorString(R"(Model file is corrupted or invalid: "{}")", _path.native());
        return false;
    }

    // parse
    const jmtl::Model* pFbsModel = jmtl::GetModel(buf.data());
    if (!pFbsModel)
    {
        SetErrorString(R"(Failed to parse model file: "{}")", _path.native());
        return false;
    }

    // parse mesh
    m_meshPaths.clear();
    if (const auto* pFbsMeshPaths = pFbsModel->mesh_paths())
    {
        m_meshPaths.reserve(pFbsMeshPaths->size());
        for (const flatbuffers::String* pStr: *pFbsMeshPaths)
        {
            m_meshPaths.emplace_back(pStr->c_str());
        }
    }

    // parse material
    m_matPaths.clear();
    if (const auto* pFbsMaterialPaths = pFbsModel->material_paths())
    {
        m_matPaths.reserve(pFbsMaterialPaths->size());
        for (const flatbuffers::String* pStr: *pFbsMaterialPaths)
        {
            m_matPaths.emplace_back(pStr->c_str());
        }
    }

    // parse root node
    const jmtl::ModelNode* pFbsRootNode = pFbsModel->root_node();
    if (!pFbsRootNode)
    {
        SetErrorString(R"(Model file is missing root_node: "{}")", _path.native());
        return false;
    }

    m_path = _path;
    if (std::optional<ModelNode> optRoot = CreateModelNode_Recursive(pFbsRootNode))
    {
        ModelNode& root = optRoot.value();
        m_root          = std::move(root);
    }
    else
    {
        SetErrorString(R"(Failed to parse model node tree for model file: "{}")", _path.native());
        return false;
    }

    // load done
    m_bValid = true;
    return true;
}

void Model::Unload()
{
    m_path.clear();
    m_root = {};
    m_meshPaths.clear();
    m_matPaths.clear();
    m_bValid = false;
}

bool Model::Save(const fs::path& _path) const
{
    JAM_FUNCTION_PROFILE();

    JAM_ASSERT(IsValid(), "Model is not valid to save");
    flatbuffers::FlatBufferBuilder builder { 1024 };

    // create node tree
    auto rootOffset = CreateFbsModelNode_Recursive(builder, m_root);

    // create mesh path strings vector
    std::vector<flatbuffers::Offset<flatbuffers::String>> fbsMeshPaths;
    fbsMeshPaths.reserve(m_meshPaths.size());
    for (const fs::path& path: m_meshPaths)
    {
        fbsMeshPaths.emplace_back(builder.CreateString(path.native()));
    }
    auto fbsMeshPathsVec = fbsMeshPaths.empty() ? 0 : builder.CreateVector(fbsMeshPaths);

    // create material path strings vector
    std::vector<flatbuffers::Offset<flatbuffers::String>> fbsMaterialPaths;
    fbsMaterialPaths.reserve(m_matPaths.size());
    for (const fs::path& path: m_matPaths)
    {
        fbsMaterialPaths.emplace_back(builder.CreateString(path.native()));
    }
    auto fbsMaterialPathsVec = fbsMaterialPaths.empty() ? 0 : builder.CreateVector(fbsMaterialPaths);

    const auto modelOffset = jmtl::CreateModel(builder, rootOffset, fbsMeshPathsVec, fbsMaterialPathsVec);
    jmtl::FinishModelBuffer(builder, modelOffset);
    return SaveBinaryToFile(_path, { builder.GetBufferPointer(), builder.GetSize() });
}

bool Model::IsValid() const
{
    return m_bValid;
}

eAssetType Model::GetAssetType() const
{
    return k_type;
}

std::string_view Model::GetTypeName() const
{
    return k_name;
}

UInt32 Model::GetTypeHash() const
{
    return k_hash;
}

const fs::path& Model::GetPath() const
{
    return m_path;
}

const ModelNode& Model::GetRootNode() const
{
    return m_root;
}

const std::vector<fs::path>& Model::GetMeshs() const
{
    return m_meshPaths;
}

const std::vector<fs::path>& Model::GetMaterials() const
{
    return m_matPaths;
}

}   // namespace jam