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
    Mtx4              mtx       = Numbers<Mtx4>::k_identity;
    Vector<Int32>     meshIdxes = {};
    Vector<Int32>     matIdxes  = {};
    Vector<ModelNode> children  = {};
};

struct ModelDesc
{
    ModelNode             root      = {};
    Vector<Ref<Mesh>>     meshes    = {};
    Vector<Ref<Material>> materials = {};
};

class Model : public IAsset
{
public:
    [[nodiscard]] static Ref<Model>         Load(const ModelDesc& _desc);
    [[nodiscard]] static Ref<Model>         Load(ModelDesc&& _desc);
    [[nodiscard]] static Expect<Ref<Model>> LoadFromFile(StringView _path);

    Error Save(StringView _path, bool _bReplaceExtension = true, eFileWriterCollisionPolicy _policy = eFileWriterCollisionPolicy::Overwrite) const override;
    void  SetName(StringView _name) override;
    void  SetVirtualPath(StringView _path) override;
    void  Invalidate() override;

    [[nodiscard]] bool       IsValid() const override;
    [[nodiscard]] eAsset GetAssetType() const override;
    [[nodiscard]] StringView GetTypeName() const override;
    [[nodiscard]] UInt32     GetTypeHash() const override;
    [[nodiscard]] StringView GetPath() const override;
    [[nodiscard]] StringView GetName() const override;
    [[nodiscard]] StringView GetExtension() const override;
    [[nodiscard]] StringView GetFilter() const override;

    [[nodiscard]] const ModelDesc&             GetDesc() const;
    [[nodiscard]] const ModelNode&             GetRootNode() const;
    [[nodiscard]] const Vector<Ref<Mesh>>&     GetMeshes() const;
    [[nodiscard]] const Vector<Ref<Material>>& GetMaterials() const;

    static constexpr StringView k_typeName  = NameOf<Model>();
    static constexpr UInt32     k_typeHash  = HashOf<Model>();
    static constexpr eAsset k_assetType = eAsset::Model;
    static constexpr StringView k_extension = ".jmdl";
    static constexpr StringView k_filter    = "*.jmdl";

private:
    Model() = default;
    explicit Model(const ModelDesc& _desc);
    explicit Model(ModelDesc&& _desc);
    Error Load_(StringView _path) override;
    void  CleanUp_();

    String    m_path            = {};
    String    m_name            = {};
    ModelDesc m_desc            = {};
    bool      m_bValid          = false;
    bool      m_bLoadedFromFile = false;
};

void  WriteToArchive(ArchiveWriter& _archive, const Ref<Model>& _pModel);
Error ReadFromArchive(ArchiveReader& _archive, Ref<Model>& _value);
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Ref<Model>);

}   // namespace Jam