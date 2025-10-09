#pragma once
#include "AssimpBase.h"
#include "IImporter.h"
#include "Material.h"
#include "Mesh.h"

namespace jam
{

class AssimpImporter : public IImporter
{
    enum class eNormalGenType
    {
        None,
        Flat,
        Smooth
    };

public:
    eErrorCode Import() override;
    void       Reset() override;

    void ComputeTangent(bool _bEnable);
    void ComputeNormal(eNormalGenType _type);
    void DropExistingNormal(bool _bEnable);
    void LimitBoneWeight(bool _bEnable);
    void LoadOptimizationForStaticMesh(bool _bEnable);

    void SetPath(const fs::path& _filePath) override;

    JAM_NODISCARD std::span<const AssimpMesh> GetMeshes() const;
    JAM_NODISCARD std::span<const AssimpMaterial> GetMaterials() const;

    JAM_NODISCARD std::vector<Mesh> CreateMeshes(const VertexLayout& _layout) const;
    JAM_NODISCARD std::vector<Material> CreateMaterial() const;

private:
    fs::path m_filePath = {};

    std::vector<AssimpMesh>     m_meshes    = {};
    std::vector<AssimpMaterial> m_materials = {};

    // options
    bool           m_bComputeTangent    = true;
    eNormalGenType m_normalGenType      = eNormalGenType::None;
    bool           m_dropExistingNormal = false;
    bool           m_bLimitBondWeights  = false;
    bool           m_bLoadStaticMesh    = false;
};

}   // namespace jam