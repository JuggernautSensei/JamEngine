#include "pch.h"

#include "Model.h"

#include "AssetManager.h"
#include "AssetSaveLoader.h"
#include "Material.h"
#include "Mesh.h"
#include "Profile.h"

namespace Jam
{

namespace Dto
{

    struct ModelNode
    {
        StringView        name      = {};
        Matrix            localMtx  = {};
        Vector<Int32>     meshIdxes = {};
        Vector<Int32>     matIdxes  = {};
        Vector<ModelNode> children  = {};
    };

    struct Model
    {
        StringView         name      = {};
        ModelNode          root      = {};
        Vector<StringView> meshPaths = {};
        Vector<StringView> matPaths  = {};
    };
}   // namespace Dto

namespace
{

    [[nodiscard]] Dto::ModelNode WrapModelNode_Recursive(
        const ModelNode& _node)
    {
        Dto::ModelNode node = {};
        node.name           = Dto::Wrap(_node.name);
        node.localMtx       = Dto::Wrap(_node.localMtx);
        node.meshIdxes      = Dto::Wrap<Int32>(_node.meshIdxes);
        node.matIdxes       = Dto::Wrap<Int32>(_node.matIdxes);

        // children
        node.children.reserve(static_cast<UInt32>(_node.children.size()));
        for (const ModelNode& child: _node.children)
        {
            node.children.emplace_back(WrapModelNode_Recursive(child));
        }
        return node;
    }

    [[nodiscard]] ModelNode UnwrapModelNode_Recursive(
        const Dto::ModelNode& _node)
    {
        ModelNode node = {};
        node.name      = Dto::Unwrap(_node.name);
        node.localMtx  = Dto::Unwrap(_node.localMtx);
        node.meshIdxes = Dto::Unwrap<Int32>(_node.meshIdxes);
        node.matIdxes  = Dto::Unwrap<Int32>(_node.matIdxes);

        // children
        node.children.reserve(_node.children.size());
        for (const Dto::ModelNode& child: _node.children)
        {
            node.children.emplace_back(UnwrapModelNode_Recursive(child));
        }

        return node;
    }

