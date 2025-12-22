#pragma once
#include "IAsset.h"
#include "Magic.h"

namespace jam
{

struct ModelNode
{
    std::string name = {};
    Mat4        mtx  = Mat4::Identity;

    Vector<Int32>     meshIndices     = {};
    Vector<Int32>     materialIndices = {};
    Vector<ModelNode> children        = {};
};

struct ModelDesc
{
    ModelNode        rootNode  = {};
    Vector<fs::path> meshPaths = {};
    Vector<fs::path> matPaths  = {};
};

class Model : public IAsset
{
public:
    bool Load(const fs::path& _path) override;
    void LoadMemory(const ModelDesc& _desc, const fs::path& _virtualPath = {});
    void Unload() override;
    bool Save(const fs::path& _path) const override;

    [[nodiscard]] bool                    IsValid() const override;
    [[nodiscard]] eAssetType              GetAssetType() const override;
    [[nodiscard]] StringView        GetTypeName() const override;
    [[nodiscard]] UInt32                  GetTypeHash() const override;
    [[nodiscard]] const fs::path&         GetPath() const override;
    [[nodiscard]] const ModelNode&        GetRootNode() const;
    [[nodiscard]] const Vector<fs::path>& GetMeshs() const;
    [[nodiscard]] const Vector<fs::path>& GetMaterials() const;

    static constexpr UInt32           k_hash      = HashOf<Model>();
    static constexpr StringView k_name      = NameOf<Model>();
    static constexpr eAssetType       k_type      = eAssetType::Model;
    static constexpr StringView k_extension = k_jamModelExt;

private:
    fs::path         m_path      = {};
    ModelNode        m_root      = {};
    Vector<fs::path> m_meshPaths = {};
    Vector<fs::path> m_matPaths  = {};
    bool             m_bValid    = false;
};

}   // namespace jam