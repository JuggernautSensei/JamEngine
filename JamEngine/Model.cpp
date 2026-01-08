#include "pch.h"

#include "Model.h"

#include "AssetManager.h"
#include "FileIOUtilities.h"
#include "Material.h"
#include "Mesh.h"
#include "Profile.h"

#include "Resources/flatc/jmtl_generated.h"
namespace fbs = flatbuffers;

namespace jam
{

[[nodiscard]] static jmtl::Matrix4x4 JmtlMatrixFrom(
    const Mat4& _mtx)
{
    Array<float, 16> elems = {};
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            elems[r * 4 + c] = _mtx.m[r][c];
        }
    }

    // 4행 고정
    elems[0 + 3]  = 0.0f;
    elems[4 + 3]  = 0.0f;
    elems[8 + 3]  = 0.0f;
    elems[12 + 3] = 1.0f;

    return {
        fbs::span<float, 16> { elems.data(), elems.size() }
    };
}

[[nodiscard]] static Mat4 Mat4From(
    const jmtl::Matrix4x4& _fbsMtx)
{
    Mat4                         mtx   = {};
    const fbs::Array<float, 16>* elems = _fbsMtx.values();

    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            const Int32 idx = r * 4 + c;
            mtx.m[r][c]     = elems->Get(idx);
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
    {
        const fbs::String* pName = _pJmtlNode->name();
        if (!pName)
        {
            GlobalBuffer::SetLastError("Model node is missing name field");
            return k_nullopt;
        }
        node.name = pName->str();
    }

    // mesh indices
    {
        const fbs::Vector<Int32>* meshIdxes = _pJmtlNode->mesh_indices();
        if (!meshIdxes)
        {
            GlobalBuffer ::SetLastError("Model node is missing mesh_indices field");
            return k_nullopt;
        }

        node.meshIdxes.reserve(meshIdxes->size());
        for (Int32 idx: *meshIdxes)
        {
            node.meshIdxes.emplace_back(idx);
        }
    }

    // material indices
    {
        const fbs::Vector<Int32>* matIdxes = _pJmtlNode->material_indice();
        if (!matIdxes)
        {
            GlobalBuffer ::SetLastError("Model node is missing material_indice field");
            return k_nullopt;
        }

        node.matIdxes.reserve(matIdxes->size());
        for (Int32 idx: *matIdxes)
        {
            node.matIdxes.emplace_back(idx);
        }
    }

    // local transform
    {
        const jmtl::Matrix4x4* pFbsMtx = _pJmtlNode->transform();
        if (!pFbsMtx)
        {
            GlobalBuffer::SetLastError("Model node is missing transform field");
            return k_nullopt;
        }

        node.mtx = Mat4From(*pFbsMtx);
    }

    // children
    {
        const fbs::Vector<fbs::Offset<jmtl::ModelNode>>* pJmtlChildren = _pJmtlNode->children();
        if (pJmtlChildren)
        {
            node.children.reserve(pJmtlChildren->size());
            for (const jmtl::ModelNode* pJmtlChildNode: *pJmtlChildren)
            {
                // recursive
                ModelNode childNode = {};
                if (!MoveIfHasValue(ModelNodeFrom_Recursive(pJmtlChildNode), childNode))
                {
                    return k_nullopt;
                }

                // add
                node.children.emplace_back(std::move(childNode));
            }
        }
        // else: no children, leave as empty vector
    }

    return node;
}

static fbs::Offset<jmtl::ModelNode> JmtlModelNodeOffsetFrom_Recursive(
    fbs::FlatBufferBuilder& _builder,
    const ModelNode&        _node)
{
    // make children offsets recursively
    Vector<fbs::Offset<jmtl::ModelNode>> children = {};
    children.reserve(_node.children.size());
    for (const ModelNode& child: _node.children)
    {
        children.emplace_back(JmtlModelNodeOffsetFrom_Recursive(_builder, child));
    }

    // create node offset
    const jmtl::Matrix4x4 jmtlMtx = JmtlMatrixFrom(_node.mtx);
    return jmtl::CreateModelNode(
        _builder,
        _builder.CreateString(_node.name),
        &jmtlMtx,
        _builder.CreateVector<Int32>(_node.meshIdxes),
        _builder.CreateVector<Int32>(_node.matIdxes),
        _builder.CreateVector(children.data(), children.size()));
}

}   // namespace jam

///////////////////////////////////////////////////////////////////////////

