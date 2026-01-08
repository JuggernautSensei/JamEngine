#pragma once
#include "AssetDialog.h"
#include "Mesh.h"
#include "WindowsUtilities.h"

namespace jam
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

class PrimitiveCreationDialog : public AssetDialog
{
public:
    explicit PrimitiveCreationDialog(ePrimitiveCreationType _meshType, const fs::path& _dirOrEmpty);
    void OnShow() override;

private:
    void Panel_CubeCreation_();
    void Panel_SphereCreation_();
    void Panel_PlaneCreation_();
    void Panel_CircleCreation_();
    void Panel_ConeCreation_();
    void Panel_CylinderCreation_();

    bool OnClickCreateButton_() const;

    ePrimitiveCreationType m_meshType = ePrimitiveCreationType::Cube;
    fs::path               m_dir      = {};
    std::string            m_name     = {};
    Ref<VertexLayoutEntry> m_pVle     = nullptr;
    FileDialog             m_fd;

    float m_fParam0 = 1.f;
    float m_fParam1 = 1.f;
    float m_fParam2 = 1.f;
    Int32 m_iParam  = 1;
};

}   // namespace jam