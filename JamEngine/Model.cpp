#include "pch.h"

#include "Model.h"

#include "FileIOUtilities.h"
#include "Profile.h"
#include "Resources/flatc/jmtl_generated.h"

namespace jam
{

[[nodiscard]] static jmtl::Matrix4x4 JmtlMatrixFrom(const Mat4& _mtx)
{
    Array<float, 16> vals {};
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            vals[r * 4 + c] = _mtx.m[r][c];
        }
    }

    // 4행 고정
    vals[0 + 3]  = 0.0f;
    vals[4 + 3]  = 0.0f;
    vals[8 + 3]  = 0.0f;
    vals[12 + 3] = 1.0f;

    return jmtl::Matrix4x4(::flatbuffers::span<const float, 16>(vals.data(), vals.size()));
}

[[nodiscard]] static Mat4 Mat4From(const jmtl::Matrix4x4& _fbsMtx)
{
    Mat4                                 mtx  = Mat4::Identity;
    const flatbuffers::Array<float, 16>* vals = _fbsMtx.values();
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            const Int32 idx = r * 4 + c;
            mtx.m[r][c]     = vals->Get(idx);
        }
    }

    // delete invalid values
    mtx._14 = 0.0f;
    mtx._24 = 0.0f;
    mtx._34 = 0.0f;
    mtx._44 = 1.0f;
    return mtx;
}

static Optional<ModelNode> ModelNodeFrom_Recursive(
    const jmtl::ModelNode* _pJmtlNode)
{
    ModelNode node = {};

    // name
    if (const flatbuffers::String* pName = _pJmtlNode->name())
    {
        node.name = pName->str();
    }
    else
    {
        GlobalBuffer::SetLastError("Model node is missing name field");
        return k_nullopt;
    }

    // mesh indices
    if (const flatbuffers::Vector<Int32>* pMeshIndices = _pJmtlNode->mesh_indices())
    {
        node.meshIndices.reserve(pMeshIndices->size());
        for (Int32 idx: *pMeshIndices)
        {
            node.meshIndices.emplace_back(idx);
        }
    }

    // material indices
    if (const flatbuffers::Vector<Int32>* pMaterialIndices = _pJmtlNode->material_indice())
    {
        node.materialIndices.reserve(pMaterialIndices->size());
        for (Int32 idx: *pMaterialIndices)
        {
            node.materialIndices.emplace_back(idx);
        }
    }

    if (const jmtl::Matrix4x4* pFbsMtx = _pJmtlNode->transform())
    {
        node.mtx = Mat4From(*pFbsMtx);
    }
    else
    {
        GlobalBuffer::SetLastError("Model node is missing local_transform field");
        return k_nullopt;
    }

    // children
    if (const auto* pJmtlNode = _pJmtlNode->children())
    {
        node.children.reserve(pJmtlNode->size());
        for (const jmtl::ModelNode* pFbsChildNode: *pJmtlNode)
        {
            // recursive
            if (Optional<ModelNode> optNode = ModelNodeFrom_Recursive(pFbsChildNode))
            {
                node.children.emplace_back(std::move(optNode.value()));
            }
            else
            {
                return k_nullopt;
            }
        }
    }
    // else: no children, leave as empty vector

    return node;
}

static flatbuffers::Offset<jmtl::ModelNode> JmtlModelNodeOffsetFrom_Recursive(
    flatbuffers::FlatBufferBuilder& _builder,
    const ModelNode&                _node)
{
    const auto            jmtlName          = _builder.CreateString(_node.name);
    const auto            jmtlMeshIdxes     = _node.meshIndices.empty() ? 0 : _builder.CreateVector<Int32>(_node.meshIndices);
    const auto            jmtlMaterialIdxes = _node.materialIndices.empty() ? 0 : _builder.CreateVector<Int32>(_node.materialIndices);
    const jmtl::Matrix4x4 jmtlMatrix        = JmtlMatrixFrom(_node.mtx);

    // children
    std::vector<flatbuffers::Offset<jmtl::ModelNode>> children;
    children.reserve(_node.children.size());
    for (const ModelNode& child: _node.children)
    {
        children.emplace_back(JmtlModelNodeOffsetFrom_Recursive(_builder, child));
    }
    const auto jmtlChildren = children.empty() ? 0 : _builder.CreateVector(children);

    return jmtl::CreateModelNode(_builder, jmtlName, &jmtlMatrix, jmtlMeshIdxes, jmtlMaterialIdxes, jmtlChildren);
}

}   // namespace jam

//////////////////////////////////////////////////////

