#pragma once
#include "Asset.h"
#include "Model.h"

namespace jam
{

class ModelAsset : public Asset
{
public:
    ModelAsset()           = default;
    ~ModelAsset() override = default;

    ModelAsset(ModelAsset&&) noexcept            = default;
    ModelAsset& operator=(ModelAsset&&) noexcept = default;
    ModelAsset(const ModelAsset&)                = delete;
    ModelAsset& operator=(const ModelAsset&)     = delete;

    bool Load(AssetManager& _assetMgrRef, const fs::path& _path) override;
    bool Save(const fs::path& _path) const override;
    void Unload() override;

    NODISCARD eAssetType   GetType() const override;
    NODISCARD const Model& GetModel() const { return m_model; }
    NODISCARD Model&       GetModelRef() { return m_model; }

    constexpr static eAssetType s_type = eAssetType::Model;

private:
    Model m_model;
};

}   // namespace jam