    using ModelLoader = AssetLoader<Dto::Model>;
    using ModelSaver  = AssetSaver<Dto::Model>;

}   // namespace

Ref<Model> Model::Load()
{
    return new Model;
}

Expect<Ref<Model>> Model::LoadFromFile(
    const StringView _path)
{
    Ref<Model> pModel = new Model;
    if (const Error err = pModel->LoadFromFile_(_path))
    {
        return err;
    }
    return pModel;
}

Error Model::LoadFromFile_(
    const StringView _path)
{
    JAM_FUNCTION_PROFILE();

    // open
    Expect<ModelLoader> loader = ModelLoader::Open(_path);
    if (!loader)
    {
        return loader.GetError();
    }

    // read
    Expect<Dto::Model> modelOrNot = loader->Read();
    if (!modelOrNot)
    {
        return modelOrNot.GetError();
    }

    CleanUp_();

    const Dto::Model& model = modelOrNot.GetValue();
    m_name                  = Dto::Unwrap(model.name);
    m_root                  = UnwrapModelNode_Recursive(model.root);

    // meshes
    m_meshes.reserve(modelOrNot->meshPaths.size());
    for (const Dto::StringView path: modelOrNot->meshPaths)
    {
        AssetManager&     mgr   = GetAssetManager();
        Expect<Ref<Mesh>> pMesh = mgr.LoadMesh(Dto::Unwrap(path));
        if (!pMesh)
        {
            CleanUp_();
            return pMesh.GetError();
        }

        m_meshes.emplace_back(pMesh.Take());
    }

    // materials
    m_materials.reserve(modelOrNot->matPaths.size());
    for (const Dto::StringView path: modelOrNot->matPaths)
    {
        AssetManager&         mgr       = GetAssetManager();
        Expect<Ref<Material>> pMaterial = mgr.LoadMaterial(Dto::Unwrap(path));
        if (!pMaterial)
        {
            CleanUp_();
            return pMaterial.GetError();
        }

        m_materials.emplace_back(pMaterial.Take());
    }

    // finalize
    m_path   = _path;
    m_bValid = true;
    return Error::None;
}

Error Model::Save(
    const StringView                 _path,
    const bool                       _bReplaceExtension,
    const eFileWriterCollisionPolicy _policy) const
{
    JAM_FUNCTION_PROFILE();
    JAM_ASSERT(IsValid(), "Cannot save invalid model");

    String path = _path;
    if (_bReplaceExtension)
    {
        path.ReplaceExtension(kExtension);
    }

    Expect<ModelSaver> saver = ModelSaver::Open(path.ToStringView(), _policy);
    if (!saver)
    {
        return saver.GetError();
    }

    // data
    Dto::Model model = {};
    model.name       = Dto::Wrap(m_name);
    model.root       = WrapModelNode_Recursive(m_root);

    // meshes
    model.meshPaths.reserve(static_cast<UInt32>(m_meshes.size()));
    for (const Ref<Mesh>& pMesh: m_meshes)
    {
        model.meshPaths.emplace_back(Dto::Wrap(pMesh->GetPath()));
    }

    // materials
    model.matPaths.reserve(static_cast<UInt32>(m_materials.size()));
    for (const Ref<Material>& pMaterial: m_materials)
    {
        model.matPaths.emplace_back(Dto::Wrap(pMaterial->GetPath()));
    }

    // serialize
    return saver->Write(model);
}

void Model::SetName(
    const StringView _name)
{
    m_name = _name;
}

void Model::Invalidate()
{
    CleanUp_();
}

bool Model::IsValid() const
{
    return m_bValid;
}

eAsset Model::GetAssetType() const
{
    return kAssetType;
}

StringView Model::GetTypeName() const
{
    return kTypeName;
}

UInt32 Model::GetTypeHash() const
{
    return kTypeHash;
}

StringView Model::GetPath() const
{
    return m_path;
}

StringView Model::GetName() const
{
    return m_name;
}

StringView Model::GetExtension() const
{
    return kExtension;
}

StringView Model::GetFilter() const
{
    return kFilter;
}

void Model::SetRootNode(
    const ModelNode& _root)
{
    m_root = _root;
}

void Model::SetRootNode(
    ModelNode&& _root)
{
    m_root = std::move(_root);
}

void Model::SetMeshes(
    const Vector<Ref<Mesh>>& _meshes)
{
    m_meshes = _meshes;
}

void Model::SetMeshes(
    Vector<Ref<Mesh>>&& _meshes)
{
    m_meshes = std::move(_meshes);
}

void Model::SetMaterials(
    const Vector<Ref<Material>>& _materials)
{
    m_materials = _materials;
}

void Model::SetMaterials(
    Vector<Ref<Material>>&& _materials)
{
    m_materials = std::move(_materials);
}

const ModelNode& Model::GetRootNode() const
{
    return m_root;
}

const Vector<Ref<Mesh>>& Model::GetMeshes() const
{
    return m_meshes;
}

const Vector<Ref<Material>>& Model::GetMaterials() const
{
    return m_materials;
}

void Model::CleanUp_()
{
    m_path.Clear();
    m_name.Clear();
    m_bValid = false;
    m_root   = {};
    m_meshes.clear();
    m_materials.clear();
}

void WriteToArchive(
    ArchiveWriter&    _archive,
    const Ref<Model>& _pModel)
{
    if (_pModel && _pModel->IsValid())
    {
        _archive.Write(_pModel->GetPath());
    }
}

Error ReadFromArchive(
    ArchiveReader& _archive,
    Ref<Model>&    _value)
{
    StringView path = {};
    if (const Error err = _archive.Read(path))
    {
        return err;
    }

    Expect<Ref<Model>> pModel = GetAssetManager().LoadModel(path);
    if (pModel)
    {
        _value = pModel.Take();
    }

    return pModel.GetError();
}

}   // namespace Jam