#pragma once
#include "IAsset.h"
#include "Magic.h"
#include "Mesh.h"

namespace jam
{

struct ModelNode
{
    std::string name      = {};
    Mat4        transform = Mat4::Identity;

    std::vector<Int32>     meshIndices     = {};
    std::vector<Int32>     materialIndices = {};
    std::vector<ModelNode> children        = {};
};

struct ModelDesc
{
    ModelNode             rootNode  = {};
    std::vector<fs::path> meshPaths = {};
    std::vector<fs::path> matPaths  = {};
};

class Model : public IAsset
{
public:
    bool Load(const fs::path& _path) override;
    void LoadFromMemory(const ModelDesc& _desc, const fs::path& _virtualPath = {});
    void Unload() override;
    bool Save(const fs::path& _path) const override;

    [[nodiscard]] bool                         IsValid() const override;
    [[nodiscard]] eAssetType                   GetAssetType() const override;
    [[nodiscard]] std::string_view             GetTypeName() const override;
    [[nodiscard]] UInt32                       GetTypeHash() const override;
    [[nodiscard]] const fs::path&              GetPath() const override;
    [[nodiscard]] const ModelNode&             GetRootNode() const;
    [[nodiscard]] const std::vector<fs::path>& GetMeshs() const;
    [[nodiscard]] const std::vector<fs::path>& GetMaterials() const;

    static constexpr UInt32           k_hash      = HashOf<Model>();
    static constexpr std::string_view k_name      = NameOf<Model>();
    static constexpr eAssetType       k_type      = eAssetType::Model;
    static constexpr std::string_view k_extension = k_jamModelExt;

private:
    fs::path              m_path      = {};
    ModelNode             m_root      = {};
    std::vector<fs::path> m_meshPaths = {};
    std::vector<fs::path> m_matPaths  = {};
    bool                  m_bValid    = false;
};

}   // namespace jam