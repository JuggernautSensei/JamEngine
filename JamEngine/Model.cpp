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

Ref<Model> Model::Load(
    const ModelDesc& _desc)
{
    return new Model(_desc);
}

Ref<Model> Model::Load(
    ModelDesc&& _desc)
{
    return new Model(std::move(_desc));
}

Expect<Ref<Model>> Model::LoadFromFile(
    const StringView _path)
{
    Ref<Model> pModel = new Model;
    if (const Error err = pModel->Load_(_path))
    {
        return err;
    }
    return pModel;
}

Model::Model(
    const ModelDesc& _desc)
    : m_desc(_desc)
{
}

Model::Model(
    ModelDesc&& _desc)
    : m_desc(std::move(_desc))
{
}

Error Model::Load_(
    const StringView _path)
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
    m_name      = StringFrom(pDto->name);
    m_desc.root = CreateModelNode_Recursive(pDto->root);

    // meshes
    m_desc.meshes.reserve(pDto->meshPaths.size());
    for (const StringViewDto path: pDto->meshPaths)
    {
        Expect<Ref<Mesh>> pMesh = GetAssetManager().LoadMesh(StringViewFrom(path));
        if (!pMesh)
        {
            CleanUp_();
            return pMesh.GetError();
        }

        m_desc.meshes.emplace_back(pMesh.Take());
    }

    // materials
    m_desc.materials.reserve(pDto->matPaths.size());
    for (const StringViewDto path: pDto->matPaths)
    {
        Expect<Ref<Material>> pMaterial = GetAssetManager().LoadMaterial(StringViewFrom(path));
        if (!pMaterial)
        {
            CleanUp_();
            return pMaterial.GetError();
        }

        m_desc.materials.emplace_back(pMaterial.Take());
    }

    // finalize
    m_path            = _path;
    m_bValid          = true;
    m_bLoadedFromFile = true;
    return Error::None;
}

Error Model::Save(
    StringView                 _path,
    const bool                 _bReplaceExtension,
    eFileWriterCollisionPolicy _policy) const
{
    JAM_FUNCTION_PROFILE();
    JAM_ASSERT(IsValid(), "Cannot save invalid model");

    StringView dir = {}, base = {}, ext = {};
    _path.Decompose(&dir, nullptr, &base, &ext);

    String path = dir;
    path.Append(base);
    path.Append(_bReplaceExtension ? k_extension : ext);

    Expect<ModelSaver> saver = ModelSaver::Open(path.ToStringView(), _policy);
    if (!saver)
    {
        return saver.GetError();
    }

    // data
    ModelDto dto = {};
    dto.name     = StringViewDtoFrom(m_name);
    dto.root     = CreateModelNodeData_Recursive(m_desc.root);

    // meshes
    dto.meshPaths.reserve(NumericCast<UInt32>(m_desc.meshes.size()));
    for (const Ref<Mesh>& pMesh: m_desc.meshes)
    {
        dto.meshPaths.emplace_back(StringViewDtoFrom(pMesh->GetPath()));
    }

    // materials
    dto.matPaths.reserve(NumericCast<UInt32>(m_desc.materials.size()));
    for (const Ref<Material>& pMaterial: m_desc.materials)
    {
        dto.matPaths.emplace_back(StringViewDtoFrom(pMaterial->GetPath()));
    }

    // serialize
    return saver->Write(dto);
}

void Model::SetName(
    const StringView _name)
{
    m_name = _name;
}

void Model::SetVirtualPath(
    StringView _path)
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

eAsset Model::GetAssetType() const
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
    return k_extension;
}

StringView Model::GetFilter() const
{
    return k_filter;
}

const ModelDesc& Model::GetDesc() const
{
    return m_desc;
}

const ModelNode& Model::GetRootNode() const
{
    return m_desc.root;
}

const Vector<Ref<Mesh>>& Model::GetMeshes() const
{
    return m_desc.meshes;
}

const Vector<Ref<Material>>& Model::GetMaterials() const
{
    return m_desc.materials;
}

void Model::CleanUp_()
{
    m_path.Clear();
    m_name.Clear();
    m_desc            = {};
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

    Expect<Ref<Model>> pModel = GetAssetManager().LoadModel(path);
    if (pModel)
    {
        _value = pModel.Take();
    }

    return pModel.GetError();
}

}   // namespace Jam