namespace jam
{

void Model::LoadMemory(
    const ModelDesc& _desc,
    const fs::path&  _virtualPath)
{
    JAM_FUNCTION_PROFILE();
    CleanUp_();

    m_root      = _desc.rootNode;
    m_meshPaths = _desc.meshPaths;
    m_matPaths  = _desc.matPaths;
    m_path      = _virtualPath;
    m_bValid    = true;
}

bool Model::Load(
    const fs::path& _path)
{
    JAM_FUNCTION_PROFILE();
    CleanUp_();

    // read
    Optional<Vector<Byte>> optData = LoadBinaryFile(_path);
    if (!optData)
    {
        return false;
    }

    // verify
    const Vector<Byte>&   data     = optData.value();
    flatbuffers::Verifier verifier = { data.data(), data.size() };
    if (!jmtl::VerifyModelBuffer(verifier))
    {
        GlobalBuffer::SetLastError(R"(model file is corrupted or invalid: "{}")", _path.native());
        return false;
    }

    // parse
    const jmtl::Model* pJmtlModel = jmtl::GetModel(data.data());
    if (!pJmtlModel)
    {
        GlobalBuffer::SetLastError(R"(Failed to parse model file: "{}")", _path.native());
        return false;
    }

    // parse mesh
    if (const auto* pJmtlMeshPaths = pJmtlModel->mesh_paths())
    {
        m_meshPaths.reserve(pJmtlMeshPaths->size());
        for (const flatbuffers::String* pStr: *pJmtlMeshPaths)
        {
            m_meshPaths.emplace_back(pStr->c_str());
        }
    }

    // parse material
    if (const auto* pJmtlMaterialPaths = pJmtlModel->material_paths())
    {
        m_matPaths.reserve(pJmtlMaterialPaths->size());
        for (const flatbuffers::String* pStr: *pJmtlMaterialPaths)
        {
            m_matPaths.emplace_back(pStr->c_str());
        }
    }

    // parse root node
    const jmtl::ModelNode* pJmtlNode = pJmtlModel->root_node();
    if (!pJmtlNode)
    {
        GlobalBuffer::SetLastError(R"(Model file is missing root_node: "{}")", _path.native());
        return false;
    }

    if (Optional<ModelNode> optNode = ModelNodeFrom_Recursive(pJmtlNode))
    {
        m_root = std::move(optNode.value());
    }
    else
    {
        GlobalBuffer::SetLastError(R"(Failed to parse model node tree for model file: "{}")", _path.native());
        return false;
    }

    // load done
    m_path   = _path;
    m_bValid = true;
    return true;
}

void Model::Unload()
{
    CleanUp_();
}

bool Model::Save(const fs::path& _path) const
{
    JAM_FUNCTION_PROFILE();

    JAM_ASSERT(IsValid(), "Model is not valid to save");
    flatbuffers::FlatBufferBuilder builder { 1024 };

    // create node tree
    const auto jmtlNode = JmtlModelNodeOffsetFrom_Recursive(builder, m_root);

    // create mesh path strings vector
    Vector<flatbuffers::Offset<flatbuffers::String>> meshPaths;
    meshPaths.reserve(m_meshPaths.size());
    for (const fs::path& path: m_meshPaths)
    {
        meshPaths.emplace_back(builder.CreateString(path.native()));
    }
    const auto jmtlMeshPaths = meshPaths.empty() ? 0 : builder.CreateVector(meshPaths.data(), meshPaths.size());

    // create material path strings vector
    Vector<flatbuffers::Offset<flatbuffers::String>> matPaths;
    matPaths.reserve(m_matPaths.size());
    for (const fs::path& path: m_matPaths)
    {
        matPaths.emplace_back(builder.CreateString(path.native()));
    }
    const auto jmtlMatPaths = matPaths.empty() ? 0 : builder.CreateVector(matPaths.data(), matPaths.size());

    // create model
    const auto jmtlModel = jmtl::CreateModel(builder, jmtlNode, jmtlMeshPaths, jmtlMatPaths);
    jmtl::FinishModelBuffer(builder, jmtlModel);
    return SaveBinaryFile(_path, { builder.GetBufferPointer(), NarrowCast<Int32>(builder.GetSize()) });
}

bool Model::IsValid() const
{
    return m_bValid;
}

eAssetType Model::GetAssetType() const
{
    return k_type;
}

StringView Model::GetTypeName() const
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

const Vector<fs::path>& Model::GetMeshs() const
{
    return m_meshPaths;
}

const Vector<fs::path>& Model::GetMaterials() const
{
    return m_matPaths;
}

void Model::CleanUp_()
{
    m_path.clear();
    m_meshPaths.clear();
    m_matPaths.clear();
    m_root   = {};
    m_bValid = false;
}

}   // namespace jam