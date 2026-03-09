#pragma once
#include "Archive.h"
#include "File.h"
#include "IAsset.h"

namespace Jam
{

class Material;
class Mesh;

struct ModelNode
{
    String            name      = {};
    Matrix4x4              mtx       = Mtx4::k_identity;
    Vector<Int32>     meshIdxes = {};
    Vector<Int32>     matIdxes  = {};
    Vector<ModelNode> children  = {};
};

class Model : public IAsset
{
public:
    [[nodiscard]] static Expect<Ref<Model>> Create(StringView _path);
    [[nodiscard]] static Ref<Model>         Create(const ModelNode& _node, const Vector<Ref<Mesh>>& _meshes, const Vector<Ref<Material>>& _mats);

    Error Load(StringView _path) override;
    void  Load(const ModelNode& _node, const Vector<Ref<Mesh>>& _meshes, const Vector<Ref<Material>>& _mats);
    Error Save(StringView _path, bool _bReplaceExtension = true, eFileWriterCollisionPolicy _policy = eFileWriterCollisionPolicy::Overwrite) const override;
    void  SetName(StringView _name) override;
    void  SetVirtualPath(StringView _path) override;
    void  Invalidate() override;

    [[nodiscard]] bool       IsValid() const override;
    [[nodiscard]] eAssetType GetAssetType() const override;
    [[nodiscard]] StringView GetTypeName() const override;
    [[nodiscard]] UInt32     GetTypeHash() const override;
    [[nodiscard]] StringView GetPath() const override;
    [[nodiscard]] StringView GetName() const override;
    [[nodiscard]] StringView GetExtension() const override;
    [[nodiscard]] StringView GetFilter() const override;

    [[nodiscard]] const ModelNode&             GetRoot() const;
    [[nodiscard]] const Vector<Ref<Mesh>>&     GetMeshes() const;
    [[nodiscard]] const Vector<Ref<Material>>& GetMaterials() const;

    static constexpr StringView k_typeName  = NameOf<Model>();
    static constexpr UInt32     k_typeHash  = HashOf<Model>();
    static constexpr eAssetType k_assetType = eAssetType::Model;
    static constexpr StringView k_extension = ".jmdl";
    static constexpr StringView k_filter    = "*.jmdl";

private:
    void CleanUp_();

    String                m_path            = {};
    String                m_name            = {};
    ModelNode             m_root            = {};
    Vector<Ref<Mesh>>     m_meshes          = {};
    Vector<Ref<Material>> m_materials       = {};
    bool                  m_bValid          = false;
    bool                  m_bLoadedFromFile = false;
};

void  WriteToArchive(ArchiveWriter& _archive, const Ref<Model>& _pModel);
Error ReadFromArchive(ArchiveReader& _archive, Ref<Model>& _value);
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Ref<Model>);

}   // namespace Jam