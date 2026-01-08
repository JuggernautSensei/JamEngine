#pragma once
#include "IAsset.h"
#include "Magics.h"

namespace jam
{
class Material;
class Mesh;

struct ModelNode
{
    std::string       name      = {};
    Mat4              mtx       = Mat4::Identity;
    Vector<Int32>     meshIdxes = {};
    Vector<Int32>     matIdxes  = {};
    Vector<ModelNode> children  = {};
};

class Model : public IAsset
{
public:
    bool Load(const fs::path& _path) override;
    void LoadMemory(const ModelNode& _node, const Vector<Ref<Mesh>>& _meshes, const Vector<Ref<Material>>& _mats);
    void Unload() override;
    bool Save(const fs::path& _path, bool _bReplaceExtension = true, eSaveFileFlags _flags = eSaveFileFlags_None) const override;
    void SetName(StringView _name) override;
    void SetVirtualPath(const fs::path& _path) override;

    [[nodiscard]] bool                         IsValid() const override;
    [[nodiscard]] eAssetType                   GetAssetType() const override;
    [[nodiscard]] StringView                   GetTypeName() const override;
    [[nodiscard]] UInt32                       GetTypeHash() const override;
    [[nodiscard]] const fs::path&              GetPath() const override;
    [[nodiscard]] const StringView             GetName() const override;
    [[nodiscard]] const ModelNode&             GetRootNode() const;
    [[nodiscard]] const Vector<Ref<Mesh>>&     GetMeshs() const;
    [[nodiscard]] const Vector<Ref<Material>>& GetMaterials() const;

    static constexpr UInt32     k_hash = HashOf<Model>();
    static constexpr StringView k_name = NameOf<Model>();
    static constexpr eAssetType k_type = eAssetType::Model;

private:
    void CleanUp_();

    fs::path              m_path            = {};
    std::string           m_name            = {};
    ModelNode             m_root            = {};
    Vector<Ref<Mesh>>     m_meshes          = {};
    Vector<Ref<Material>> m_mats            = {};
    bool                  m_bValid          = false;
    bool                  m_bLoadedFromFile = false;
};

}   // namespace jam