namespace jam
{

bool Model::Load(
    const fs::path& _path)
{
    JAM_FUNCTION_PROFILE();
    CleanUp_();

    // read
    Vector<Byte> data = {};
    if (!MoveIfHasValue(LoadBinaryFile(_path), data))
    {
        GlobalBuffer::SetLastError("Failed to load model file: {}. {}", _path.native(), GlobalBuffer::GetLastError());
        return false;
    }

    // verify
    fbs::Verifier verifier { data.data(), static_cast<size_t>(data.size()) };
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

    // name
    {
        const fbs::String* pJmtlName = pJmtlModel->name();
        if (!pJmtlName)
        {
            GlobalBuffer::SetLastError(R"(Model file is missing name: "{}")", _path.native());
            return false;
        }

        m_name = pJmtlName->str();
    }

    // mesh
    const fbs::Vector<fbs::Offset<fbs::String>>* pJmtlMeshPaths = pJmtlModel->mesh_paths();
    if (pJmtlMeshPaths)
    {
        m_meshes.reserve(pJmtlMeshPaths->size());
        for (const fbs::String* pStr: *pJmtlMeshPaths)
        {
            Ref<Mesh> pMesh = nullptr;
            if (!MoveIfHasValue(AssetManager::LoadMesh(pStr->c_str()), pMesh))
            {
                GlobalBuffer::SetLastError(R"(Failed to load mesh "{}" for model file: "{}")", pStr->c_str(), _path.native());
                return false;
            }

            m_meshes.emplace_back(pMesh);
        }
    }

    // material
    const fbs::Vector<fbs::Offset<fbs::String>>* pJmtlMaterialPaths = pJmtlModel->material_paths();
    if (pJmtlMaterialPaths)
    {
        m_mats.reserve(pJmtlMaterialPaths->size());
        for (const fbs::String* pStr: *pJmtlMaterialPaths)
        {
            Ref<Material> pMat = nullptr;
            if (!MoveIfHasValue(AssetManager::LoadMaterial(pStr->c_str()), pMat))
            {
                GlobalBuffer::SetLastError(R"(Failed to load material "{}" for model file: "{}")", pStr->c_str(), _path.native());
                return false;
            }

            m_mats.emplace_back(pMat);
        }
    }

    // root node
    const jmtl::ModelNode* pJmtlNode = pJmtlModel->root_node();
    if (!pJmtlNode)
    {
        GlobalBuffer::SetLastError(R"(Model file is missing root_node: "{}")", _path.native());
        return false;
    }

    // parse node tree recursively
    if (!MoveIfHasValue(ModelNodeFrom_Recursive(pJmtlNode), m_root))
    {
        GlobalBuffer::SetLastError(R"(Failed to parse model node tree for model file: "{}")", _path.native());
        return false;
    }

    // done
    m_path            = _path;
    m_bValid          = true;
    m_bLoadedFromFile = true;
    return true;
}

void Model::LoadMemory(
    const ModelNode&             _node,
    const Vector<Ref<Mesh>>&     _meshes,
    const Vector<Ref<Material>>& _mats)
{
    JAM_FUNCTION_PROFILE();
    CleanUp_();

    m_root            = _node;
    m_meshes          = _meshes;
    m_mats            = _mats;
    m_bValid          = true;
    m_bLoadedFromFile = false;
}

void Model::Unload()
{
    CleanUp_();
}

bool Model::Save(
    const fs::path&      _path,
    const bool           _bReplaceExtension,
    const eSaveFileFlags _flags) const
{
    JAM_FUNCTION_PROFILE();

    JAM_ASSERT(IsValid(), "Model is not valid to save");
    fbs::FlatBufferBuilder builder { 1024 };

    // create mesh asset pathes
    Vector<fbs::Offset<fbs::String>> meshPathes = {};
    meshPathes.reserve(m_meshes.size());
    for (const Ref<Mesh>& pMesh: m_meshes)
    {
        const fs::path& path = pMesh->GetPath();
        meshPathes.emplace_back(builder.CreateString(path.native()));
    }

    // create material path strings vector
    Vector<fbs::Offset<fbs::String>> matPathes;
    matPathes.reserve(m_mats.size());
    for (const Ref<Material>& pMat: m_mats)
    {
        const fs::path& path = pMat->GetPath();
        matPathes.emplace_back(builder.CreateString(path.native()));
    }

    // create model
    const fbs::Offset<jmtl::Model> jmtlModel = jmtl::CreateModel(
        builder,
        builder.CreateString(m_name),
        JmtlModelNodeOffsetFrom_Recursive(builder, m_root),
        builder.CreateVector(meshPathes.data(), meshPathes.size()),
        builder.CreateVector(matPathes.data(), matPathes.size()));

    jmtl::FinishModelBuffer(builder, jmtlModel);

    // make path
    if (_bReplaceExtension)
    {
        fs::path path = _path;
        path.replace_extension(k_jamModelExt.GetPtr());
        return SaveBinaryFile(path, { builder.GetBufferPointer(), NarrowCast<Int32>(builder.GetSize()) }, _flags);
    }

    // io
    return SaveBinaryFile(_path, { builder.GetBufferPointer(), NarrowCast<Int32>(builder.GetSize()) }, _flags);
}

void Model::SetName(
    const StringView _name)
{
    m_name = _name.ToString();
}

void Model::SetVirtualPath(
    const fs::path& _path)
{
    JAM_ASSERT(!m_bLoadedFromFile, "Cannot change path of model loaded from file");
    m_path = _path;
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

const StringView Model::GetName() const
{
    return m_name;
}

const ModelNode& Model::GetRootNode() const
{
    return m_root;
}

const Vector<Ref<Mesh>>& Model::GetMeshs() const
{
    return m_meshes;
}

const Vector<Ref<Material>>& Model::GetMaterials() const
{
    return m_mats;
}

void Model::CleanUp_()
{
    m_path.clear();
    m_meshes.clear();
    m_mats.clear();
    m_name            = {};
    m_root            = {};
    m_bValid          = false;
    m_bLoadedFromFile = false;
}

}   // namespace jam