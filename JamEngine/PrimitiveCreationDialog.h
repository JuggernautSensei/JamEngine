#pragma once
#include "AssetCreationDialog.h"
#include "FileDialog.h"
#include "Mesh.h"

namespace Jam
{

class VertexLayoutEntry;

enum class ePrimitiveCreation
{
    Cube,
    Sphere,
    Plane,
    Circle,
    Cone,
    Cylinder
};

struct PrimitiveCreationDialogInfo
{
    StringView             title          = {};
    ImVec2                 size           = {};
    ePrimitiveCreation type           = ePrimitiveCreation::Cube;
    String                 saveDirOrEmpty = {};
};

class PrimitiveCreationDialog : public AssetCreationDialog
{
public:
    explicit PrimitiveCreationDialog(const PrimitiveCreationDialogInfo& _desc);
    void OnShow() override;

private:
    void Panel_CubeCreation_();
    void Panel_SphereCreation_();
    void Panel_PlaneCreation_();
    void Panel_CircleCreation_();
    void Panel_ConeCreation_();
    void Panel_CylinderCreation_();

    bool      OnClickCreateButton_();
    Ref<Mesh> CreatePrimitiveMesh_() const;
    bool      CreateAndSaveAsset_(StringView _path) override;

    ePrimitiveCreation m_type = ePrimitiveCreation::Cube;
    Ref<VertexLayoutEntry> m_pVle = nullptr;
    FileDialog             m_saveFd;

    String m_assetSaveDir  = {};
    String m_assetBaseName = {};

    float m_fParam0 = 1.f;
    float m_fParam1 = 1.f;
    float m_fParam2 = 1.f;
    Int32 m_iParam  = 1;
};

}   // namespace Jam