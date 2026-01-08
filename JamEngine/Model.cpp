#include "pch.h"

#include "Model.h"

#include "AssetManager.h"
#include "FileIOUtilities.h"
#include "Material.h"
#include "Mesh.h"
#include "Profile.h"

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