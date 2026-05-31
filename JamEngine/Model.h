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
    Mtx4              localMtx  = Mtx4::kIdentity;
    Vector<Int32>     meshIdxes = {};
    Vector<Int32>     matIdxes  = {};
    Vector<ModelNode> children  = {};
};

class Model : public IAsset
{
public:
    [[nodiscard]] static Ref<Model>         Load();
    [[nodiscard]] static Expect<Ref<Model>> LoadFromFile(StringView _path);

    Error Save(StringView _path, bool _bReplaceExtension = true, eFileWriterCollisionPolicy _policy = eFileWriterCollisionPolicy::Overwrite) const override;
    void  SetName(StringView _name) override;
    void  Invalidate() override;

    [[nodiscard]] bool       IsValid() const override;
    [[nodiscard]] eAsset     GetAssetType() const override;
    [[nodiscard]] StringView GetTypeName() const override;
    [[nodiscard]] UInt64 GetTypeHash() const override;
    [[nodiscard]] StringView GetPath() const override;
    [[nodiscard]] StringView GetName() const override;
    [[nodiscard]] StringView GetExtension() const override;
    [[nodiscard]] StringView GetFilter() const override;

    void SetRootNode(const ModelNode& _root);
    void SetRootNode(ModelNode&& _root);
    void SetMeshes(const Vector<Ref<Mesh>>& _meshes);
    void SetMeshes(Vector<Ref<Mesh>>&& _meshes);
    void SetMaterials(const Vector<Ref<Material>>& _materials);
    void SetMaterials(Vector<Ref<Material>>&& _materials);

    [[nodiscard]] const ModelNode&             GetRootNode() const;
    [[nodiscard]] const Vector<Ref<Mesh>>&   GetMeshes() const;
    [[nodiscard]] const Vector<Ref<Material>>& GetMaterials() const;

    static constexpr StringView kTypeName  = NameOf<Model>();
    static constexpr UInt64     kTypeHash  = HashOf<Model>();
    static constexpr eAsset     kAssetType = eAsset::Model;
    static constexpr StringView kExtension = ".jmdl";
    static constexpr StringView kFilter    = "*.jmdl";

private:
    Model() = default;
    Error LoadFromFile_(StringView _path) override;
    void  CleanUp_();

    String m_path   = {};
    String m_name   = {};
    bool   m_bValid = false;

    ModelNode             m_root      = {};
    Vector<Ref<Mesh>>   m_meshes    = {};
    Vector<Ref<Material>> m_materials = {};
};

void  WriteToArchive(ArchiveWriter& _archive, const Ref<Model>& _pModel);
Error ReadFromArchive(ArchiveReader& _archive, Ref<Model>& _value);
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Ref<Model>);

}   // namespace Jam