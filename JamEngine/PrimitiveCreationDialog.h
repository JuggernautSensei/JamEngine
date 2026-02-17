#pragma once
#include "AssetCreationDialog.h"
#include "Dialog.h"
#include "Mesh.h"
#include "WindowsUtilities.h"

namespace Jam
{

class VertexLayoutEntry;

enum class ePrimitiveCreationType
{
    Cube,
    Sphere,
    Plane,
    Circle,
    Cone,
    Cylinder
};

struct PrimitiveCreationDialogDesc
{
    DialogInfo             baseInfo       = {};
    ePrimitiveCreationType type           = ePrimitiveCreationType::Cube;
    Path                   saveDirOrEmpty = {};
};

class PrimitiveCreationDialog : public AssetCreationDialog
{
public:
    explicit PrimitiveCreationDialog(const PrimitiveCreationDialogDesc& _desc);
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
    bool      CreateAndSaveAsset_(const Path& _path) override;

    ePrimitiveCreationType m_type = ePrimitiveCreationType::Cube;
    Ref<VertexLayoutEntry> m_pVle = nullptr;
    FileDialog             m_saveFd { eFileDialogMode::SaveFile };

    Path   m_assetSaveDir  = {};
    String m_assetBaseName = {};

    float m_fParam0 = 1.f;
    float m_fParam1 = 1.f;
    float m_fParam2 = 1.f;
    Int32 m_iParam  = 1;
};

}   // namespace Jam