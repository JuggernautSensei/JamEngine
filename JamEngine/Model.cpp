#include "pch.h"

#include "Model.h"

#include "AssetManager.h"
#include "AssetSaveLoader.h"
#include "Material.h"
#include "Mesh.h"
#include "Profile.h"

namespace Jam
{

namespace
{

    struct ModelNodeDto
    {
        StringViewDto           name      = {};
        Mtx4Dto                 mtx       = {};
        VectorDto<Int32>        meshIdxes = {};
        VectorDto<Int32>        matIdxes  = {};
        VectorDto<ModelNodeDto> children  = {};
    };

    struct ModelDto
    {
        StringViewDto            name      = {};
        ModelNodeDto             root      = {};
        VectorDto<StringViewDto> meshPaths = {};
        VectorDto<StringViewDto> matPaths  = {};
    };

    [[nodiscard]] ModelNodeDto CreateModelNodeData_Recursive(
        const ModelNode& _node)
    {
        ModelNodeDto node = {};
        node.name         = StringViewDtoFrom(_node.name);
        node.mtx          = Mtx4Dto::FromMat4(_node.mtx);
        node.meshIdxes    = VectorDtoFrom(_node.meshIdxes);
        node.matIdxes     = VectorDtoFrom(_node.matIdxes);

        // children
        node.children.reserve(NumericCast<Int32>(_node.children.size()));
        for (const ModelNode& child: _node.children)
        {
            node.children.emplace_back(CreateModelNodeData_Recursive(child));
        }
        return node;
    }

    [[nodiscard]] ModelNode CreateModelNode_Recursive(
        const ModelNodeDto& _node)
    {
        ModelNode node = {};
        node.name      = StringFrom(_node.name);
        node.mtx       = _node.mtx.ToMtx4();
        node.meshIdxes = VectorFrom(_node.meshIdxes);
        node.matIdxes  = VectorFrom(_node.matIdxes);

        // children
        node.children.reserve(_node.children.size());
        for (const ModelNodeDto& child: _node.children)
        {
            node.children.emplace_back(CreateModelNode_Recursive(child));
        }

        return node;
    }

    using ModelLoader = AssetLoader<ModelDto>;
    using ModelSaver  = AssetSaver<ModelDto>;

}   // namespace

Expect<Ref<Model>> Model::Create(
    const Path& _path)
{
    Ref<Model> pModel = new Model;
    if (const Error err = pModel->Load(_path))
    {
        return err;
    }
    return pModel;
}

Ref<Model> Model::Create(
    const ModelNode&             _node,
    const Vector<Ref<Mesh>>&     _meshes,
    const Vector<Ref<Material>>& _mats)
{
    Ref<Model> pModel = new Model;
    pModel->Load(_node, _meshes, _mats);
    return pModel;
}

Error Model::Load(
    const Path& _path)
{
    JAM_FUNCTION_PROFILE();

    // read
    Expect<ModelLoader> loader = ModelLoader::Open(_path);
    if (!loader)
    {
        return loader.GetError();
    }

    // deserialize
    Expect<ModelDto> pDto = loader->Read();
    if (!pDto)
    {
        return pDto.GetError();
    }

    CleanUp_();
    m_name = StringFrom(pDto->name);
    m_root = CreateModelNode_Recursive(pDto->root);

    // meshes
    m_meshes.reserve(pDto->meshPaths.size());
    for (const StringViewDto path: pDto->meshPaths)
    {
        Expect<Ref<Mesh>> pMesh = JAM_ASSET_MGR.LoadMesh(PathFrom(path));
        if (!pMesh)
        {
            CleanUp_();
            return pMesh.GetError();
        }

        m_meshes.emplace_back(pMesh.Take());
    }

    // materials
    m_materials.reserve(pDto->matPaths.size());
    for (const StringViewDto path: pDto->matPaths)
    {
        Expect<Ref<Material>> pMat = JAM_ASSET_MGR.LoadMaterial(PathFrom(path));
        if (!pMat)
        {
            CleanUp_();
            return pMat.GetError();
        }

        m_materials.emplace_back(pMat.Take());
    }

    // finalize
    m_path            = _path;
    m_bValid          = true;
    m_bLoadedFromFile = true;
    return Error::None;
}

void Model::Load(
    const ModelNode&             _node,
    const Vector<Ref<Mesh>>&     _meshes,
    const Vector<Ref<Material>>& _mats)
{
    JAM_FUNCTION_PROFILE();
    CleanUp_();
    m_root      = _node;
    m_meshes    = _meshes;
    m_materials = _mats;
    m_bValid    = true;
}

Error Model::Save(
    const Path&           _path,
    const bool            _bReplaceExtension,
    Flags<eSaveFileFlags> _flags) const
{
    JAM_FUNCTION_PROFILE();
    JAM_ASSERT(IsValid(), "Cannot save invalid model");

    // io
    Expect<ModelSaver> saver = {};
    if (_bReplaceExtension)
    {
        const Path path = ReplacedExtension(_path, k_extension);
        saver           = ModelSaver::Open(path, _flags);
    }
    else
    {
        saver = ModelSaver::Open(_path, _flags);
    }

    if (!saver)
    {
        return saver.GetError();
    }

    // data
    ModelDto dto = {};
    dto.name     = StringViewDtoFrom(m_name);
    dto.root     = CreateModelNodeData_Recursive(m_root);

    // meshes
    dto.meshPaths.reserve(NumericCast<UInt32>(m_meshes.size()));
    for (const Ref<Mesh>& pMesh: m_meshes)
    {
        dto.meshPaths.emplace_back(StringViewDtoFrom(pMesh->GetPath()));
    }

    // materials
    dto.matPaths.reserve(NumericCast<UInt32>(m_materials.size()));
    for (const Ref<Material>& pMat: m_materials)
    {
        dto.matPaths.emplace_back(StringViewDtoFrom(pMat->GetPath()));
    }

    // serialize
    return saver->Write(dto);
}

void Model::SetName(
    const StringView _name)
{
    m_name = _name.ToString();
}

void Model::SetVirtualPath(
    const Path& _path)
{
    JAM_ASSERT(!m_bLoadedFromFile, "Cannot change path of model loaded from file");
    m_path = _path;
}

void Model::Invalidate()
{
    CleanUp_();
}

bool Model::IsValid() const
{
    return m_bValid;
}

eAssetType Model::GetAssetType() const
{
    return k_assetType;
}

StringView Model::GetTypeName() const
{
    return k_typeName;
}

UInt32 Model::GetTypeHash() const
{
    return k_typeHash;
}

const Path& Model::GetPath() const
{
    return m_path;
}

StringView Model::GetName() const
{
    return m_name;
}

StringView Model::GetExtension() const
{
    return k_extension;
}

StringView Model::GetFilter() const
{
    return k_filter;
}

const ModelNode& Model::GetRoot() const
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
    m_path.clear();
    m_meshes.clear();
    m_materials.clear();
    m_name.Clear();
    m_root            = {};
    m_bValid          = false;
    m_bLoadedFromFile = false;
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

    Expect<Ref<Model>> pModel = JAM_ASSET_MGR.LoadModel(path.ToStdString());
    if (pModel)
    {
        _value = pModel.Take();
    }

    return pModel.GetError();
}

}   // namespace Jam